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
static inline bool CGDF_init(void) {
    return core_init();
}

// Уничтожить фреймворк:
static inline bool CGDF_destroy(void) {
    return core_destroy();
}

// Получить версию фреймворка:
static inline const char* CGDF_GetVersion(void) {
    return CGDF_VERSION;
}


#ifdef __cplusplus
}
#endif
