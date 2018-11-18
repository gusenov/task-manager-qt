#ifndef TABLEMODELPROCESSES_H
#define TABLEMODELPROCESSES_H

#include <QAbstractTableModel>
#include <winternl.h>

// Модель данных для таблицы с процессами.
class TableModelProcesses : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TableModelProcesses(QObject *parent = nullptr);  // конструктор.
    ~TableModelProcesses();  // деструктор.

    // Заголовки для столбцов.
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Количество строк.
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Количество столбцов.
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    // Данные для строки с индексом index.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Флаги для строки с индексом index.
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Обновление модели данных.
    void reloadProcessesFromSystem();

    // Количество процессов.
    int getProcessesCount() const;

    // Получение информации о процессе по индексу строки в которой он отображается.
    PSYSTEM_PROCESS_INFORMATION getProcessAtIndex(int index) const;

    // Получение индекса строки процесса по его идентификатору.
    int getIndexOfProcessWithId(int id) const;

private:
    HANDLE heapHandle = 0;  // дескриптор кучи процесса.
    PVOID sysInfoBuffPtr = 0;  // буфер для хранения информации полученной через функцию NtQuerySystemInformation.

    void readProcessesFromSystem();  // метод для получения информации о процессах через NtQuerySystemInformation.
};

#endif // TABLEMODELPROCESSES_H
