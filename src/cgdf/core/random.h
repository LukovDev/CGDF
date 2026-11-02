//
// random.h - Детерминированный рандомайзер на основе PCG32.
//

#pragma  once


// Подключаем:
#include "std.h"


// Инициализировать рандомайзер:
void random_init(uint64_t init_state);

// Получить случайное число в диапазоне uint32_t:
uint32_t random(void);
