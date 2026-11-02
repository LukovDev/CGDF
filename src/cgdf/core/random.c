//
// random.c - Детерминированный рандомайзер на основе PCG32.
//


// Подключаем:
#include "std.h"
#include "random.h"


// Состояние генератора:
static uint64_t pcg_state = 0x853c49e6748fea9bULL;
static uint64_t pcg_inc   = 0xda3e39cb94b95bdbULL;


// Инициализировать рандомайзер:
void random_init(uint64_t init_state) {
    pcg_state = 0U;
    pcg_state = pcg_state * 6364136223846793005ULL + pcg_inc;
    pcg_state += init_state;
    pcg_state = pcg_state * 6364136223846793005ULL + pcg_inc;
}


// Получить случайное число в диапазоне uint32_t:
uint32_t random(void) {
    uint64_t old_state = pcg_state;
    pcg_state = old_state * 6364136223846793005ULL + pcg_inc;
    uint32_t xorshifted = ((old_state >> 18u) ^ old_state) >> 27u;
    uint32_t rot = old_state >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
