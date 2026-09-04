//
// std.h - Заголовочный файл, импортирующий все минимальные стандартные библиотеки.
//

#pragma once

#if !defined(_WIN32) && !defined(__APPLE__)
    #define _POSIX_C_SOURCE 200809L
#endif


// Подключаем:
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <limits.h>
#include <stdalign.h>

// Атомарный тип float:
typedef struct atomic_float_t { uint32_t bits; } atomic_float_t;

// Записать данные в float:
static inline void atomic_float_store(atomic_float_t *obj, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(float));
    __atomic_store_n(&obj->bits, bits, __ATOMIC_RELAXED);
}

// Прочитать данные из float:
static inline float atomic_float_load(const atomic_float_t *obj) {
    uint32_t bits = __atomic_load_n(&obj->bits, __ATOMIC_RELAXED);
    float value;
    memcpy(&value, &bits, sizeof(float));
    return value;
}
