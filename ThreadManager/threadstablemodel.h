#ifndef THREADSTABLEMODEL_H
#define THREADSTABLEMODEL_H

#include <QAbstractTableModel>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>
#include <QtCharts>
#include <QtCharts/QBarSet>

#define WINDOWS_TICK 10000000.0

class ThreadsTableModel;
typedef void (ThreadsTableModel::*PointerToThreadHandler)(PTHREADENTRY32, THREAD_STATE, FILETIME, FILETIME);

// Структура для представления данных о потоке:
struct ThreadInfo
{
    THREADENTRY32 thread;
    THREAD_STATE state;
    double kernelTime;
    double userTime;
};

// Сигнатура функции ядра NtQuerySystemInformation:
typedef NTSTATUS (WINAPI* t_NtQueryInfo)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

const NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;

// Модель данных для таблицы с потоками:
class ThreadsTableModel : public QAbstractTableModel
{

    // Q_OBJECT - этот макрос обязателен для любого класса на Си++,
    // в котором планируется описать сигналы и/или слоты:
    Q_OBJECT

public:

    // Конструктор:
    explicit ThreadsTableModel(QObject *parent = nullptr);

    // Заголовки строк:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Количество строк:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Колиество столбцов:
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // Данные для строк:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Флаги для строки с индексом index:
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Количество потоков:
    int getThreadsCount() const;

    // Обновить данные о потоках:
    void refreshData(void);

    // Получить время ЦП проведенное в режиме ядра:
    double getKernelTimeByTid(DWORD tid);

    // Получить время ЦП проведенное в режиме пользователя:
    double getUserTimeByTid(DWORD tid);

    // Получить серии даннных о времени ЦП для заданного процесса:
    QtCharts::QStackedBarSeries* getStackedBarSeriesForPid(DWORD pid);

    // Очистить серии данных:
    void clearStackedBarSeries();

    // Получить метки в виде ИД потоков для отображения по оси X:
    QStringList* getCategoriesForPid(DWORD pid);

    // Получить максимальное время ЦП для заданного процесса:
    qreal getMaxCpuTimeForPid(DWORD pid);

private:

    // Список потоков:
    QList<ThreadInfo> tlist;

    // Обход списка потоков и обработка каждого потока функцией threadHandler:
    int loopThroughThreads(PointerToThreadHandler threadHandler);

    // Добавить поток в отображаемый список потоков:
    void addThreadToList(PTHREADENTRY32 p, THREAD_STATE s, FILETIME kernel_t, FILETIME user_t);

    // Указатель на функцию NtQuerySystemInformation:
    t_NtQueryInfo mf_NtQueryInfo = nullptr;

    // Серии даннных о времени ЦП для заданного процесса:
    QMap<DWORD, QtCharts::QStackedBarSeries*> stackedBarSeriesMap;

    // Метки в виде ИД потоков для отображения по оси X:
    QMap<DWORD, QStringList*> categories;

    // Столбцы диаграммы:

    QMap<DWORD, QtCharts::QBarSet*> kernelTimeMapPidBarSet;

    QMap<DWORD, QtCharts::QBarSet*> userTimeMapPidBarSet;

    QtCharts::QBarSet* getKernelTimeBarSetForPid(DWORD pid);

    QtCharts::QBarSet* getUserTimeBarSetForPid(DWORD pid);

};

#endif // THREADSTABLEMODEL_H
