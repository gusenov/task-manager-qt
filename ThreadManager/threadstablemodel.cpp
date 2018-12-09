#include "threadstablemodel.h"
#include <QDebug>
#include <QFont>

char const* getTextOfState(THREAD_STATE s);

// Конструктор:
ThreadsTableModel::ThreadsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    // Указатель на функцию NtQuerySystemInformation:
    mf_NtQueryInfo = (t_NtQueryInfo)GetProcAddress(GetModuleHandleA("NtDll.dll"), "NtQuerySystemInformation");

    loopThroughThreads(&ThreadsTableModel::addThreadToList);
}

// Заголовки строк:
QVariant ThreadsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //qDebug() << "section = " << section << endl;

    // Параметр role используется для того чтобы узнать какое свойство запрашивается.
    // Роль Qt::DisplayRole означает текст.
    // Роль Qt::FontRole означает шрифт.
    if (role == Qt::DisplayRole)
    {

        // Ориентация Qt::Horizontal означает заголовок, который находится над столбцом сверху.
        // Ориентация Qt::Vertical означает заголовок, который находится слева от строки.
        if (orientation == Qt::Horizontal)
        {

            // По номеру столбца возвращаем текстовый заголовок для него:
            switch (section)
            {
                case 0:
                    return tr("ИД\nпроцесса\n(PID)");
                case 1:
                    return tr("ИД\nпотока\n(TID)");
                case 2:
                    return tr("Базовый\nприоритет\n(от 0 до 31)");
                case 3:
                    return tr("Состояние");
                case 4:
                    return tr("Время\nядра\nЦП (с)");
                case 5:
                    return tr("Время\nпользователя\nЦП (с)");
                default:
                    break;
            }

        }

    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        font.setStyle(QFont::StyleItalic);
        return font;
    }

    return QVariant();
}

// Количество строк:
int ThreadsTableModel::rowCount(const QModelIndex &parent) const
{
    // Неиспользуемые параметры:
    Q_UNUSED(parent);

    return getThreadsCount();
}

// Колиество столбцов:
int ThreadsTableModel::columnCount(const QModelIndex &parent) const
{
    // Неиспользуемые параметры:
    Q_UNUSED(parent);

    return 6;
}

// Данные для строк:
QVariant ThreadsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        int i = index.row();

        switch (index.column())
        {
        case 0:  // ИД процесса (PID)
            return QVariant((int)tlist[i].thread.th32OwnerProcessID);
        case 1:  // ИД потока (TID)
            return QVariant((int)tlist[i].thread.th32ThreadID);
        case 2:  // Базовый приоритет (от 0 до 31)
            return QVariant((int)tlist[i].thread.tpBasePri);
        case 3:  // Состояние
            TCHAR text[512];
            wsprintf(text, TEXT("%s"), getTextOfState(tlist[i].state));
            return (LPSTR)text;
        case 4:  // Время ядра ЦП
            return QVariant((double)tlist[i].kernelTime);
        case 5:  // Время пользователя ЦП
            return QVariant((double)tlist[i].userTime);
        default:
            break;
        }
    }

    return QVariant();
}

// Флаги для строки с индексом index:
Qt::ItemFlags ThreadsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // Строки включена и её можно выделять:
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// Процедура добавления данных о потоке в список:
void ThreadsTableModel::addThreadToList(PTHREADENTRY32 p, THREAD_STATE s, FILETIME kernel_t, FILETIME user_t)
{
    ThreadInfo ti;
    ti.thread = *p;
    ti.state = s;

    ti.kernelTime = (static_cast<__int64>(kernel_t.dwHighDateTime) << 32 | kernel_t.dwLowDateTime) / WINDOWS_TICK;
    ti.userTime = (static_cast<__int64>(kernel_t.dwHighDateTime) << 32 | user_t.dwLowDateTime) / WINDOWS_TICK;

    tlist.append(ti);

    // ИД процесса:
    DWORD pid = ti.thread.th32OwnerProcessID;

    // Получаем QBarSet для процесса:
    QtCharts::QBarSet* kernelTimeBarSet = getKernelTimeBarSetForPid(pid);

    // Добавляем в QBarSet значение kernel time для очередного потока:
    *kernelTimeBarSet << ti.kernelTime;

    QtCharts::QBarSet* userTimeBarSet = getUserTimeBarSetForPid(pid);
    *userTimeBarSet << ti.userTime;

    // Записываем ИД потока в список категорий:
    QStringList* category = getCategoriesForPid(pid);
    *category << QString::number(ti.thread.th32ThreadID);

    QStackedBarSeries* stackedBarSeries = getStackedBarSeriesForPid(pid);
    if (!stackedBarSeries->barSets().contains(kernelTimeBarSet)) {
        stackedBarSeries->append(kernelTimeBarSet);
    }
    if (!stackedBarSeries->barSets().contains(userTimeBarSet)) {
        stackedBarSeries->append(userTimeBarSet);
    }

}

