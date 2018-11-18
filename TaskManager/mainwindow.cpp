#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <Tchar.h>
#include <QTest>

// Конструктор.
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);  // настроить пользовательский интерфейс.

    // Окно для создания нового процесса.
    newProcessDialog = new DialogNewProcess();

    // Окно "О программе-диспетчере для ОС Windows".
    aboutDialog = new DialogAbout();

    // Модель данных для таблицы процессов.
    processesTableModel = new TableModelProcesses(this);

    // Прокси-модель данных для таблицы процессов.
    processesProxyModel = new QSortFilterProxyModel(this);
    processesProxyModel->setSourceModel(processesTableModel);

    // Назначение модели данных таблице процессов.
    ui->tableViewProcesses->setModel(processesProxyModel);

    // Добавление обработчика событию выбора строки в таблице процессов.
    connect(ui->tableViewProcesses->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &MainWindow::processSelectionChanged);

    emit processSelectionChanged(ui->tableViewProcesses->selectionModel()->selection());

    // Статусная строка.
    this->statusBarContainer = new QWidget();
    this->statusBarLayout = new QHBoxLayout();
    this->processesCount = new QLabel(this->getProcessesCount());
    this->statusBarLayout->addWidget(this->processesCount);
    this->statusBarLayout->setContentsMargins(0, 0, 0, 0);
    this->statusBarContainer->setLayout(this->statusBarLayout);
    ui->statusBar->addWidget(this->statusBarContainer);

    // Выключение некоторых компонентов при запуске приложения.
    disableWhenStartUp();

    // Обновить информацию на вкладке "Быстродействие".
    this->updatePerformanceTabData();

    // Добавление обработчика событию смены вкладки.
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSelected()));
}

// Деструктор.
MainWindow::~MainWindow()
{
    delete ui;  // удалить из памяти пользовательский интерфейс.

    delete newProcessDialog;  // удалить из памяти окно для создания нового процесса.

    delete aboutDialog;  // удалить из памяти окно "О программе".

    delete processesProxyModel;  // удалить из памяти прокси-модель данных для таблицы процессов.
    delete processesTableModel;  // удалить из памяти модель данных для таблицы процессов.
}

// Создание нового процесса.
void MainWindow::on_actionFileNewProcess_triggered()
{
    // Показать диалоговое окно для создания нового процесса.
    int dialogCode = newProcessDialog->exec();

    if (dialogCode == QDialog::Accepted)  // если пользователь выбрал исполняемый файл и нажал на кнопку "ОК":
    {
        //qDebug() << newProcessDialog->fileName;

        // Аргументы необходимые функции CreateProcess:
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        // Обнуление и запись размеров структуры STARTUPINFO.
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        // Конвертируем путь к исполняемому файлу из QString в wchar_t*, который требует функция CreateProcess.
        wchar_t *applicationName = new wchar_t[newProcessDialog->fileName.length() + 1];
        newProcessDialog->fileName.toWCharArray(applicationName);
        applicationName[newProcessDialog->fileName.length()] = 0;

        // Вызов WinAPI функции для создания процесса.
        bool status = CreateProcess(
            //_T("C:\\Windows\\notepad.exe")
            applicationName,  // The name of the module to be executed.
            NULL,  // The command line to be executed.
            NULL,  // A pointer to a SECURITY_ATTRIBUTES structure that determines whether
                   // the returned handle to the new process object can be inherited by child processes.
            NULL,  // A pointer to a SECURITY_ATTRIBUTES structure that determines whether
                   // the returned handle to the new thread object can be inherited by child processes.
            FALSE, // If this parameter is TRUE, each inheritable handle in the calling process is inherited by the new process.
            0,     // The flags that control the priority class and the creation of the process.
            NULL,  // A pointer to the environment block for the new process.
            NULL,  // The full path to the current directory for the process.
            &si,   // A pointer to a STARTUPINFO or STARTUPINFOEX structure.
            &pi);  // A pointer to a PROCESS_INFORMATION structure
                   // that receives identification information about the new process.

        // Путь к исполняемому файлу больше не нужен, поэтому освобождается из памяти.
        delete[] applicationName;

        // Если процесс запустить не удалось, то показываем сообщение об ошибке.
        if (!status)
        {
            QString errorMsg = QString("Код ошибки: (%1)").arg(GetLastError(), 0, 16);
            //qDebug() << errorMsg;
            QMessageBox::warning(this, tr("CreateProcess failed"), errorMsg, QMessageBox::Ok, 0);
        }

        // Если процесс запустить удалось, то обновляем таблицу процессов.
        else
        {
            this->on_action_ViewRefresh_triggered();
        }
    }
}

