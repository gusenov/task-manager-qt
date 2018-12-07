#ifndef PIDFILTERPROXYMODEL_H
#define PIDFILTERPROXYMODEL_H


#include <QApplication>
#include <QtWidgets>
#include <QDebug>

// Фильтр для модели данных потоков, чтобы показывать потоки
// только для выбранного процесса:
class PidFilterProxyModel : public QSortFilterProxyModel
{
    // Q_OBJECT - этот макрос обязателен для любого класса на Си++,
    // в котором планируется описать сигналы и/или слоты:
    Q_OBJECT

public:

    // Конструктор:
    explicit PidFilterProxyModel(QObject* parent = nullptr):
        QSortFilterProxyModel(parent)  // вызов родительского конструктора.
    {
        // Настройка регулярного выражения:
        pidRegExp.setCaseSensitivity(Qt::CaseInsensitive);  // не чувствительно к регистру символов.
        pidRegExp.setPatternSyntax(QRegExp::FixedString);  // использовать как фиксированную строку.
    }

    // Перезапись родительского метода, который фильтрует строки:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        // Индекс строки:
        QModelIndex rowIndex = sourceModel()->index(sourceRow, 0, sourceParent);

        // ИД процесса:
        QString pid = sourceModel()->data(rowIndex).toString();

        // Если ИД процесса не задан, то показываем строку,
        // иначе проверяем удовлетворяет ли ИД процесса регулярному выражению:
        return pid.isEmpty() ? true : pid.contains(pidRegExp);
    }

    // Метод для установки в качестве фильтра регулярного выражения,
    // по которому будут фильтроваться строки на основе поля ИД процесса:
    void setPidFilter(const QString& regExp);

private:

    // Регулярное выражение для фильтрации строк:
    QRegExp pidRegExp;

};

#endif // PIDFILTERPROXYMODEL_H
