#ifndef PROCESSESTABLEMODEL_H
#define PROCESSESTABLEMODEL_H

#include <QAbstractTableModel>
#include <windows.h>
#include <TlHelp32.h>
#include <psapi.h>

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
    int getPidByRowIndex(int rowIndex) const;

    // Получить родительский ИД прцесса по номеру строки:
    int getParentPidByRowIndex(int rowIndex) const;

    // Обновить данные о процессах:
    void refreshData(void);

private:

    // Список процессов:
    QList<TaskInfo> tlist;

    // Обход списка процессов и обработка каждого процесса функцией processHandler:
    int loopThroughProcesses(PointerToProcessHandler processHandler);

    // Добавить процесс в отображаемый список процессов:
    void addTaskToList(PPROCESSENTRY32 p, SIZE_T mem, DWORD pri);

};

#endif // PROCESSESTABLEMODEL_H
