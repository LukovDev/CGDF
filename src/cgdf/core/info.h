//
// info.h - Реализует функции для получения информации о CPU и RAM.
//

#pragma once


// Подключаем:
#include "std.h"

#ifdef _WIN32
    #include <windows.h>
    #include <intrin.h>
#else
    // Эти заголовки нужны для GCC/Clang/Linux/MacOS:
    #include <unistd.h>
    #if defined(__GNUC__) || defined(__clang__)
        #include <cpuid.h>
    #endif
    #ifdef __linux__
        #include <sys/sysinfo.h>
    #endif
#endif


// Объявление структур:
typedef struct CpuInfo CpuInfo;  // Информация о процессоре.
typedef struct MemInfo MemInfo;  // Информация о памяти.


// Информация о процессоре:
struct CpuInfo {
    char model[64];  // Модель процессора.
    int threads;     // Количество логических ядер (потоков).
};


// Информация о памяти:
struct MemInfo {
    size_t total;  // Всего памяти в байтах.
    size_t free;   // Свободно памяти в байтах.
    size_t used;   // Использовано памяти в байтах.
};


// Функция для получения количества логических ядер процессора (-1 = ошибка):
static inline CpuInfo Info_get_cpu() {
    CpuInfo info = {.threads = 1};

    // Получаем модель процессора:
    uint32_t regs[4];
    char name[49];  // 3 блока по 16 байт + null-терминатор.
    memset(name, 0, sizeof(name));

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t step = 0x80000002 + i;
        #ifdef _WIN32  // Для MSVC (Windows):
            __cpuid((int*)regs, step);
        #elif defined(__GNUC__) || defined(__clang__)  // Для GCC/Clang (Linux и MinGW):
            __get_cpuid(step, &regs[0], &regs[1], &regs[2], &regs[3]);
        #else  // Не удалось получить модель процессора:
            strncpy(info.model, "<Unknown CPU>", 48); break;
        #endif
        memcpy(name + i * 16, regs, 16);
    }

    // Убираем пробелы в начале строки:
    char *p = name;
    while (*p == ' ') p++;
    strncpy(info.model, p, sizeof(info.model) - 1);
    info.model[sizeof(info.model) - 1] = '\0';

    // Убираем пробелы в конце строки:
    for (int i = (int)strlen(info.model) - 1; i >= 0; i--) {
        if (info.model[i] == ' ') info.model[i] = '\0';
        else break;
    }

    // Получаем количество потоков:
    #ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        info.threads = sysinfo.dwNumberOfProcessors;
    #elif __linux__
        info.threads = sysconf(_SC_NPROCESSORS_ONLN);
    #else
        info.threads = -1;  // Не удалось получить.
    #endif
    return info;
}


// Функция для получения ОЗУ (в байтах):
static inline MemInfo Info_get_mem() {
    MemInfo info = {.total = 0, .free = 0, .used = 0};
    #ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        info.total = status.ullTotalPhys;
        info.free = status.ullAvailPhys;
    #elif __linux__
        struct sysinfo si;
        sysinfo(&si);
        info.total = si.totalram * si.mem_unit;
        info.free = si.freeram * si.mem_unit;
    #endif
    info.used = info.total - info.free;
    return info;
}
