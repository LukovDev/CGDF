//
// info.h - Определяет функции для получения информации о CPU и RAM.
//

#pragma once


// Подключаем:
#include "std.h"


// Перечисляем архитектуры:
typedef enum {
    INFO_X86_64,
    INFO_X86,
    INFO_ARM64,
    INFO_ARM,
    INFO_UNKNOWN
} Info_cpu_arch;


// Объявление структур:
typedef struct CpuInfo CpuInfo;  // Информация о процессоре.
typedef struct MemInfo MemInfo;  // Информация о памяти.


// Информация о процессоре:
struct CpuInfo {
    char model[64];      // Модель процессора.
    int threads;         // Количество логических ядер (потоков).
    Info_cpu_arch arch;  // Архитектура процессора.
};


// Информация о памяти:
struct MemInfo {
    size_t total;  // Всего памяти в байтах.
    size_t free;   // Свободно памяти в байтах.
    size_t used;   // Использовано памяти в байтах.
};


// Глобальные переменные:
extern bool _Info_cpu_cached_;
extern CpuInfo _Info_cpu_info_cache_;


// Получить архитектуру процессора в виде строки:
const char* Info_get_cpu_arch_name(Info_cpu_arch arch);

// Функция для получения информации о процессоре:
CpuInfo Info_get_cpu();

// Функция для получения ОЗУ (в байтах):
MemInfo Info_get_mem();
