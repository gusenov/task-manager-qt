#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

// Конструктор:
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),  // вызов родительского конструктора.
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Отключение кнопки для разворачивания окна
    // и возможности менять размеры окна:
    setFixedSize(width(), height());

    // Создание модели данных для таблицы с процессами:
    processesTableModel = new ProcessesTableModel();

    // Настройка таблицы с процессами:
    setupTable(ui->processesTableView, processesTableModel);

    // Обработка события выделения строки в таблице процессов:
    connect(
        ui->processesTableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this,
        SLOT(processSelectionChangedSlot(const QItemSelection&, const QItemSelection&))
    );

    // Создание модели данных для таблицы с потоками:
    threadsTableModel = new ThreadsTableModel();

    threadsProxyModel = new PidFilterProxyModel();
    threadsProxyModel->setSourceModel(threadsTableModel);

    // Настройка таблицы с потоками:
    setupTable(ui->threadsTableView, threadsProxyModel);

    // Обработка события выделения строки в таблице потоков:
    connect(
        ui->threadsTableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this,
        SLOT(threadSelectionChangedSlot(const QItemSelection&, const QItemSelection&))
    );

    // Настройка статусной строки:

    statusBarContainer = new QWidget();
    statusBarLayout = new QHBoxLayout();

    // Текстовая метка "Время ядра ЦП: # с":
    cpuKernelTime = new QLabel();
    statusBarLayout->addWidget(cpuKernelTime);
    statusBarLayout->addSpacing(10);

    // Текстовая метка "Время пользователя ЦП: # с":
    cpuUserTime = new QLabel();
    statusBarLayout->addWidget(cpuUserTime);
    statusBarLayout->addSpacing(10);

    statusBarLayout->setContentsMargins(0, 0, 0, 0);
    statusBarContainer->setLayout(this->statusBarLayout);
    ui->statusBar->addWidget(this->statusBarContainer);

    createProcessesChart();
    processesChartView->setChart(processesChart);

    createThreadsChart();
    threadsChartView->setChart(threadsChart);

    if (ui->processesTableView->model()->rowCount() > 0)
        ui->processesTableView->selectRow(0);

    InitializeCriticalSection(&CrSectionUpd);

    QObject::connect(&updater, SIGNAL(tick()),
        this, SLOT(on_actionRefresh_triggered()));
    updater.startUpdater();
}

// Метод для настройки внешнего вида таблиц процессов и потоков:
void MainWindow::setupTable(QTableView *tableView, QAbstractItemModel *tableModel)
{
    // Установка модели данных для таблицы:
    tableView->setModel(tableModel);

    // Выделять целиком всю строку, а не отдельные ячейки:
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Подогнать ширину столбцов:
    autosizeTable(tableView);

    // Не выделять строку с заголовками при выделении строки с данными:
    tableView->horizontalHeader()->setHighlightSections(false);

    // Убрать синее выделение строки и пунктирное выделение ячейки:
    tableView->setFocusPolicy(Qt::NoFocus);

    // Сделать возможным выделение только одной строки, а не многих:
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
}

