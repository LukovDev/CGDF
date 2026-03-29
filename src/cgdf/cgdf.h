//
// cgdf.h - C Game Development Framework. Фреймворк для разработки игр на Си.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// Определения:
#define CGDF_VERSION "v1.0.0-alpha"

// Подключаем:
#include "core/core.h"

// Инициализировать фреймворк:
static inline bool CGDF_Init(void) {
    if (!core_init()) return false;
    // ...
    return true;
}

// Получить версию фреймворка:
static inline const char* CGDF_GetVersion(void) {
    return CGDF_VERSION;
}


#ifdef __cplusplus
}
#endif
