#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <windows.h>
#include <tchar.h>

static DWORD WINAPI threadFunc(LPVOID param);

class Updater : public QObject
{

    // Q_OBJECT - этот макрос обязателен для любого класса на Си++,
    // в котором планируется описать сигналы и/или слоты:
    Q_OBJECT

    friend DWORD WINAPI threadFunc(LPVOID);

public:
    Updater();
    ~Updater();

    // Процедура запуска отдельного потока циклического обновления списка запущенных процессов:
    BOOL startUpdater();

private:

    // Процедура-поток, обновляющая в цикле с задержкой  список запущенных процессов:
    DWORD WINAPI updateThread();

signals:
    void tick(void);
};

#endif // UPDATER_H
