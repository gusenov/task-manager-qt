#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include "dialognewprocess.h"
#include "dialogabout.h"
#include "tablemodelprocesses.h"
#include "systemfunctions.h"

namespace Ui {
class MainWindow;
}

// Главное окно программы.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);  // конструктор.
    ~MainWindow();  // деструктор.

private slots:
    // Нажатие на пункт меню для создания нового процесса.
    void on_actionFileNewProcess_triggered();

    // Нажатие на пункт меню для выхода из программы.
    void on_actionFileClose_triggered();

    // Нажатие на кнопку для завершения процесса.
    void on_pushButtonKillProcess_clicked();

    // Нажатие на пункт меню для обновления данных показывающихся в программе.
    void on_action_ViewRefresh_triggered();

    // Обработчик смены выбора процесса в таблице процессов.
    void processSelectionChanged(const QItemSelection &selection);

    // Смена выбора вкладки.
    void tabSelected();

    // Нажатие на кнопку выделения виртуальной памяти.
    void on_pushButtonAllocMemory_clicked();

    // Нажатие на кнопку освобождения виртуальной памяти.
    void on_pushButtonFreeMemory_clicked();

    // Нажатие на кнопку записи в виртуальную память.
    void on_pushButtonWriteToMemory_clicked();

    // Нажатие на кнопку чтения из виртуальной памяти.
    void on_pushButtonReadFromMemory_clicked();

    // Нажатие на пункт меню для того чтобы окно программы было всегда поверх других окон.
    void on_actionAlwaysOnTop_triggered();

    // Нажатие на пункт меню "О программе".
    void on_actionHelpAbout_triggered();

private:
    Ui::MainWindow *ui;  // указатель на пользовательский интерфейс.

    // Окно для создания нового процесса.
    DialogNewProcess *newProcessDialog;

    // Окно "О программе".
    DialogAbout *aboutDialog;

    // Модель данных таблицы с процессами.
    TableModelProcesses *processesTableModel;

    // Прокси-модель данных таблицы с процессами.
    QSortFilterProxyModel *processesProxyModel;

    // Статусная строка.
    QHBoxLayout *statusBarLayout;
    QWidget *statusBarContainer;

    // Текстовая метка показывающая количество процессов в статусной строке.
    QLabel *processesCount;

    // Идентификатор выбранного процесса.
    int selectedProcessId = 0;

    // Дескриптор выбранного процесса.
    HANDLE selectedProcessHandle = 0;

    // Базовый адрес выделенного региона страниц в виртуальной памяти.
    PVOID baseAddrOfAllocRegionOfPages = 0;

    // Фактический размер выделенного региона страниц в виртуальной памяти.
    SIZE_T actualSzInBytesOfAllocRegionOfPages = 0;

    // Количество процессов.
    QString getProcessesCount();

    // Метод для получения идентификатора выделенного процесса.
    int getSelectedProcessId();

    // Обновление данных на вкладке "Быстродействие".
    void updatePerformanceTabData();

    // Функции для включения/выключения компонентов:

    // Выключить компоненты при старте приложения.
    void disableWhenStartUp();

    // Выключить компоненты при выделении виртуальной памяти.
    void disableWhenMemoryAllocated();

    // Включить компоненты после выделения виртуальной памяти.
    void enableWhenMemoryAllocated();

    // Выключить компоненты при освобождении виртуальной памяти.
    void disableWhenMemoryFree();

    // Включить компоненты после освобождения виртуальной памяти.
    void enableWhenMemoryFree();
};

#endif // MAINWINDOW_H
