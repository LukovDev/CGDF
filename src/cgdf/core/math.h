//
// math.h - Заголовочный файл, который определяет включения математики и определения других типов данных.
//

#pragma once


// Подключаем:
#include <math.h>
#include <cglm/cglm.h>
#include "vector.h"


// Перевести градусы в радианы:
static inline double radians(double degrees) { return degrees * (GLM_PI / 180.0); }

// Перевести радианы в градусы:
static inline double degrees(double radians) { return radians * (180.0 / GLM_PI); }


// Сравнение двух вещественных чисел:
static inline bool cmp_float(float a, float b) {
    float epsilon = 1e-6f;
    return fabsf(a-b) < epsilon;
}


// Зациклить вещественное число:
static inline float wrap_float(float v, float min, float max) {
    float range = max - min;
    v = fmodf(v - min, range);
    if (v < 0.0f) v += range;
    return v + min;
}


// Нормализовать угол:
static inline double normalize_deg(double a) {
    a = fmod(a, 360.0);
    if (a < -180.0) a += 360.0;
    if (a >  180.0) a -= 360.0;
    return a;
}
