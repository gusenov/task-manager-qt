#include "mainwindow.h"
#include <QApplication>

// Точка входа в программу.
int main(int argc, char *argv[])
{
    // Создание экземпляра приложения.
    QApplication a(argc, argv);

    // Создание главного окна.
    MainWindow w;
    w.show();

    return a.exec();
}
