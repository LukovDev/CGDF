//
// logger.c - Реализация кода обработки ошибок вылета программы и логирования.
//
// define CGDF_NO_LOG_FILE - Чтобы отключить запись в лог-файл.
//
// Теги логирования:
// [I] - Info.
// [W] - Warning.
// [E] - Error.
// [GL] - OpenGL.
// [Тег отсутствует] - Message.
//


// Подключаем:
#include "constants.h"
#include "std.h"
#include "time.h"
#include "logger.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif


// Преобразование кода сигнала в строку:
static inline const char* code_to_string(int code) {
    switch(code) {
        case SIGSEGV: return "Error: Segmentation fault";
        case SIGABRT: return "Error: Abort signal";
        case SIGFPE:  return "Error: Floating point exception";
        case SIGILL:  return "Error: Illegal instruction";
        case SIGTERM: return "Termination request";
        case SIGINT:  return "Interrupt (Ctrl+C)";
        case ENOMEM:  return "Error: Out of memory";
        default: return "Error: Unknown signal";
    }
}


// Обработчик вылета:
static inline void crash_handler(int sig) {
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
    _exit(sig);
}


// Инициализация логгера:
void logger_init() {
    // Инициализируем лог-файл:
    #ifndef CGDF_NO_LOG_FILE
    TimeCurrent tc = Time_get_current(true);
    int y = tc.year, m = tc.month, d = tc.day, h = tc.hour, mi = tc.min, s = tc.sec, ms = tc.ms;
    FILE *f = fopen(LOG_FILE_PATH, "w");
    if (f) {
        fprintf(f, "[%02d:%02d:%02d.%03d]: Logger initialized: [%d.%02d.%02d]\n", h, mi, s, ms, y, m, d);
        fclose(f);
    }
    #endif

    // Устанавливаем обработчик сигналов:
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE,  crash_handler);
    signal(SIGILL,  crash_handler);
    signal(SIGTERM, crash_handler);
    signal(SIGINT,  crash_handler);
    signal(ENOMEM,  crash_handler);
}


// Вывод сообщения в лог-файл и в консоль:
void log_msg(const char *fmt, ...) {
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
    #ifndef CGDF_NO_LOG_FILE
    FILE *f = fopen(LOG_FILE_PATH, "a");
    if (f) {
        fprintf(f, "[%02d:%02d:%02d.%03d]: ", h, mi, s, ms);
        vfprintf(f, fmt, args_copy);
        fclose(f);
    }
    #endif

    va_end(args_copy);
    va_end(args);
}
