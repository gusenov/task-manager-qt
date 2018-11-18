#include "tablemodelprocesses.h"
#include <windows.h>
#include <QDebug>
#include "systemfunctions.h"

// Конструктор.
TableModelProcesses::TableModelProcesses(QObject *parent)
    : QAbstractTableModel(parent)
{
    // Сразу считываем информацию о процессах.
    readProcessesFromSystem();
}

// Деструктор.
TableModelProcesses::~TableModelProcesses()
{
    // Освобождение буфера использующегося для хранения информации о процессах.
    if (sysInfoBuffPtr)
        HeapFree(heapHandle, 0, sysInfoBuffPtr);
}

// Заголовки столбцов таблицы.
QVariant TableModelProcesses::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    // По номеру столбца возвращаем текстовый заголовок для него:

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return tr("ИД процесса (PID)");
            case 1:
                return tr("Имя образа");
            case 2:
                return tr("Память");
            case 3:
                return tr("Выделенная память");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

// Количество строк в таблице процессов.
int TableModelProcesses::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return getProcessesCount();
}

// Количество столбцов в таблице процессов.
int TableModelProcesses::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

// Метод для получения строки таблицы процессов с индексом index.
QVariant TableModelProcesses::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= getProcessesCount() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole) {

        // Процесс соответствующий строке с индексом index.
        PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = getProcessAtIndex(index.row());

        // Получения соответствующих столбцам данных:
        switch (index.column()) {
        case 0:
            // Идентификатор процесса.
            return QVariant((int)sysProcInfoPtr->UniqueProcessId);
        case 1:
            // Имя образа.
            return QString::fromWCharArray(sysProcInfoPtr->ImageName.Buffer, sysProcInfoPtr->ImageName.Length / sizeof(wchar_t));
        case 2:
            // Память.
            return QString("%1 КБ").arg(QString::number((unsigned long)sysProcInfoPtr->VirtualMemoryCounters.WorkingSetSize / 1024));
        case 3:
            // Выделенная память.
            return QString("%1 КБ").arg(QString::number((unsigned long)sysProcInfoPtr->VirtualMemoryCounters.PagefileUsage / 1024));
        default:
            break;
        }
    }
    return QVariant();
}

// Флаги строки.
Qt::ItemFlags TableModelProcesses::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // Строки включена и её можно выделять.
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// Количество процессов.
int TableModelProcesses::getProcessesCount() const
{
    // Подсчет в цикле количества процессов:

    // Указатель на начало списка процессов.
    PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)sysInfoBuffPtr;

    int idx = 0;  // счетчик процессов.

    while (sysProcInfoPtr)
    {
        ++idx;

        // Если нет следующего элемента списка процессов, то прерываем цикл.
        if (!sysProcInfoPtr->NextEntryOffset)
            break;

        // Вычисление указатель на следующий элемент списка процессов.
        sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)(((LPBYTE)sysProcInfoPtr) + sysProcInfoPtr->NextEntryOffset);
    }

    return idx;
}

// Получение процесса по индексу.
PSYSTEM_PROCESS_INFORMATION TableModelProcesses::getProcessAtIndex(int index) const
{
    // Указатель на начало списка процессов.
    PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)sysInfoBuffPtr;

    int idx = 0;  // счетчик процессов.

    while (sysProcInfoPtr)
    {

        // Если процесс найден или нет следующего элемента списка процессов, то прерываем цикл.
        if (index == idx || !sysProcInfoPtr->NextEntryOffset)
            break;

        // Вычисление указатель на следующий элемент списка процессов.
        sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)(((LPBYTE)sysProcInfoPtr) + sysProcInfoPtr->NextEntryOffset);

        ++idx;
    }
    return sysProcInfoPtr;
}

