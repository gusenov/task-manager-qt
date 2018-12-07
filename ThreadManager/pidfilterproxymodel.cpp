#include "pidfilterproxymodel.h"

// Метод для установки в качестве фильтра регулярного выражения,
// по которому будут фильтроваться строки на основе поля ИД процесса:
void PidFilterProxyModel::setPidFilter(const QString& regExp)
{
    pidRegExp.setPattern(regExp);
}