// Количество потоков:
int ThreadsTableModel::getThreadsCount() const
{
    return tlist.size();
}

// Обход списка потоков и обработка каждого потока функцией threadHandler:
int ThreadsTableModel::loopThroughThreads(PointerToThreadHandler threadHandler)
{
    // Количество потоков:
    int threadsCount = 0;


    BYTE* mp_Data = 0;
    DWORD mu32_DataSize = 0;
    ULONG u32_Needed = 0;
    NTSTATUS s32_Status;

    // Пытаемся получить до тех пор пока не получится
    // данные о процессах через NtQuerySystemInformation:
    do
    {
        // Освобождаем память:
        if (mp_Data)
        {
            LocalFree(mp_Data);
            mp_Data = 0;
        }

        // Вызов функции NtQuerySystemInformation:
        s32_Status = mf_NtQueryInfo(SystemProcessInformation, mp_Data, mu32_DataSize, &u32_Needed);

        // Если памяти оказалось недостаточно:
        if (s32_Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            // Смотрим сколько памяти нужно:
            mu32_DataSize = u32_Needed;
            if (!mu32_DataSize)
            {
                qDebug() << "Размер не получен!";
                break;
            }

            // Выделяем необходимое количество памяти:
            mp_Data = (BYTE*)LocalAlloc(LMEM_FIXED, mu32_DataSize);

            // Снова вызываем функцию NtQuerySystemInformation:
            s32_Status = mf_NtQueryInfo(SystemProcessInformation, mp_Data, mu32_DataSize, &u32_Needed);

            // Если памяти оказалось недостаточно:
            if (s32_Status == STATUS_INFO_LENGTH_MISMATCH)
            {
                // Повторяем итерацию цикла:
                continue;
            }

            // Если какая-то другая ошибка:
            else if (!NT_SUCCESS(s32_Status))
            {
                // Выводим код ошибки:
                qDebug() << QString("NTSTATUS = %1").arg(s32_Status, 0, 16);

                // Освобождаем выделенную память:
                LocalFree(mp_Data);

                break;
            }
            else
            {
                break;
            }
        }

        // Если какая-то другая ошибка:
        else
        {
            // Выводим код ошибки:
            qDebug() << QString("NTSTATUS = %1").arg(s32_Status, 0, 16);
            break;
        }
    }
    while (1);

    if (!NT_SUCCESS(s32_Status))
    {
        return threadsCount;
    }

    // Получаем информацию о процессах с помощью Microsoft Tool Help Library:
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE)
    {
         DWORD errorCode = GetLastError();
         qDebug() << "errorCode = " << errorCode;
         return threadsCount;
    }

    THREADENTRY32 ThreadEntry = {};
    ThreadEntry.dwSize = sizeof(THREADENTRY32);

    // Извлекаем из полученных данных информацию о первом процессе:
    BOOL bResult = Thread32First(hSnapShot, &ThreadEntry);
    if (bResult == FALSE)
    {
         DWORD errorCode = GetLastError();
         qDebug() << "errorCode = " << errorCode;
         CloseHandle(hSnapShot);
         return threadsCount;
    }

    // Обрабатываем весь список полученных процессов:
    while (bResult == TRUE)
    {
        THREAD_STATE threadState = StateUnknown;

        // Находим рассматриваемый на данной итерации процесс:
        PSYSTEM_PROCESS_INFORMATION pk_Proc = 0;
        if (mp_Data)
        {
            pk_Proc = (PSYSTEM_PROCESS_INFORMATION)mp_Data;
            while (pk_Proc)
            {
                if (PtrToUlong(pk_Proc->UniqueProcessId) == ThreadEntry.th32OwnerProcessID)
                    break;
                else if (pk_Proc->NextEntryOffset)
                    pk_Proc = (PSYSTEM_PROCESS_INFORMATION)((BYTE*)pk_Proc + pk_Proc->NextEntryOffset);
                else
                    pk_Proc = 0;
            }

            // Если процесс найти удалось, то находим рассматриваемый на данной итерации поток:
            PSYSTEM_THREADS pk_Thread = 0;
            if (pk_Proc)
            {
                pk_Thread = (PSYSTEM_THREADS)((BYTE*)pk_Proc + sizeof(SYSTEM_PROCESS_INFORMATION));
                for (DWORD i = 0; i < pk_Proc->NumberOfThreads; i++)
                {
                    if (pk_Thread->ClientId.UniqueThread == (HANDLE)(DWORD_PTR)ThreadEntry.th32ThreadID)
                    {
                        threadState = pk_Thread->State;
                        break;
                    }
                    pk_Thread++;
                }
            }
            else
            {
                qDebug() << "Процесс не найден!";
            }
        }
        else
        {
            qDebug() << "Данные о процессах не получены!";
        }

        // Получаем время ЦП:
        HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, ThreadEntry.th32ThreadID);
        FILETIME create_t = {0, 0}, exit_t = {0, 0}, kernel_t = {0, 0}, user_t = {0, 0};
        if (hThread)
        {
            if (!GetThreadTimes(hThread, &create_t, &exit_t, &kernel_t, &user_t))
            {
                qDebug() << "Неудачный запуск GetThreadTimes! Код ошибки =" << GetLastError();
            }
            CloseHandle(hThread);
        }
        else
        {
            //qDebug() << "Не удалось получить дескриптор потока!";
        }

        // Передаём собранную информацию в пользовательскую функцию:
        if (threadHandler)
        {
            (this->*threadHandler)(&ThreadEntry, threadState, kernel_t, user_t);
        }

        bResult = Thread32Next(hSnapShot, &ThreadEntry);
        ++threadsCount;
    }

    CloseHandle(hSnapShot);

    if (mp_Data)
    {
        LocalFree(mp_Data);
    }

    return threadsCount;
}

