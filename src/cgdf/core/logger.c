//
// logger.c - Реализация кода обработки ошибок вылета программы и логирования.
//
// Теги логирования:
// [I] - Info.
// [W] - Warning.
// [E] - Error.
// [GL] - OpenGL.
// [Тег отсутствует] - Message.
//


// Подключаем:
#include "std.h"
#include "libs.h"
#include "time.h"
#include "logger.h"

#ifdef _WIN32
    #include <process.h>
#else
    #include <unistd.h>
#endif


// Определения:
#define LOG_FILE_PATH "data/latest.log"
static mtx_t _logger_mutex_;
static bool _logger_initialized_ = false;
static bool _logger_enabled_ = false;


// Преобразование кода сигнала в строку:
static inline const char* code_to_string(int code) {
    switch(code) {
        case SIGSEGV: return "Error: Segmentation fault";
        case SIGABRT: return "Error: Abort signal";
        case SIGFPE:  return "Error: Floating point exception";
        case SIGILL:  return "Error: Illegal instruction";
        case SIGTERM: return "Termination request";
        case SIGINT:  return "Interrupt (Ctrl+C)";
        default: return "Error: Unknown signal";
    }
}


// Обработчик вылета:
[[noreturn]] static void crash_handler(int sig) {
    if (_logger_enabled_) {
        FILE *f = fopen(LOG_FILE_PATH, "a");
        if (!f) f = stderr;

        // Сообщение о сигнале:
        const char *desc = code_to_string(sig);

        // Вывод одновременно в файл и консоль:
        fprintf(stderr, "\n%s. %d <0x%X>\n", desc, sig, sig);
        fprintf(f, "\n%s. %d <0x%X>\n", desc, sig, sig);

        // Немедленное обновление буферов:
        fflush(f);
        fflush(stderr);

        if (f != stderr) fclose(f);
    }
    _exit(sig);
}


// Инициализация логгера:
void Logger_init(void) {
    if (_logger_initialized_) return;

    // Устанавливаем обработчик сигналов:
    if (!_logger_initialized_) {
        signal(SIGSEGV, crash_handler);
        signal(SIGABRT, crash_handler);
        signal(SIGFPE,  crash_handler);
        signal(SIGILL,  crash_handler);
        signal(SIGTERM, crash_handler);
        signal(SIGINT,  crash_handler);
        // Инициализируем мьютекс:
        mtx_init(&_logger_mutex_, mtx_plain);
        _logger_initialized_ = true;
    }
}


// Включить логгер:
void Logger_enable(void) {
    if (!_logger_initialized_) return;
    mtx_lock(&_logger_mutex_);

    // Инициализируем лог-файл:
    TimeCurrent tc = Time_get_current(true);
    int y = tc.year, m = tc.month, d = tc.day, h = tc.hour, mi = tc.min, s = tc.sec, ms = tc.ms;
    FILE *f = fopen(LOG_FILE_PATH, "w");
    if (f) {
        fprintf(f, "[%02d:%02d:%02d.%03d]: Logger initialized: [%d.%02d.%02d]\n", h, mi, s, ms, y, m, d);
        fclose(f);
    }

    _logger_enabled_ = true;
    mtx_unlock(&_logger_mutex_);
}


// Выключить логгер:
void Logger_disable(void) {
    if (!_logger_initialized_) return;
    mtx_lock(&_logger_mutex_);
    _logger_enabled_ = false;
    mtx_unlock(&_logger_mutex_);
}


// Вывод сообщения в лог-файл и в консоль:
void log_msg(const char *fmt, ...) {
    if (!_logger_enabled_) return;

    mtx_lock(&_logger_mutex_);
    va_list args, args_copy;

    // Получаем время:
    TimeCurrent tc = Time_get_current(true);
    int h = tc.hour, mi = tc.min, s = tc.sec, ms = tc.ms;

    // Начинаем работу с аргументами:
    va_start(args, fmt);
    va_copy(args_copy, args);

    // В консоль:
    printf("[%02d:%02d:%02d.%03d]: ", h, mi, s, ms);
    vprintf(fmt, args);

    // В файл:
    if (_logger_enabled_) {
        FILE *f = fopen(LOG_FILE_PATH, "a");
        if (f) {
            fprintf(f, "[%02d:%02d:%02d.%03d]: ", h, mi, s, ms);
            vfprintf(f, fmt, args_copy);
            fclose(f);
        }
    }

    va_end(args_copy);
    va_end(args);
    mtx_unlock(&_logger_mutex_);
}