// Выход из программы.
void MainWindow::on_actionFileClose_triggered()
{
    QApplication::quit();
}

// Завершение процесса.
void MainWindow::on_pushButtonKillProcess_clicked()
{
    int pid = this->getSelectedProcessId();  // идентификатор выделенного процесса.
    if (pid != -1)
    {
        HANDLE selectedProcessHandle = OpenProcess(PROCESS_TERMINATE, 0, pid);  // дескриптор выделенного процесса.
        if (selectedProcessHandle != 0)
        {
            // Вызов WinAPI функций для завершения процесса.
            TerminateProcess(selectedProcessHandle, 9);
            CloseHandle(selectedProcessHandle);

            QTest::qSleep(64);  // небольшое ожидание завершения процесса.
            this->on_action_ViewRefresh_triggered();  // обновляем таблицу процессов.
        }
    }
}

// Обновление таблицы процессов.
void MainWindow::on_action_ViewRefresh_triggered()
{
    processesTableModel->reloadProcessesFromSystem();  // перезагрузка модели данных.

    // Обновление количества процессов в статусной строке.
    this->processesCount->setText(this->getProcessesCount());

    // Обновление информации на вкладке "Быстродействие".
    this->updatePerformanceTabData();
}

// Код выполняющийся при выделении строки в таблице на вкладке "Процессы".
void MainWindow::processSelectionChanged(const QItemSelection &selection)
{
    QModelIndexList indexes = selection.indexes();

    // Если какой-нибудь процесс выделен, то включаем панель для работы с виртуальной памятью:
    if (!indexes.isEmpty()) {
        ui->groupBoxVirtualMemoryManagement->setEnabled(true);

    // В противном случае выключаем эту группу компонентов:
    } else {
        ui->groupBoxVirtualMemoryManagement->setEnabled(false);
    }
}

// Количество процессов в таблице процессов на вкладке "Процессы".
QString MainWindow::getProcessesCount()
{
    return QString("Процессов: %1").arg(QString::number(this->processesTableModel->getProcessesCount()));
}

// Метод возвращает идентификатор выделенного процесса в таблице на вкладке "Процессы".
// Или -1, если никакой процесс не выделен.
int MainWindow::getSelectedProcessId()
{
    // Модель выбора в таблице процессов.
    QItemSelectionModel *selectionModel = ui->tableViewProcesses->selectionModel();

    if (selectionModel->hasSelection()) {

        // Индексы выбранных процессов.
        QModelIndexList selectedRows = selectionModel->selectedRows();

        for(int i = 0; i < selectedRows.count(); ++i)
        {
            QModelIndex index = selectedRows.at(i);  // индекс выбранного процесса.

            // Указатель на информацию о выбранном процессе.
            PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = this->processesTableModel->getProcessAtIndex(index.row());

            // Идентификатор выбранного процесса.
            int pid = (int)sysProcInfoPtr->UniqueProcessId;

            return pid;
        }
    }
    return -1;
}

