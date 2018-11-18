#include "dialogabout.h"
#include "ui_dialogabout.h"

// Конструктор.
DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);  // настроить пользовательский интерфейс.
}

// Деструктор.
DialogAbout::~DialogAbout()
{
    delete ui;  // удалить из памяти пользовательский интерфейс.
}

// Нажатие на кнопку "ОК".
void DialogAbout::on_pushButtonOK_clicked()
{
    DialogAbout::close();  // закрыть окно.
}
