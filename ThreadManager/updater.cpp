#include "updater.h"

Updater::Updater()
{

}

Updater::~Updater()
{

}

// Процедура-поток, обновляющая в цикле с задержкой  список запущенных процессов:
DWORD WINAPI Updater::updateThread()
{
    DWORD TimeOut = 1000;  // задержка между запросам списка процессов.

    BOOL bUpdaterRun;
    bUpdaterRun = TRUE;
    while (bUpdaterRun)
    {

        emit tick();  // формирование списка процессов.

        Sleep(TimeOut);
    }  // end while
}

static DWORD WINAPI threadFunc(LPVOID param)
{
   Updater* u = (Updater*)param;
   u->updateThread();
}

// Процедура запуска отдельного потока циклического обновления списка запущенных процессов:
BOOL Updater::startUpdater()
{
    DWORD dwThreadId;

    HANDLE hThread = CreateThread(
        nullptr,       // no security attribute
        0,             // default stack size
        threadFunc,    // thread proc
        (LPVOID)this,  // thread parameter
        0,             // not suspended
        &dwThreadId    // returns thread ID
    );

    if (hThread == nullptr)
    {
        // Поток  не запущен:
        MessageBox(nullptr, _T("CreateThread failed."), _T("Ошибка"), MB_OK);
        return FALSE;
    }
    else
    {
        CloseHandle(hThread);
        return TRUE;
    }
}
