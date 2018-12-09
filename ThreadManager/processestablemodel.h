#ifndef PROCESSESTABLEMODEL_H
#define PROCESSESTABLEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QtCharts/QLineSeries>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QDateTime>

#define MAX_HISTORY_ITEMS 100

class ProcessesTableModel;
typedef void (ProcessesTableModel::*PointerToProcessHandler)(PPROCESSENTRY32, SIZE_T, DWORD);

// Структура для представления данных о процессе:
struct TaskInfo
{
    PROCESSENTRY32 proc;
    SIZE_T mem;
    DWORD priority;
};

// Модель данных для таблицы с процессами:
class ProcessesTableModel : public QAbstractTableModel
{

    // Q_OBJECT - этот макрос обязателен для любого класса на Си++,
    // в котором планируется описать сигналы и/или слоты:
    Q_OBJECT

public:

    // Конструктор:
    explicit ProcessesTableModel(QObject *parent = nullptr);

    // Заголовки столбцов:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Количество строк:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Колиество столбцов:
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // Данные для строк:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Флаги для строки с индексом index:
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Количество процессов:
    int getProcessesCount() const;

    // Получить ИД процесса по номеру строки:
    DWORD getPidByRowIndex(int rowIndex) const;

    // Получить родительский ИД прцесса по номеру строки:
    DWORD getParentPidByRowIndex(int rowIndex) const;

    // Обновить данные о процессах:
    void refreshData(void);

    // Получить итератор серий данных о потреблении памяти процессами:
    QMapIterator<DWORD, QtCharts::QLineSeries*> getMemSeriesIterator();

    // Получить серии данных о потреблении памяти заданным процессом:
    QtCharts::QLineSeries* getMemSeriesForPid(DWORD pid);

    // Получить максимальное потребление памяти заданным процессов:
    qreal getMaxMemForPid(DWORD pid);

    // Получить серии данных о количестве потоков у заданного процесса:
    QtCharts::QLineSeries* getThreadCountSeriesForPid(DWORD pid);

    // Получить максимальное количество потоков у заданного процесса:
    qreal getMaxThreadCountForPid(DWORD pid);

    // Получить точку отсчета:
    QDateTime getMinTimeForPid(DWORD pid);

private:

    // Список процессов:
    QList<TaskInfo> tlist;

    // Обход списка процессов и обработка каждого процесса функцией processHandler:
    int loopThroughProcesses(PointerToProcessHandler processHandler);

    // Добавить процесс в отображаемый список процессов:
    void addTaskToList(PPROCESSENTRY32 p, SIZE_T mem, DWORD pri);

    // Серии данных о потреблении памяти:
    QMap<DWORD, QtCharts::QLineSeries*> memSeries;

    // Серии данных о количестве потоков:
    QMap<DWORD, QtCharts::QLineSeries*> threadCountSeries;

};

#endif // PROCESSESTABLEMODEL_H
