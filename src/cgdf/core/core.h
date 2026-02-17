//
// core.h - Заголовочный файл ядра. Подключает все части ядра здесь.
//
// Является независимым пакетом полезных инструментов и функций, для любого приложения на си.
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
static inline bool core_init(void) {
    Time_init();
    logger_init();

    // Настраиваем пути для OS X:
    #ifdef __APPLE__
        Files_fix_apple_path();
    #endif
    return true;
}


#ifdef __cplusplus
}
#endif