// Выделение виртуальной памяти.
void MainWindow::on_pushButtonAllocMemory_clicked()
{
    selectedProcessId = getSelectedProcessId();  // идентификатор выбранного процесса.

    // Получаем дескриптор выбранного процесса.
    selectedProcessHandle = OpenProcess(
        // Флаги доступа к виртуальной памяти:
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
        FALSE,
        selectedProcessId);  // идентификатор процесса.

    // Базовый адрес выделяемого региона страниц в виртуальной памяти.
    // Ставим 0, чтобы система сама выбрала.
    this->baseAddrOfAllocRegionOfPages = 0;

    // Размер выделяемого региона страниц в виртуальной памяти.
    this->actualSzInBytesOfAllocRegionOfPages = ui->spinBoxMemorySize->value() * 1024;

    //qDebug() << "actualSzInBytesOfAllocRegionOfPages = " << this->actualSzInBytesOfAllocRegionOfPages;

    // Вызов функции для выделения виртуальной памяти.
    NTSTATUS status = SystemFunctions::getInstance().NtAllocateVirtualMemoryFunction(
        this->selectedProcessHandle,  // дескриптор выбранного процесса.
        &this->baseAddrOfAllocRegionOfPages,  // базовый адрес выделяемого региона страниц в виртуальной памяти.
        0,  // ZeroBits.
        &this->actualSzInBytesOfAllocRegionOfPages,  // указатель на переменную в которую запишется фактический размер в байтах выделенного региона страниц.
        MEM_COMMIT | MEM_RESERVE,  // зарезервировать и передать виртуальную память процессу.
        PAGE_READWRITE);  // разрешено читать из памяти и записывать в память.

    // Если память удалось выделить успешно:
    if (NT_SUCCESS(status)) {

        // Обновляем отображаемую информацию в программе:
        on_action_ViewRefresh_triggered();
        ui->tableViewProcesses->selectRow(this->processesTableModel->getIndexOfProcessWithId(selectedProcessId));

        // Выключаем и включаем соответствующие компоненты.
        this->disableWhenMemoryAllocated();
        this->enableWhenMemoryAllocated();

        QString successMsg = QString("Виртуальная память зарезервирована и передеана процессу.");
        qDebug() << successMsg;
        //QMessageBox::information(this, tr("Успешно!"), successMsg, QMessageBox::Ok, 0);

    // Если возникли ошибки, то показываем соответствующие сообщения:
    } else if (status == STATUS_CONFLICTING_ADDRESSES) {
        QMessageBox::warning(this, tr("{Conflicting Address Range}"), "The specified address range conflicts with the address space.", QMessageBox::Ok, 0);
    } else if (status == (NTSTATUS)STATUS_INVALID_HANDLE) {
        QMessageBox::warning(this, tr("STATUS_INVALID_HANDLE"), "An invalid HANDLE was specified.", QMessageBox::Ok, 0);
    } else {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(status, 0, 16).arg(GetLastError(), 0, 16);
        //qDebug() << errorMsg;
        QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
    }
}

// Освобождение виртуальной памяти.
void MainWindow::on_pushButtonFreeMemory_clicked()
{
    this->actualSzInBytesOfAllocRegionOfPages = 0;
    // 0 означает, что будем освобождать весь регион.

    // Вызов функции для освобождения памяти.
    NTSTATUS status =  SystemFunctions::getInstance().NtFreeVirtualMemoryFunction(
                this->selectedProcessHandle,  // дескриптор процесса.
                &this->baseAddrOfAllocRegionOfPages,  // указатель на базовый адрес в заданном процессе.
                &this->actualSzInBytesOfAllocRegionOfPages,  // указатель на переменную в которую запишется фактический размер в байтах освобожденного региона страниц.
                MEM_RELEASE);  // битовая маска.

    // Если освобождение памяти прошло успешно:
    if (NT_SUCCESS(status)) {

        // Выключаем и включаем соответствующие компоненты.
        this->disableWhenMemoryFree();
        this->enableWhenMemoryFree();

         // Обновляем отображаемую информацию в программе:
        on_action_ViewRefresh_triggered();
        ui->tableViewProcesses->selectRow(this->processesTableModel->getIndexOfProcessWithId(selectedProcessId));

        ui->labelDataFromMemory->setText(QString());

        QString successMsg = QString("Область памяти освобождена.");
        qDebug() << successMsg;
        //QMessageBox::information(this, tr("Успешно!"), successMsg, QMessageBox::Ok, 0);

    // Если при освобождении памяти возникли ошибки, то показываем их:
    } else {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(status, 0, 16).arg(GetLastError(), 0, 16);
        //qDebug() << errorMsg;
        QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
    }
}

