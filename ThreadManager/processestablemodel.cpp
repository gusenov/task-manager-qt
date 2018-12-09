#include "processestablemodel.h"
#include <QDebug>
#include <QFont>
#include <tchar.h>

char const* getTextOfPriority(DWORD p);

// Конструктор:
ProcessesTableModel::ProcessesTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    // Обход списка процессов
    // и обработка каждого процесса функцией addTaskToList:
    loopThroughProcesses(&ProcessesTableModel::addTaskToList);
}

// Заголовки столбцов:
QVariant ProcessesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                    return tr("PPID");
                case 1:
                    return tr("ИД процесса\n(PID)");
                case 2:
                    return tr("Имя образа");
                case 3:
                    return tr("Счётчик\nпотоков");
                case 4:
                    return tr("Базовый\nприоритет");
                case 5:
                    return tr("Память\n(КБ)");
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
int ProcessesTableModel::rowCount(const QModelIndex &parent) const
{
    // Неиспользуемые параметры:
    Q_UNUSED(parent);

    return getProcessesCount();
}

// Колиество столбцов:
int ProcessesTableModel::columnCount(const QModelIndex &parent) const
{
    // Неиспользуемые параметры:
    Q_UNUSED(parent);

    return 6;
}

// Данные для строк:
QVariant ProcessesTableModel::data(const QModelIndex &index, int role) const
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
        case 0:  // PPID
            return QVariant((int)tlist[i].proc.th32ParentProcessID);
        case 1:  // ИД процесса (PID)
            return QVariant((int)tlist[i].proc.th32ProcessID);
        case 2:  // Имя образа
            return QString::fromWCharArray(tlist[i].proc.szExeFile, wcslen(tlist[i].proc.szExeFile));
        case 3:  // Счётчик потоков
            return QVariant((int)tlist[i].proc.cntThreads);
            break;
        case 4:  // Базовый приоритет
            TCHAR text[512];
            wsprintf(text, TEXT("%s"), getTextOfPriority(tlist[i].priority));
            return (LPSTR)text;
        case 5:  // Память
            return QVariant((int)tlist[i].mem);
            break;
        default:
            break;
        }
    }

    return QVariant();
}

// Флаги для строки с индексом index:
Qt::ItemFlags ProcessesTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // Строки включена и её можно выделять:
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// Процедура добавления данных о процессе в список:
void ProcessesTableModel::addTaskToList(PPROCESSENTRY32 p, SIZE_T mem, DWORD pri)
{
     TaskInfo ti;

     ti.proc = *p;
     ti.mem = mem;
     ti.priority = pri;

     tlist.append(ti);


     DWORD pid = ti.proc.th32ProcessID;
     QMap<DWORD, QtCharts::QLineSeries*>::const_iterator valueIt = memSeries.find(pid);
     QtCharts::QLineSeries *value;
     if (valueIt == memSeries.end())
     {
         value = new QtCharts::QLineSeries();
         value->setName("Память (КБ)");
         value->setColor(Qt::blue);
         value->setUseOpenGL(false);
         memSeries.insert(pid, value);
     }
     else
     {
         value = valueIt.value();
         if (value->count() > MAX_HISTORY_ITEMS)
         {
             value->removePoints(0, 1);
         }
     }
     QDateTime momentInTime = QDateTime::currentDateTime();
     //qDebug() << "momentInTime =" << momentInTime;
     value->append(momentInTime.toMSecsSinceEpoch(), ti.mem);


     valueIt = threadCountSeries.find(pid);
     if (valueIt == threadCountSeries.end())
     {
         value = new QtCharts::QLineSeries();
         value->setName("Счетчик потоков");
         value->setColor(Qt::magenta);
         value->setUseOpenGL(false);
         threadCountSeries.insert(pid, value);
     }
     else
     {
         value = valueIt.value();
         if (value->count() > MAX_HISTORY_ITEMS)
         {
             value->removePoints(0, 1);
         }
     }
     value->append(momentInTime.toMSecsSinceEpoch(), ti.proc.cntThreads);
}

// Количество процессов:
int ProcessesTableModel::getProcessesCount() const
{
    return tlist.size();
}

// Получить ИД процесса по номеру строки:
DWORD ProcessesTableModel::getPidByRowIndex(int rowIndex) const
{
    return tlist[rowIndex].proc.th32ProcessID;
}

// Получить родительский ИД прцесса по номеру строки:
DWORD ProcessesTableModel::getParentPidByRowIndex(int rowIndex) const
{
    return tlist[rowIndex].proc.th32ParentProcessID;
}

