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
        int pid = processesTableModel->getPidByRowIndex(rowIndex);

        // Отображаем в таблице потоков только потоки принадлежащие выделенному процессу:
        threadsProxyModel->setPidFilter(QString("%1").arg(pid));
        threadsProxyModel->invalidate();  // оповещаем о том, что модель изменилась и нужно обновить таблицу.

        // Автоматическая установка нужной ширины столбцов в таблице потоков:
        autosizeTable(ui->threadsTableView);
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
    // Отвязываем таблицу процессов от модели данных:
    ui->processesTableView->setModel(0);

    // Обновляем модель данных для таблицы процессов:
    processesTableModel->refreshData();

    // Связываем таблицу процессов с моделью данных:
    ui->processesTableView->setModel(processesTableModel);

    // Обработка события выделения строки в таблице процессов:
    connect(
        ui->processesTableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this,
        SLOT(processSelectionChangedSlot(const QItemSelection&, const QItemSelection&))
    );

    // Отвязываем таблицу потоков от модели данных:
    ui->threadsTableView->setModel(0);
    threadsProxyModel->setSourceModel(0);

    // Обновляем модель данных для таблицы потоков:
    threadsTableModel->refreshData();

    // Очищаем фильтрацию:
    threadsProxyModel->setPidFilter("");
    threadsProxyModel->setSourceModel(threadsTableModel);

    // Связываем таблицу потоков с моделью данных:
    ui->threadsTableView->setModel(threadsProxyModel);

    // Обработка события выделения строки в таблице потоков:
    connect(
        ui->threadsTableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        this,
        SLOT(threadSelectionChangedSlot(const QItemSelection&, const QItemSelection&))
    );

    // Очистка статусной строки:
    cpuKernelTime->setText("");
    cpuUserTime->setText("");
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
        DWORD tid = threadsProxyModel->data(threadsProxyModel->index(rowIndex, 1)).toInt();
        //qDebug() << "TID =" << tid;

        // Время проведенноё потоком в режиме ядра:
        double kernelTime = threadsTableModel->getKernelTimeByTid(tid);
        QString kernelTimeStr = QString::number(kernelTime);
        cpuKernelTime->setText(QString("Время ядра ЦП: %1 с").arg(kernelTimeStr));

        // Время проведенноё потоком в режиме пользователя:
        double userTime = threadsTableModel->getUserTimeByTid(tid);
        QString userTimeStr = QString::number(userTime);
        cpuUserTime->setText(QString("Время пользователя ЦП: %1 с").arg(userTimeStr));
    }
}
