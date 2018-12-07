#include "mainwindow.h"
#include <QApplication>

// Самая главная функция приложения,
// с которой начинается выполнение:
int main(int argc, char *argv[])
{
    // Создание приложения:
    QApplication a(argc, argv);

    // Создание главного окна:
    MainWindow w;

    // Отображение главного окна:
    w.show();

    return a.exec();
}