// Обход списка процессов и обработка каждого процесса функцией processHandler:
int ProcessesTableModel::loopThroughProcesses(PointerToProcessHandler processHandler)
{
    // Количество процессов:
    int processesCount = 0;

    // Получаем информацию о процессах с помощью Microsoft Tool Help Library:
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE)
    {
         DWORD errorCode = GetLastError();
         qDebug() << "errorCode = " << errorCode;
         return processesCount;
    }

    PROCESSENTRY32 ProcEntry = {};
    ProcEntry.dwSize = sizeof(PROCESSENTRY32);

     // Извлекаем из полученных данных информацию о первом процессе:
    BOOL bResult = Process32First(hSnapShot, &ProcEntry);
    if (bResult == FALSE)
    {
         DWORD errorCode = GetLastError();
         qDebug() << "errorCode = " << errorCode;
         CloseHandle(hSnapShot);
         return processesCount;
    }

    // Обрабатываем весь список полученных процессов:
    while (bResult == TRUE)
    {
        // Получаем дескриптор процесса:
        HANDLE hProc = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE,
            ProcEntry.th32ProcessID
        );

        if (hProc != NULL)
        {
            PROCESS_MEMORY_COUNTERS mc;
            mc.cb = sizeof(mc);

            // Считаем сколько памяти в КБ занимает:
            SIZE_T sz = 0;
            if (GetProcessMemoryInfo(hProc, &mc, sizeof(mc)) != 0)
            {
                sz = mc.WorkingSetSize / 1024;
            }

            // Базовый приоритет:
            DWORD pri = GetPriorityClass(hProc);

            // Передаем информацию о процессе в пользовательскую функцию processHandler:
            if (processHandler)
            {
                (this->*processHandler)(&ProcEntry, sz, pri);
            }

            CloseHandle(hProc);
        }

        bResult = Process32Next(hSnapShot, &ProcEntry);
        ++processesCount;
    }

    CloseHandle(hSnapShot);
    return processesCount;
}

// Список названий приоритетов процессов:
char const* TEXT_PRIORITY[7] =
{
    "Выше среднего",     // 0
    "Ниже среднего",     // 1
    "Средний",           // 2
    "Высокий",           // 3
    "Низкий",            // 4
    "Реального времени", // 5
    "Не определен"       // 6
};

// Массив значений классов приоритетов процесса:
DWORD PRIOR_LIST[6] =
{
    ABOVE_NORMAL_PRIORITY_CLASS,
    BELOW_NORMAL_PRIORITY_CLASS,
    NORMAL_PRIORITY_CLASS,
    HIGH_PRIORITY_CLASS,
    IDLE_PRIORITY_CLASS,
    REALTIME_PRIORITY_CLASS
};

// Получить название приоритета процесса по значению:
char const* getTextOfPriority(DWORD p)
{
    char const* txt;

    switch (p)
    {
    case ABOVE_NORMAL_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[0];
        break;

    case BELOW_NORMAL_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[1];
        break;

    case NORMAL_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[2];
        break;

    case HIGH_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[3];
        break;

    case IDLE_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[4];
        break;

    case REALTIME_PRIORITY_CLASS:
        txt = TEXT_PRIORITY[5];
        break;

    default:
        txt = TEXT_PRIORITY[6];
        break;
    }

    return txt;
}

// Обновить данные о процессах:
void ProcessesTableModel::refreshData(void)
{
    beginResetModel();
    tlist.clear();
    loopThroughProcesses(&ProcessesTableModel::addTaskToList);
    endResetModel();
}

// Получить итератор серий данных о потреблении памяти процессами:
QMapIterator<DWORD, QtCharts::QLineSeries*> ProcessesTableModel::getMemSeriesIterator()
{
    return QMapIterator<DWORD, QtCharts::QLineSeries*>(memSeries);
}

// Получить серии данных о потреблении памяти заданным процессом:
QtCharts::QLineSeries* ProcessesTableModel::getMemSeriesForPid(DWORD pid)
{
    QMap<DWORD, QtCharts::QLineSeries*>::const_iterator valueIt = memSeries.find(pid);
    return valueIt == memSeries.end() ? nullptr : valueIt.value();
}

// Получить серии данных о количестве потоков у заданного процесса:
QtCharts::QLineSeries* ProcessesTableModel::getThreadCountSeriesForPid(DWORD pid)
{
    QMap<DWORD, QtCharts::QLineSeries*>::const_iterator valueIt = threadCountSeries.find(pid);
    return valueIt == threadCountSeries.end() ? nullptr : valueIt.value();
}

// Получить максимальное потребление памяти заданным процессов:
qreal ProcessesTableModel::getMaxMemForPid(DWORD pid)
{
    QtCharts::QLineSeries *series = getMemSeriesForPid(pid);

    qreal max = 0;
    foreach (QPointF point, series->pointsVector())
    {
        if (point.y() > max)
            max = point.y();
    }

    return max;
}

// Получить максимальное количество потоков у заданного процесса:
qreal ProcessesTableModel::getMaxThreadCountForPid(DWORD pid)
{
    QtCharts::QLineSeries *series = getThreadCountSeriesForPid(pid);

    qreal max = 0;
    foreach (QPointF point, series->pointsVector())
    {
        if (point.y() > max)
            max = point.y();
    }

    return max;
}

// Получить точку отсчета:
QDateTime ProcessesTableModel::getMinTimeForPid(DWORD pid)
{
    QtCharts::QLineSeries *series = getMemSeriesForPid(pid);
    if (series->count() > 0)
    {
        return QDateTime::fromMSecsSinceEpoch(series->at(0).x());
    }
    else
    {
        return QDateTime::currentDateTime();
    }
}
