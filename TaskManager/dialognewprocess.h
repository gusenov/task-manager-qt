#ifndef DIALOGNEWPROCESS_H
#define DIALOGNEWPROCESS_H

#include <QDialog>

namespace Ui {
class DialogNewProcess;
}

// Окно "Создать новый процесс".
class DialogNewProcess : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNewProcess(QWidget *parent = 0);  // конструктор.
    ~DialogNewProcess();  // деструктор.

    QString fileName;  // путь к выбранному исполняемому файлу.

private slots:
    void on_pushButtonBrowse_clicked();  // обработчик нажатия на кнопку "Обзор".

    void on_buttonBoxOkCancel_accepted();  // обработчик нажатия на кнопку "ОК".

private:
    Ui::DialogNewProcess *ui;  // указатель на пользовательский интерфейс.
};

#endif // DIALOGNEWPROCESS_H