// Загрузка информации о процессах путем вызова системной функции NtQuerySystemInformation с параметром SystemProcessInformation.
void TableModelProcesses::readProcessesFromSystem()
{
    // Если функция NtQuerySystemInformation недоступна, то выходим из метода.
    if (!SystemFunctions::getInstance().NtQuerySystemInformationFunction)
        return;

    // Получаем дескриптор кучи.
    heapHandle = GetProcessHeap();
    if (!heapHandle)
        return;

    ULONG sysInfoLen = 0;  // переменная для хранения длины буфера.
    NTSTATUS sysInfoStatus;  // статус выполнения функции ядра NtQuerySystemInformation.

    do
    {
        // Освобождаем буфер, если он чем-то уже занят.
        if (sysInfoBuffPtr)
            HeapFree(heapHandle, 0, sysInfoBuffPtr);
        sysInfoBuffPtr = 0;

        // Получаем необходимую длину буфера.
        // Если sysInfoLen равен 0, то функция NtQuerySystemInformation вернет нужную длину буфера.
        // А в качестве статуса вернет STATUS_INFO_LENGTH_MISMATCH.
        sysInfoLen = 0;
        sysInfoStatus = SystemFunctions::getInstance().NtQuerySystemInformationFunction(SystemProcessInformation, sysInfoBuffPtr, sysInfoLen, &sysInfoLen);

        if (sysInfoStatus == STATUS_INFO_LENGTH_MISMATCH)
        {
            // Если длина буфера равна нулю, то прекращаем выполнение.
            if (!sysInfoLen)
                return;

            // Выделяем на куче нужный объем памяти для хранения буфера.
            sysInfoBuffPtr = HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, sysInfoLen);
            if (!sysInfoBuffPtr)
                return;

            // Получаем информацию о процессах, которая запишется в буфер sysInfoBuffPtr.
            sysInfoStatus = SystemFunctions::getInstance().NtQuerySystemInformationFunction(SystemProcessInformation, sysInfoBuffPtr, sysInfoLen, &sysInfoLen);

            // Если длина буфера оказалась не подходящей, то повторим шаги на следующей итерации цикла.
            if (sysInfoStatus == STATUS_INFO_LENGTH_MISMATCH)
                continue;

            // Если произошла другая ошибка, то освобождаем буфер и прекращаем выполнение метода.
            else if (!NT_SUCCESS(sysInfoStatus))
            {
                if (sysInfoBuffPtr)
                    HeapFree(heapHandle, 0, sysInfoBuffPtr);

                return;
            }
            else
                break;
        }
        else
            return;
    }
    while (true);

    /* ОТЛАДОЧНЫЙ ВЫВОД:

    if (!sysInfoBuffPtr)
        return;

    // Указатель на начало списка процессов.
    PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)sysInfoBuffPtr;

    while (sysProcInfoPtr)
    {
        //qDebug("UniqueProcessId: %d", sysProcInfoPtr->UniqueProcessId);
        //qDebug() << "ImageName.Buffer: " << QString::fromWCharArray(sysProcInfoPtr->ImageName.Buffer, sysProcInfoPtr->ImageName.Length / sizeof(wchar_t));
        //qDebug() << "ImageName.Length: " << sysProcInfoPtr->ImageName.Length;

        // Если нет следующего элемента списка процессов, то прерываем цикл.
        if (!sysProcInfoPtr->NextEntryOffset)
            break;

        // Вычисление указатель на следующий элемент списка процессов.
        sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)(((LPBYTE)sysProcInfoPtr) + sysProcInfoPtr->NextEntryOffset);
    }

    */
}

// Перезагрузка информации о процессах.
void TableModelProcesses::reloadProcessesFromSystem()
{
    beginResetModel();  // начать сброс модели.
    readProcessesFromSystem();  // перезагрузка информации о процессах.
    endResetModel();  // завершить сброс модель.
}

// Получения индекса строки процесса по его идентификатору.
int TableModelProcesses::getIndexOfProcessWithId(int id) const
{
    // Указатель на начало списка процессов.
    PSYSTEM_PROCESS_INFORMATION sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)sysInfoBuffPtr;

    int idx = 0;  // счетчик процессов.

    while (sysProcInfoPtr)
    {
        // Если процесс найден, то возвращаем его индекс.
        if ((int)sysProcInfoPtr->UniqueProcessId == id)
            return idx;

        // Если нет следующего элемента списка процессов, то прерываем цикл.
        if (!sysProcInfoPtr->NextEntryOffset)
            break;

        // Вычисление указатель на следующий элемент списка процессов.
        sysProcInfoPtr = (PSYSTEM_PROCESS_INFORMATION)(((LPBYTE)sysProcInfoPtr) + sysProcInfoPtr->NextEntryOffset);

        ++idx;
    }
    return -1;
}
