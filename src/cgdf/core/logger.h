//
// logger.h
//

#pragma once


// Инициализация логгера:
void logger_init();

// Вывод сообщения в лог-файл и в консоль:
void log_msg(const char *fmt, ...);
