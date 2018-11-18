#ifndef DIALOGABOUT_H
#define DIALOGABOUT_H

#include <QDialog>

namespace Ui {
class DialogAbout;
}

// Окно "О программе-диспетчере для ОС Windows".
class DialogAbout : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAbout(QWidget *parent = 0);  // конструктор.
    ~DialogAbout();  // деструктор.

private slots:
    void on_pushButtonOK_clicked();  // обработчик нажатия на кнопку "ОК".

private:
    Ui::DialogAbout *ui;  // указатель на пользовательский интерфейс.
};

#endif // DIALOGABOUT_H
