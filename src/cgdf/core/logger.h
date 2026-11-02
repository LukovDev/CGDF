//
// logger.h - Логгер событий.
//

#pragma once


// Подключаем:
#include "std.h"


// Инициализация логгера:
void Logger_init(void);

// Включить логгер:
void Logger_enable(void);

// Выключить логгер:
void Logger_disable(void);

// Вывод сообщения в лог-файл и в консоль:
PRINTF_FORMAT(1, 2) void log_msg(const char *fmt, ...);