// Список названий состояний потоков:
char const* TEXT_STATE[8] =
{
    "Инициализирован",       // 0
    "Готов",                 // 1
    "Выполняется",           // 2
    "Простаивает",           // 3
    "Завершен",              // 4
    "Ожидает",               // 5
    "Переходное состояние",  // 6
    "Unknown"                // 7
};

// Получить название состояния потока по значению:
char const* getTextOfState(THREAD_STATE s)
{
    char const* txt = TEXT_STATE[7];

    switch (s)
    {
    case StateInitialized:
        txt = TEXT_STATE[0];
        break;

    case StateReady:
        txt = TEXT_STATE[1];
        break;

    case StateRunning:
        txt = TEXT_STATE[2];
        break;

    case StateStandby:
        txt = TEXT_STATE[3];
        break;

    case StateTerminated:
        txt = TEXT_STATE[4];
        break;

    case StateWait:
        txt = TEXT_STATE[5];
        break;

    case StateTransition:
        txt = TEXT_STATE[6];
        break;

    case StateUnknown:
    default:
        txt = TEXT_STATE[7];
        break;
    }

    return txt;
}

// Обновить данные о потоках:
void ThreadsTableModel::refreshData(void)
{
    beginResetModel();

    tlist.clear();

    clearStackedBarSeries();

    kernelTimeMapPidBarSet.clear();
    userTimeMapPidBarSet.clear();

    // Очищаем все категории:
    QMapIterator<DWORD, QStringList*> categoriesIt(categories);
    while (categoriesIt.hasNext()) {
        categoriesIt.next();
        categoriesIt.value()->clear();
    }

    loopThroughThreads(&ThreadsTableModel::addThreadToList);

    endResetModel();
}

// Получить время ЦП проведенное в режиме ядра:
double ThreadsTableModel::getKernelTimeByTid(DWORD tid)
{
    foreach (ThreadInfo threadInfo, tlist)
    {
        if (threadInfo.thread.th32ThreadID == tid) {
            return threadInfo.kernelTime;
        }
    }
    return 0;
}