// Запись в виртуальную память.
void MainWindow::on_pushButtonWriteToMemory_clicked()
{
    // Переводим текст, который будем записывать в память из QString в wchar_t*.
    QString text = ui->lineEditUserDataForMemory->text();
    wchar_t *array = new wchar_t[text.length() + 1];
    text.toWCharArray(array);
    array[text.length()] = 0;

    // Вызов функции ядра NtWriteVirtualMemory для записи в память.
    NTSTATUS status = SystemFunctions::getInstance().NtWriteVirtualMemoryFunction(
        selectedProcessHandle,  // дескриптор процесса.
        this->baseAddrOfAllocRegionOfPages,  // указатель на базовый адрес в заданном процессе.
        array,  // буфер из которого списываются данные.
        (text.length() + 1) * sizeof(wchar_t),  // количество байт для записи.
        0);  // переменная для хранения количества записанных байт отсутствует.

    // Если запись данных прошла успешно:
    if (NT_SUCCESS(status)) {
        // Включаем кнопку для считывания данных из памяти.
        ui->pushButtonReadFromMemory->setEnabled(true);

        //QString successMsg = QString("Запись прошла успешно.");
        //QMessageBox::information(this, tr("Успешно!"), successMsg, QMessageBox::Ok, 0);

    // Иначе, показываем ошибки:
    } else {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(status, 0, 16).arg(GetLastError(), 0, 16);
        //qDebug() << errorMsg;
        QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
    }

    delete[] array;
}

// Чтение из виртуальной памяти.
void MainWindow::on_pushButtonReadFromMemory_clicked()
{
    // Выделяем буфер для считанных данных.
    wchar_t *array = new wchar_t[this->actualSzInBytesOfAllocRegionOfPages];

    // Вызов функции ядра NtReadVirtualMemory для чтения данных из виртуальной памяти.
    NTSTATUS status = SystemFunctions::getInstance().NtReadVirtualMemoryFunction(
        selectedProcessHandle,  // дескриптор процесса.
        this->baseAddrOfAllocRegionOfPages,  // указатель на базовый адрес в заданном процессе.
        array,  // буфер для прочитанных данных.
        actualSzInBytesOfAllocRegionOfPages,  // количество байт для чтения.
        0);  // указатель на переменную для хранения количества прочитанных байт отсутствует.

    // Чтение данных прошло успешно:
    if (NT_SUCCESS(status)) {
        //qDebug() << "wcslen = " << wcslen(array);

        // Показываем считанные данные:
        ui->labelDataFromMemory->setText(QString::fromWCharArray(array, wcslen(array)));

        //QString successMsg = QString("Чтение прошло успешно.");
        //QMessageBox::information(this, tr("Успешно!"), successMsg, QMessageBox::Ok, 0);

    // Иначе, показываем ошибки:
    } else {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(status, 0, 16).arg(GetLastError(), 0, 16);
        //qDebug() << errorMsg;
        QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
    }

    // Освобождаем из памяти буфер для считанных данных.
    delete[] array;

}

// Компоненты, которые выключаем при запуске приложения.
void MainWindow::disableWhenStartUp()
{
    ui->pushButtonFreeMemory->setEnabled(false);

    ui->lineEditUserDataForMemory->setEnabled(false);
    ui->pushButtonWriteToMemory->setEnabled(false);

    ui->pushButtonReadFromMemory->setEnabled(false);
}

// Компоненты, которые выключаем при нажатии на кнопку "Выделить память".
void MainWindow::disableWhenMemoryAllocated()
{
    ui->actionFileNewProcess->setEnabled(false);

    ui->tableViewProcesses->setEnabled(false);

    ui->pushButtonKillProcess->setEnabled(false);

    ui->spinBoxMemorySize->setEnabled(false);
    ui->pushButtonAllocMemory->setEnabled(false);

    ui->pushButtonReadFromMemory->setEnabled(false);
}