// Подогнать ширину столбцов:
void MainWindow::autosizeTable(QTableView *tableView)
{
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

// Деструктор:
MainWindow::~MainWindow()
{
    // Удаление из памяти пользовательского интерфейса:
    delete ui;

    // Удаление из памяти фильтрующей модели данных для таблицы потоков:
    delete threadsProxyModel;

    // Удаление из памяти модели данных для таблицы потоков:
    delete threadsTableModel;

    // Удаление из памяти модели данных для таблицы процессов:
    delete processesTableModel;

    destroyProcessesChart();

    DeleteCriticalSection(&CrSectionUpd);
}

// Метод, который вызывается при выделении строки в таблице с процессами:
void MainWindow::processSelectionChangedSlot(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{
    Q_UNUSED(oldSelection);

    QModelIndexList rowIndexes = newSelection.indexes();
    if (rowIndexes.size() > 0)
    {
        // Индекс выделенной строки в таблице процессов:
        int rowIndex = rowIndexes.first().row();

        // ИД процесса:
        lastSelectedPid = processesTableModel->getPidByRowIndex(rowIndex);

        // Отображаем в таблице потоков только потоки принадлежащие выделенному процессу:
        threadsProxyModel->setPidFilter(QString("%1").arg(lastSelectedPid));

        for (int i = 0; i < threadsProxyModel->rowCount(); i++)
        {
            if (lastSelectedTid == threadsProxyModel->data(threadsProxyModel->index(i, 1)).toInt())
            {
                ui->threadsTableView->selectRow(i);
                break;
            }
        }


        foreach (QAbstractSeries* item, processesChart->series())
        {
            processesChart->removeSeries(item);
        }

        QtCharts::QLineSeries *series = processesTableModel->getMemSeriesForPid(lastSelectedPid);
//        foreach (QAbstractAxis* item, processesChart->axes())
//        {
//            series->detachAxis(item);
//        }
        processesChart->addSeries(series);
        series->attachAxis(processesAxisTime);
        series->attachAxis(processesAxisMemory);


        series = processesTableModel->getThreadCountSeriesForPid(lastSelectedPid);
//        foreach (QAbstractAxis* item, processesChart->axes())
//        {
//            series->detachAxis(item);
//        }
        processesChart->addSeries(series);
        series->attachAxis(processesAxisTime);
        series->attachAxis(processesAxisThreadsCount);

        processesAxisTime->setRange(processesTableModel->getMinTimeForPid(lastSelectedPid), QDateTime::currentDateTime());
        processesAxisMemory->setRange(0, processesTableModel->getMaxMemForPid(lastSelectedPid) * 2);
        processesAxisThreadsCount->setRange(0, processesTableModel->getMaxThreadCountForPid(lastSelectedPid) * 1.1);

        processesChartView->invalidateScene();


        foreach (QAbstractSeries* item, threadsChart->series())
            threadsChart->removeSeries(item);
        QStackedBarSeries* threadsSeries = threadsTableModel->getStackedBarSeriesForPid(lastSelectedPid);
        threadsChart->addSeries(threadsSeries);
        threadsAxisX->clear();
        threadsAxisX->append(*threadsTableModel->getCategoriesForPid(lastSelectedPid));
//        foreach (QAbstractAxis* item, threadsChart->axes())
//            threadsChart->removeAxis(item);
        threadsChart->removeAxis(threadsChart->axisX());
        threadsChart->setAxisX(threadsAxisX, threadsSeries);
        threadsAxisCPU->setMax(threadsTableModel->getMaxCpuTimeForPid(lastSelectedPid));
    }

    // Очистка статусной строки:
    cpuKernelTime->setText("");
    cpuUserTime->setText("");
}

// Пункт меню Файл -> Завершение диспетчера:
void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

// Пункт меню Вид -> Обновить (F5):
void MainWindow::on_actionRefresh_triggered()
{
    int processesTableViewVertical = ui->processesTableView->verticalScrollBar()->value();

    EnterCriticalSection(&CrSectionUpd);

    // Обновляем модель данных для таблицы процессов:
    processesTableModel->refreshData();

    // Обновляем модель данных для таблицы потоков:
    threadsTableModel->refreshData();

    // Очищаем фильтрацию:
    threadsProxyModel->setPidFilter("");

    // Очистка статусной строки:
    cpuKernelTime->setText("");
    cpuUserTime->setText("");

    for (int i = 0; i < processesTableModel->rowCount(); i++)
    {
        if (lastSelectedPid == processesTableModel->data(processesTableModel->index(i, 1)).toInt())
        {
            ui->processesTableView->selectRow(i);
            break;
        }
    }
    ui->processesTableView->verticalScrollBar()->setValue(processesTableViewVertical);

    LeaveCriticalSection(&CrSectionUpd);
}

// Метод, который вызывается при выделении строки в таблице с потоками:
void MainWindow::threadSelectionChangedSlot(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{
    // Неиспользуемая переменная:
    Q_UNUSED(oldSelection);

    QModelIndexList rowIndexes = newSelection.indexes();
    if (rowIndexes.size() > 0)
    {
        // Индекс выделенной строки в таблице потоков:
        int rowIndex = rowIndexes.first().row();

        // ИД потока:
        lastSelectedTid = threadsProxyModel->data(threadsProxyModel->index(rowIndex, 1)).toInt();
        //qDebug() << "TID =" << tid;

        // Время проведенноё потоком в режиме ядра:
        double kernelTime = threadsTableModel->getKernelTimeByTid(lastSelectedTid);
        QString kernelTimeStr = QString::number(kernelTime);
        cpuKernelTime->setText(QString("Время ядра ЦП: %1 с").arg(kernelTimeStr));

        // Время проведенноё потоком в режиме пользователя:
        double userTime = threadsTableModel->getUserTimeByTid(lastSelectedTid);
        QString userTimeStr = QString::number(userTime);
        cpuUserTime->setText(QString("Время пользователя ЦП: %1 с").arg(userTimeStr));
    }
}

// Освободить память выделенную под диаграмму процессов:
void MainWindow::destroyProcessesChart()
{
    if (processesChart)
    {
        foreach (QAbstractSeries* series, processesChart->series())
        {
            foreach (QAbstractAxis* axis, series->attachedAxes())
            {
                series->detachAxis(axis);
            }
            processesChart->removeSeries(series);
        }
    }

    if (processesAxisTime)
    {
        delete processesAxisTime;
        processesAxisTime = nullptr;
    }

    if (processesAxisMemory)
    {
        delete processesAxisMemory;
        processesAxisMemory = nullptr;
    }

    if (processesAxisThreadsCount)
    {
        delete processesAxisThreadsCount;
        processesAxisThreadsCount = nullptr;
    }

    if (processesChart)
    {
        delete processesChart;
        processesChart = nullptr;
    }

    if (processesChartView)
    {
        processesChartView->setParent(nullptr);
        delete processesChartView;
        processesChartView = nullptr;
    }
}

// Создать диаграмму процессов:
void MainWindow::createProcessesChart()
{
    processesChart = new QtCharts::QChart();
    processesChart->legend()->hide();

    processesAxisMemory = new QValueAxis;
    processesAxisMemory->setLabelFormat("%i");
    processesAxisMemory->setTitleText("Память (КБ)");
    processesAxisMemory->setMin(0);
    processesAxisMemory->setMax(1);
    processesAxisMemory->setRange(0, 1);
    processesChart->addAxis(processesAxisMemory, Qt::AlignLeft);

    processesAxisThreadsCount = new QValueAxis;
    processesAxisThreadsCount->setLabelFormat("%i");
    processesAxisThreadsCount->setTitleText("Счетчик потоков");
    processesAxisThreadsCount->setMin(0);
    processesAxisThreadsCount->setMax(1);
    processesAxisThreadsCount->setRange(0, 1);
    processesChart->addAxis(processesAxisThreadsCount, Qt::AlignRight);

    processesAxisTime = new QDateTimeAxis;
    processesAxisTime->setTickCount(1);
    processesAxisTime->setFormat("HH:mm:ss");
    processesAxisTime->setTitleText("Время в формате HH : mm : ss");
    QDateTime momentInTime = QDateTime::currentDateTime();
    processesAxisTime->setMin(momentInTime.addSecs(-1));
    processesAxisTime->setMax(momentInTime);
    processesChart->addAxis(processesAxisTime, Qt::AlignBottom);

    processesChartView = new QChartView(ui->processesWidget);
    //processesChartView->move(510, 50);
    processesChartView->resize(500, 250);
    processesChartView->setRenderHint(QPainter::Antialiasing);
    processesChartView->show();
}

// Освободить память выделенную под диаграмму потоков:
void MainWindow::destroyThreadsChart()
{
    if (threadsChart)
    {
        foreach (QAbstractSeries* series, threadsChart->series())
        {
            foreach (QAbstractAxis* axis, series->attachedAxes())
            {
                series->detachAxis(axis);
            }
            threadsChart->removeSeries(series);
        }
    }

    if (threadsAxisX)
    {
        delete threadsAxisX;
        threadsAxisX = nullptr;
    }

    if (threadsAxisCPU)
    {
        delete threadsAxisCPU;
        threadsAxisCPU = nullptr;
    }

    if (threadsChart)
    {
        delete threadsChart;
        threadsChart = nullptr;
    }

    if (threadsChartView)
    {
        threadsChartView->setParent(nullptr);
        delete threadsChartView;
        threadsChartView = nullptr;
    }
}

// Создать диаграмму потоков:
void MainWindow::createThreadsChart()
{
    threadsChart = new QtCharts::QChart();
    threadsChart->createDefaultAxes();
    threadsChart->legend()->hide();
    //threadsChart->legend()->setVisible(true);
    //threadsChart->legend()->setAlignment(Qt::AlignBottom);

    threadsAxisX = new QBarCategoryAxis();
    threadsAxisX->setLabelsAngle(-90);
    threadsAxisX->setTitleText("ИД потока (TID)");

    threadsAxisCPU = new QValueAxis;
    threadsAxisCPU->setLabelFormat("%i");
    threadsAxisCPU->setTitleText("Время ЦП (с)");
    threadsAxisCPU->setMin(0);
    threadsAxisCPU->setMax(1);
    threadsAxisCPU->setRange(0, 1);
    threadsChart->addAxis(threadsAxisCPU, Qt::AlignLeft);

    threadsChartView = new QChartView(ui->threadsWidget);
    threadsChartView->resize(500, 250);
    threadsChartView->setRenderHint(QPainter::Antialiasing);
    threadsChartView->show();
}
