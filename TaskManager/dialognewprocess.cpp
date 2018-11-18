#include "dialognewprocess.h"
#include "ui_dialognewprocess.h"
#include <QFileDialog>
#include <QDebug>

// Конструктор.
DialogNewProcess::DialogNewProcess(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNewProcess)
{
    ui->setupUi(this);  // настроить пользовательский интерфейс.
}

// Деструктор.
DialogNewProcess::~DialogNewProcess()
{
    delete ui;  // удалить из памяти пользовательский интерфейс.
}

// Нажатие на кнопку "Обзор".
void DialogNewProcess::on_pushButtonBrowse_clicked()
{
    // Показать диалоговое окно для выбора исполняемого файла.
    fileName = QFileDialog::getOpenFileName(this, tr("Обзор"), QString(), tr("Программы (*.exe)"));

    // Установить путь к выбранному файлу в качестве текста выпадающего списка.
    ui->comboBoxNameOfProgram->setCurrentText(fileName);

    // Добавить путь к выбранному файлу в качестве возможных значений выпадающего списка.
    ui->comboBoxNameOfProgram->addItem(fileName);
}

// Нажатие на кнопку "ОК".
void DialogNewProcess::on_buttonBoxOkCancel_accepted()
{
    // При нажатии на кнопку "ОК" записать текст из выпадающего списка в качестве к выбранному файлу.
    fileName = ui->comboBoxNameOfProgram->currentText();
}