// Компоненты, которые включаем при нажатии на кнопку "Выделить память".
void MainWindow::enableWhenMemoryAllocated()
{
    ui->pushButtonFreeMemory->setEnabled(true);

    ui->lineEditUserDataForMemory->setEnabled(true);
    ui->pushButtonWriteToMemory->setEnabled(true);
}

// Компоненты, которые включаем при нажатии на кнопку "Освободить память".
void MainWindow::enableWhenMemoryFree()
{
    ui->actionFileNewProcess->setEnabled(true);

    ui->tableViewProcesses->setEnabled(true);

    ui->pushButtonKillProcess->setEnabled(true);

    ui->spinBoxMemorySize->setEnabled(true);
    ui->pushButtonAllocMemory->setEnabled(true);
}

// Компоненты, которые выключаем при нажатии на кнопку "Освободить память".
void MainWindow::disableWhenMemoryFree()
{
    ui->pushButtonFreeMemory->setEnabled(false);

    ui->lineEditUserDataForMemory->setEnabled(false);
    ui->pushButtonWriteToMemory->setEnabled(false);

    ui->pushButtonReadFromMemory->setEnabled(false);
}

// Сделать, чтобы окно программы было всегда поверх остальных окон.
void MainWindow::on_actionAlwaysOnTop_triggered()
{
    Qt::WindowFlags flags = windowFlags();  // флаги главного окна программы.

    // Вкл. режим "поверх остальных окон".
    if (ui->actionAlwaysOnTop->isChecked())
        MainWindow::setWindowFlags(flags | Qt::WindowStaysOnTopHint);

    // Выкл. режим "поверх остальных окон".
    else
        MainWindow::setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);

    // Показать главное окно программы.
    MainWindow::show();
}

// Нажатие на пункт меню "О программе".
void MainWindow::on_actionHelpAbout_triggered()
{
    aboutDialog->show();
}

