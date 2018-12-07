#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QSortFilterProxyModel>
#include "processestablemodel.h"
#include "threadstablemodel.h"
#include "pidfilterproxymodel.h"

namespace Ui {
class MainWindow;
}

// Объявление класса представляющего собой главное окно приложения:
class MainWindow : public QMainWindow
{
    // Q_OBJECT - этот макрос обязателен для любого класса на Си++,
    // в котором планируется описать сигналы и/или слоты:
    Q_OBJECT

public:

    // Конструктор:
    explicit MainWindow(QWidget *parent = 0);

    // Деструктор:
    ~MainWindow();

// Сигналы и слоты используются для коммуникации между объектами:
private slots:

    // Метод, который вызывается при выделении строки в таблице с процессами:
    void processSelectionChangedSlot(const QItemSelection &newSelection,
                                     const QItemSelection &oldSelection);

    // Метод, который вызывается при выделении строки в таблице с потоками:
    void threadSelectionChangedSlot(const QItemSelection &newSelection,
                                     const QItemSelection &oldSelection);

    // Пункт меню Файл -> Завершение диспетчера:
    void on_actionExit_triggered();

    // Пункт меню Вид -> Обновить (F5):
    void on_actionRefresh_triggered();

private:
    Ui::MainWindow *ui;

    // Модель данных для таблицы с процессами:
    ProcessesTableModel *processesTableModel;

    // Модель данных для таблицы с потоками:
    ThreadsTableModel *threadsTableModel;

    // Фильтр для модели данных потоков, чтобы показывать потоки
    // только для выбранного процесса:
    PidFilterProxyModel *threadsProxyModel;

    // Метод для настройки внешнего вида таблиц процессов и потоков:
    void setupTable(QTableView *tableView, QAbstractItemModel *tableModel);

    // Метод для автоматического задания ширины столбцов в таблицах:
    void autosizeTable(QTableView *tableView);

    // Статусная строка:
    QHBoxLayout *statusBarLayout;
    QWidget *statusBarContainer;

    // Текстовая метка показывающая время проведенное процессором в режиме ядра:
    QLabel *cpuKernelTime;

    // Текстовая метка показывающая время проведенное процессором в режиме пользователя:
    QLabel *cpuUserTime;

};

#endif // MAINWINDOW_H