// Получить время ЦП проведенное в режиме пользователя:
double ThreadsTableModel::getUserTimeByTid(DWORD tid)
{
    foreach (ThreadInfo threadInfo, tlist)
    {
        if (threadInfo.thread.th32ThreadID == tid) {
            return threadInfo.userTime;
        }
    }
    return 0;
}

// Получить серии даннных о времени ЦП для заданного процесса:
QStackedBarSeries* ThreadsTableModel::getStackedBarSeriesForPid(DWORD pid)
{
    QMap<DWORD, QtCharts::QStackedBarSeries*>::const_iterator stackedBarSeriesIt = stackedBarSeriesMap.find(pid);
    QtCharts::QStackedBarSeries* stackedBarSeries = nullptr;
    if (stackedBarSeriesIt == stackedBarSeriesMap.end())
    {
        stackedBarSeries = new QtCharts::QStackedBarSeries();
        stackedBarSeriesMap.insert(pid, stackedBarSeries);
    }
    else
    {
        stackedBarSeries = stackedBarSeriesIt.value();
    }
    return stackedBarSeries;
}

// Очистить серии данных:
void ThreadsTableModel::clearStackedBarSeries()
{
    QMapIterator<DWORD, QtCharts::QStackedBarSeries*> stackedBarSeriesIt(stackedBarSeriesMap);
    while (stackedBarSeriesIt.hasNext()) {
        stackedBarSeriesIt.next();
        QtCharts::QStackedBarSeries* stackedBarSeries = stackedBarSeriesIt.value();
        stackedBarSeries->clear();
    }
}

// Получить метки в виде ИД потоков для отображения по оси X:
QStringList* ThreadsTableModel::getCategoriesForPid(DWORD pid)
{
    QMap<DWORD, QStringList*>::const_iterator categoriesIt = categories.find(pid);
    QStringList *category;
    if (categoriesIt == categories.end())
    {
        category = new QStringList();
        categories.insert(pid, category);
    }
    else
    {
        category = categoriesIt.value();
    }
    return category;
}

// Получить время ЦП проведенное в режиме ядра:
QtCharts::QBarSet* ThreadsTableModel::getKernelTimeBarSetForPid(DWORD pid)
{
    QMap<DWORD, QtCharts::QBarSet*>::const_iterator it = kernelTimeMapPidBarSet.find(pid);

    QtCharts::QBarSet *barSet;
    if (it == kernelTimeMapPidBarSet.end())
    {
        barSet = new QtCharts::QBarSet("Время ядра ЦП (с)");
        barSet->setColor(Qt::red);
        kernelTimeMapPidBarSet.insert(pid, barSet);
    }
    else
    {
        barSet = it.value();
    }

    return barSet;
}

// Получить время ЦП проведенное в режиме пользователя:
QtCharts::QBarSet* ThreadsTableModel::getUserTimeBarSetForPid(DWORD pid)
{
    QMap<DWORD, QtCharts::QBarSet*>::const_iterator it = userTimeMapPidBarSet.find(pid);

    QtCharts::QBarSet *barSet;
    if (it == userTimeMapPidBarSet.end())
    {
        barSet = new QtCharts::QBarSet("Время пользователя ЦП (с)");
        barSet->setColor(Qt::green);
        userTimeMapPidBarSet.insert(pid, barSet);
    }
    else
    {
        barSet = it.value();
    }

    return barSet;
}

// Получить максимальное время ЦП для заданного процесса:
qreal ThreadsTableModel::getMaxCpuTimeForPid(DWORD pid)
{
    QtCharts::QBarSet* kernelTimeBarSet = getKernelTimeBarSetForPid(pid);
    qreal maxKernelTime = 0;

    QtCharts::QBarSet* userTimeBarSet = getUserTimeBarSetForPid(pid);
    qreal maxUserTime = 0;

    for (int i = 0; i < kernelTimeBarSet->count(); ++i)
    {
        if (kernelTimeBarSet->at(i) > maxKernelTime)
        {
            maxKernelTime = kernelTimeBarSet->at(i);
        }
    }

    for (int i = 0; i < userTimeBarSet->count(); ++i)
    {
        if (userTimeBarSet->at(i) > maxUserTime)
        {
            maxUserTime = userTimeBarSet->at(i);
        }
    }

    qreal sum = maxKernelTime + maxUserTime;

    return sum > 1 ? sum : 1;
}
