#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <windows.h>
#include <winternl.h>
#include <QList>

// Коды ошибок:
const NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;
const NTSTATUS STATUS_CONFLICTING_ADDRESSES = 0xC0000018;

// Системные функции из ntdll.dll:

// Функция для получения заданной параметром типа SYSTEM_INFORMATION_CLASS информации.
typedef NTSTATUS (NTAPI *NtQuerySystemInformationPointer) (
    SYSTEM_INFORMATION_CLASS SystemInformationClass,  // тип получаемой информации.
    PVOID SystemInformation,  // указатель на буфер для приема данных.
    ULONG SystemInformationLength,  // длина буфера для приема данных.
    PULONG ReturnLength);  // ULONG *ReturnLength

// Функция позволяет резервировать и передавать виртуальную память процессу.
typedef NTSTATUS (NTAPI *NtAllocateVirtualMemoryPointer) (
    HANDLE ProcessHandle,  // дескриптор процесса.
    PVOID *BaseAddress,  // указатель на базовый адрес в заданном процессе.
    ULONG_PTR ZeroBits,  // the number of high-order address bits that must be zero in the base address of the section view.
    PSIZE_T RegionSize,  // указатель на переменную в которую запишется фактический размер в байтах выделенного региона страниц.
    ULONG AllocationType,  // MEM_RESERVE - зарезервировать, MEM_COMMIT - передать.
    ULONG Protect);  // битовая маска из флагов для защиты страниц.

// Освобождение виртуальной памяти.
typedef NTSTATUS (NTAPI *NtFreeVirtualMemoryPointer) (
    HANDLE ProcessHandle,  // дескриптор процесса.
    PVOID *BaseAddress,  // указатель на базовый адрес в заданном процессе.
    PSIZE_T RegionSize,  // указатель на переменную в которую запишется фактический размер в байтах освобожденного региона страниц.
    ULONG FreeType);  // битовая маска из флагов MEM_DECOMMIT и MEM_RELEASE.

// Чтение из виртуальной памяти.
typedef LONG (NTAPI *NtReadVirtualMemoryPointer)(
    HANDLE ProcessHandle,  // дескриптор процесса.
    PVOID BaseAddress,  // указатель на базовый адрес в заданном процессе.
    PVOID Buffer,  // буфер для прочитанных данных.
    ULONG NumberOfBytesToRead,  // количество байт для чтения.
    PULONG NumberOfBytesReaded);  // количество прочитанных байт.

// Запись в виртуальную память.
typedef LONG (NTAPI *NtWriteVirtualMemoryPointer)(
    HANDLE ProcessHandle,  // дескриптор процесса.
    PVOID BaseAddress,  // указатель на базовый адрес в заданном процессе.
    PVOID Buffer,  // буфер из которого списываются данные.
    ULONG NumberOfBytesToWrite,  // количество байт для записи.
    PULONG  NumberOfBytesWritten);  // количество записанных байт.

class SystemFunctions
{
public:
    // Реализация паттерна Singleton (синглтон),
    // чтобы всегда был только один экземпляр данного класса:
    SystemFunctions(SystemFunctions const&) = delete;
    void operator=(SystemFunctions const&) = delete;
    static SystemFunctions& getInstance()
    {
        static SystemFunctions instance;
        return instance;
    }

    HMODULE ntdllHandle = 0;  // переменная для хранения дескриптора модуля "ntdll.dll".

    // Указатель на функцию ядра NtQuerySystemInformation.
    NtQuerySystemInformationPointer NtQuerySystemInformationFunction = 0;

    // Указатель на функцию ядра NtAllocateVirtualMemory.
    NtAllocateVirtualMemoryPointer NtAllocateVirtualMemoryFunction = 0;

    // Указатель на функцию ядра NtFreeVirtualMemory.
    NtFreeVirtualMemoryPointer NtFreeVirtualMemoryFunction = 0;

    // Указатель на функцию ядра NtReadVirtualMemory.
    NtReadVirtualMemoryPointer NtReadVirtualMemoryFunction = 0;

    // Указатель на функцию ядра NtWriteVirtualMemory.
    NtWriteVirtualMemoryPointer NtWriteVirtualMemoryFunction = 0;

private:
    SystemFunctions();  // приватный конструктор.
};

#endif // SYSTEMUTILS_H