// Обновление информации на вкладке "Быстродействие".
void MainWindow::updatePerformanceTabData()
{
    // Если функция NtQuerySystemInformation недоступна, то выходим из метода.
    if (!SystemFunctions::getInstance().NtQuerySystemInformationFunction)
        return;

    // Получаем дескриптор кучи.
    HANDLE heapHandle = GetProcessHeap();
    if (!heapHandle)
        return;

    ULONG sysInfoLen;  // переменная для хранения длины буфера.
    PVOID sysInfoBuffPtr;  // указатель на буфер для приема информации.
    NTSTATUS sysInfoStatus;  // статус выполнения функции ядра NtQuerySystemInformation.



    sysInfoLen = 0;
    // Если sysInfoLen равен 0, то функция NtQuerySystemInformation вернет нужную длину буфера.
    // А в качестве статуса вернет STATUS_INFO_LENGTH_MISMATCH.

    do {
        // Освобождаем буфер, если он чем-то уже занят.
        if (sysInfoBuffPtr) HeapFree(heapHandle, 0, sysInfoBuffPtr);

        // Выделяем на куче нужный объем памяти для хранения буфера.
        if (sysInfoLen > 0) {
            sysInfoBuffPtr = HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, sysInfoLen);
            if (!sysInfoBuffPtr) return;
        }

        // Получаем базовую информацию о системе, которая запишется в буфер sysInfoBuffPtr.
        sysInfoStatus = SystemFunctions::getInstance().NtQuerySystemInformationFunction(
            SystemBasicInformation,  // тип получаемой информации.
            sysInfoBuffPtr,  // указатель на буфер для приема данных.
            sysInfoLen,  // длина буфера для приема данных.
            &sysInfoLen);  // ULONG *ReturnLength

    } while (sysInfoStatus == STATUS_INFO_LENGTH_MISMATCH);

    // Если произошла ошибка, то освобождаем буфер и прекращаем выполнение метода.
    if (!NT_SUCCESS(sysInfoStatus))
    {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(sysInfoStatus, 0, 16).arg(GetLastError(), 0, 16);
        qDebug() << errorMsg;
        //QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
        if (sysInfoBuffPtr)
            HeapFree(heapHandle, 0, sysInfoBuffPtr);
        return;
    }

    PSYSTEM_BASIC_INFORMATION sysBasicInfoPtr = (PSYSTEM_BASIC_INFORMATION)sysInfoBuffPtr;

    // Размер страницы в байтах в физической памяти.
    ULONG physPageSz = sysBasicInfoPtr->PhysicalPageSize;
    //qDebug() << "PhysicalPageSize = " << physPageSz;

    // Количество всех страниц в физической памяти.
    ULONG numOfPhysPages = sysBasicInfoPtr->NumberOfPhysicalPages;
    //qDebug() << "NumberOfPhysicalPages = " << numOfPhysPages;

    // Освобождение буфера использующегося для хранения базовой информации о системе.
    HeapFree(heapHandle, 0, sysInfoBuffPtr);



    sysInfoLen = 0;
    //qDebug() << "sysInfoLen = " << sysInfoLen;

    do {
        // Освобождаем буфер, если он чем-то уже занят.
        if (sysInfoBuffPtr) HeapFree(heapHandle, 0, sysInfoBuffPtr);

        // Выделяем на куче нужный объем памяти для хранения буфера.
        if (sysInfoLen > 0) {
            sysInfoBuffPtr = HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, sysInfoLen);
            if (!sysInfoBuffPtr) return;
        }

        // Получаем информацию о быстродействии, которая запишется в буфер sysInfoBuffPtr.
        sysInfoStatus = SystemFunctions::getInstance().NtQuerySystemInformationFunction(
            SystemPerformanceInformation,  // тип получаемой информации.
            sysInfoBuffPtr,  // указатель на буфер для приема данных.
            sysInfoLen,  // длина буфера для приема данных.
            &sysInfoLen);  // ULONG *ReturnLength

        //qDebug() << "sysInfoLen = " << sysInfoLen;
        //qDebug() << QString("sysInfoStatus = %1").arg(sysInfoStatus, 0, 16);
    } while (sysInfoStatus == STATUS_INFO_LENGTH_MISMATCH);

    // Если произошла ошибка, то освобождаем буфер и прекращаем выполнение метода.
    if (!NT_SUCCESS(sysInfoStatus))
    {
        QString errorMsg = QString("Код ошибки: %1 (%2)").arg(sysInfoStatus, 0, 16).arg(GetLastError(), 0, 16);
        qDebug() << errorMsg;
        //QMessageBox::warning(this, tr("Ошибка!"), errorMsg, QMessageBox::Ok, 0);
        if (sysInfoBuffPtr)
            HeapFree(heapHandle, 0, sysInfoBuffPtr);
        return;
    }

    PSYSTEM_PERFORMANCE_INFORMATION sysPerformanceInfoPtr = (PSYSTEM_PERFORMANCE_INFORMATION)sysInfoBuffPtr;

    // Количество доступных страниц в физической памяти.
    ULONG availablePages = sysPerformanceInfoPtr->AvailablePages;
    //qDebug() << "AvailablePages = " << availablePages;

    // Освобождение буфера использующегося для хранения информации о быстродействии.
    HeapFree(heapHandle, 0, sysInfoBuffPtr);



    ui->labelPhysicalMemoryTotal->setText(QString("%1 МБ").arg(QString::number(physPageSz * numOfPhysPages / 1024 /1024)));
    ui->labelPhysicalMemoryAvailable->setText(QString("%1 МБ").arg(QString::number(physPageSz * availablePages / 1024 / 1024)));
}

// Смена выбора вкладки.
void MainWindow::tabSelected()
{
    switch (ui->tabWidget->currentIndex()) {
    case 1:
        // При переключении на вкладку "Быстродействие" обновляем ее.
        this->updatePerformanceTabData();
        break;
    default:
        break;
    }
}
