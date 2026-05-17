//
// logger.h - Логгер событий.
//

#pragma once


// Инициализация логгера:
void Logger_init(void);

// Вывод сообщения в лог-файл и в консоль:
void log_msg(const char *fmt, ...);
