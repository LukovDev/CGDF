//
// time.h - Заголовок с полезными кроссплатформенными способами работы с временем.
//

#pragma once


// Подключаем:
#include "std.h"
#include "math.h"
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/time.h>
    #include <unistd.h>
#endif


// Объявление структур:
typedef struct TimeCurrent TimeCurrent;      // Текущее время.
typedef struct TimeOffsetUTC TimeOffsetUTC;  // Смещение пояса UTC.


// Текущее время:
struct TimeCurrent {
    uint64_t ticks;
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t min;
    uint32_t sec;
    uint32_t ms;
};


// Смещение пояса UTC:
struct TimeOffsetUTC {
    int32_t offset;
    int32_t hour;
    int32_t min;
};


// Возвращает время с начала Unix-эпохи в секундах (double) с точностью до мс:
static inline double Time_now(double *x) {
    double result;
    #if defined(_WIN32) || defined(_WIN64)
        FILETIME ft;
        ULARGE_INTEGER uli;
        GetSystemTimeAsFileTime(&ft);
        uli.LowPart  = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        // Вычитаем разницу между 1601 и 1970 годами (в 100-нс интервалах):
        result = (double)(uli.QuadPart - 116444736000000000ULL) / 10000000.0;
    #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        result = (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
    #endif
    if (x != NULL) { *x = result; }
    return result;
}


// Остановить выполнение кода на определенное время в секундах с дробной частью (малая точность чем у Time_delay):
static inline void Time_sleep(double seconds) {
    if (seconds <= 0.0) { return; }
    #if defined(_WIN32) || defined(_WIN64)
        Sleep((unsigned long)(seconds*1000.0));
    #else
        struct timespec ts;
        ts.tv_sec  = (time_t)floor(seconds);
        ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
        nanosleep(&ts, NULL);
    #endif
}


// Задержать выполнение кода на определенное время в секундах с дробной частью (большая точность чем у Time_sleep):
// Комбинированный sleep (по возможности) + active wait для докрутки ожидания в коде.
static inline void Time_delay(double seconds) {
    if (seconds <= 0.0) return;
    // Задержка переключения со сна в разных системах:
    #if defined(_WIN32) || defined(_WIN64)
        const double OS_TICK = 0.015625;
    #else
        const double OS_TICK = 0.001;
    #endif

    double start = Time_now(NULL);                        // Получаем текущее время.
    double sleep_dur = seconds - fmod(seconds, OS_TICK);  // Сколько реально времени точно можно поспать.
    if (sleep_dur > 0.0) { Time_sleep(sleep_dur); }       // Спим не нагружая процессор циклом.
    double target = start + seconds;
    while (Time_now(NULL) < target);  // Докручиваем время проверяя текущее время с целевым временем.
}


// Узнать у устройства смещение часового времени:
static inline TimeOffsetUTC Time_get_utc_offset() {
    // Вычисляем:
    time_t now = time(NULL);
    struct tm gmt;
    struct tm loc;
    #if defined(_WIN32)
        gmtime_s(&gmt, &now);
        localtime_s(&loc, &now);
    #else
        gmt = *gmtime(&now);
        loc = *localtime(&now);
    #endif
    time_t gmt_sec = mktime(&gmt);
    time_t loc_sec = mktime(&loc);

    // Сохраняем и возвращаем:
    TimeOffsetUTC offset_utc;
    int32_t offset = (int32_t)difftime(loc_sec, gmt_sec);
    offset_utc.offset = offset;
    offset_utc.hour = offset / 3600;
    offset_utc.min = (abs(offset) % 3600) / 60;
    return offset_utc;
}


// Получить дату и время:
static inline bool _Time_is_leap_(int year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }
static inline TimeCurrent Time_get_current(bool local_time) {
    TimeCurrent result;

    // Получаем часовой пояс устройства:
    uint32_t offset_utc = Time_get_utc_offset().offset;
    double unix_time = Time_now(NULL);

    // Целые секунды и миллисекунды:
    uint64_t utc_sec = (uint64_t)unix_time;
    double frac = unix_time - (double)utc_sec;
    if (frac < 0) frac = 0;  // На всякий.

    // Считаем:
    uint64_t total_sec = utc_sec;
    if (local_time) { total_sec += offset_utc; }
    result.ticks = total_sec;  // Целые секунды (всего тиков).
    result.ms = (uint32_t)(frac * 1000.0f);  // Миллисекунды.
    result.sec = total_sec % 60;  // Секунды.
    total_sec /= 60;
    result.min = total_sec % 60;  // Минуты.
    total_sec /= 60;
    result.hour = total_sec % 24;  // Часы.
    total_sec /= 24;

    // Определяем год:
    result.year = 1970;
    uint32_t days = total_sec;
    int cycles400 = days / 146097;
    result.year += cycles400 * 400;
    days -= (uint32_t)cycles400 * 146097;
    while (1) {
        uint32_t dy = _Time_is_leap_(result.year) ? 366 : 365;
        if (days < dy) break;
        days -= dy;
        result.year++;
    }

    // Определяем месяц и день:
    static const uint32_t mdays_norm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const uint32_t mdays_leap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const uint32_t *md = _Time_is_leap_(result.year) ? mdays_leap : mdays_norm;
    result.month = 1;
    for (int i = 0; i < 12; i++) {
        if (days < md[i]) break;
        days -= md[i];
        result.month++;
    }
    result.day = days + 1;

    // Возвращаем время:
    return result;
}


// Инициализация времени:
static inline void Time_init() {
    #if defined(_WIN32)
        _tzset();
    #else
        tzset();
    #endif
}
