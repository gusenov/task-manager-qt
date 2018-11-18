#include "systemfunctions.h"
#include <Tchar.h>
#include <QDebug>

// Конструктор.
SystemFunctions::SystemFunctions()
{
    ntdllHandle = GetModuleHandle(_T("ntdll.dll"));  // дескриптор модуля "ntdll.dll".
    if (ntdllHandle)  // если дескритор есть, то:
    {
        // Получение указателей на функции ядра:

        // Функция для получения информации заданного типа.
        NtQuerySystemInformationFunction = (NtQuerySystemInformationPointer)GetProcAddress(ntdllHandle, "NtQuerySystemInformation");

        // Функция для выделения виртуальной памяти.
        NtAllocateVirtualMemoryFunction = (NtAllocateVirtualMemoryPointer)GetProcAddress(ntdllHandle, "NtAllocateVirtualMemory");

        // Функция для освобождения виртуальной памяти.
        NtFreeVirtualMemoryFunction = (NtFreeVirtualMemoryPointer)GetProcAddress(ntdllHandle, "NtFreeVirtualMemory");

        // Функция для записи в виртуальную память.
        NtWriteVirtualMemoryFunction = (NtWriteVirtualMemoryPointer)GetProcAddress(ntdllHandle, "NtWriteVirtualMemory");

        // Функция для чтения из виртуальной памяти.
        NtReadVirtualMemoryFunction = (NtReadVirtualMemoryPointer)GetProcAddress(ntdllHandle, "NtReadVirtualMemory");
    }
}
