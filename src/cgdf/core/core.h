//
// core.h - Заголовочный файл ядра. Подключает все части ядра здесь.
//
// Является независимым пакетом полезных инструментов и функций.
// Зависимости: "include/cglm/" - необходимо добавить в ваш проект в include папку.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// Подключаем:
#include "std.h"
#include "libs/tinycthread.h"
#include "array.h"
#include "constants.h"
#include "logger.h"
#include "files.h"
#include "hashtable.h"
#include "math.h"
#include "mm.h"
#include "pixmap.h"
#include "platform.h"
#include "time.h"
// #include "vector.h"  // Подключается в "math.h".


// Инициализация ядра:
static inline bool core_init() {
    Time_init();
    logger_init();
    return true;
}


#ifdef __cplusplus
}
#endif
