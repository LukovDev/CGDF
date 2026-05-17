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
#include "array.h"
#include "constants.h"
#include "files.h"
#include "hashtable.h"
#include "info.h"
#include "jobsystem.h"
#include "libs.h"
#include "logger.h"
#include "math.h"
#include "mm.h"
#include "node.h"
#include "pixmap.h"
#include "platform.h"
#include "time.h"


// Инициализация ядра:
static inline bool core_init(void) {
    Time_init();
    Logger_init();
    JobSystem_init();

    // Инициализация генератора случайных чисел:
    srand((uint32_t)Time_now(NULL));

    // Настраиваем пути для OS X:
    #ifdef __APPLE__
        Files_fix_apple_path();
    #endif
    return true;
}


// Уничтожение ядра:
static inline bool core_destroy(void) {
    JobSystem_destroy();  // Уничтожение работы с задачами (потоками).
    return true;
}


#ifdef __cplusplus
}
#endif
