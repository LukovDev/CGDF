//
// cgdf.h - C Game Development Framework. Встраиваемая библиотека (пакет) для разработки игр на Си.
//

#pragma once


// Определения:
#define CGDF_VERSION "v1.0.0-alpha"


// Подключаем:
#include "core/core.h"


// Инициализировать библиотеку:
static inline bool CGDF_Init(void) {
    if (!core_init()) return false;
    // ...
    return true;
}


// Получить версию библиотеки:
static inline const char* CGDF_GetVersion(void) {
    return CGDF_VERSION;
}
