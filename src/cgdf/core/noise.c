//
// noise.c - Реализации функций шума.
//


// Подключаем:
#include "std.h"
#include "noise.h"


// Хэш-функция для перемешивания координат на основе сида:
static int _perlin_hash_(int x, int y, uint64_t seed) {
    uint64_t ux = (uint64_t)x;
    uint64_t uy = (uint64_t)y;
    uint64_t h = seed ^ (ux * 123456789ULL) ^ (uy * 987654321ULL);
    h = (h ^ (h >> 16)) * 0x45d9f3bULL;
    h = (h ^ (h >> 16)) * 0x45d9f3bULL;
    h = h ^ (h >> 16);
    return (int)(h & 255);
}

// Функция плавного интерполирования:
static double _perlin_fade_(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Линейная интерполяция:
static double _perlin_lerp_(double t, double a, double b) {
    return a + t * (b - a);
}

// Вычисление скалярного произведения случайного градиентного вектора и вектора до точки:
static double _perlin_grad_(int hash, double x, double y) {
    switch (hash & 7) {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x * 1.41421356;
        case 5: return -x * 1.41421356;
        case 6: return  y * 1.41421356;
        case 7: return -y * 1.41421356;
        default: return 0;
    }
}

// Базовая функция одиночной октавы шума Перлина (возвращает значение от -1.0 до 1.0):
double Noise_perlin2d(double x, double y, uint64_t seed) {
    int fX = (int)floor(x);
    int fY = (int)floor(y);

    x -= floor(x);
    y -= floor(y);

    double u = _perlin_fade_(x);
    double v = _perlin_fade_(y);

    int aa = _perlin_hash_(fX,     fY,     seed);
    int ab = _perlin_hash_(fX,     fY + 1, seed);
    int ba = _perlin_hash_(fX + 1, fY,     seed);
    int bb = _perlin_hash_(fX + 1, fY + 1, seed);

    double n = _perlin_lerp_(v,
        _perlin_lerp_(u, _perlin_grad_(aa, x, y),
        _perlin_grad_(ba, x - 1.0, y)),
        _perlin_lerp_(u, _perlin_grad_(ab, x, y - 1.0),
        _perlin_grad_(bb, x - 1.0, y - 1.0))
    ) * 1.7071067812;
    if (n >  1.0) n =  1.0;
    if (n < -1.0) n = -1.0;
    return n;
}

// Функция генерации фрактального шума Перлина (от -1.0 до 1.0):
double Noise_perlin_fbm(double x, double y, uint64_t seed, int octaves, double persistence, double lacunarity) {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double max_value = 0.0;

    for (int i = 0; i < octaves; i++) {
        // Получаем шум для текущей октавы (передаем i как смещение сида, чтобы слои не накладывались одинаково):
        total += Noise_perlin2d(x * frequency, y * frequency, seed + i) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // Возвращаем результат, строго нормализованный в диапазон [-1.0, 1.0]:
    return total / max_value;
}
