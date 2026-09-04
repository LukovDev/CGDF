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
    // Используем 8 классических 2D-направлений:
    switch (hash & 7) {
        case 0: return  1.0 * x +  0.0 * y;
        case 1: return -1.0 * x +  0.0 * y;
        case 2: return  0.0 * x +  1.0 * y;
        case 3: return  0.0 * x + -1.0 * y;
        case 4: return  0.7071067811865476 * x +  0.7071067811865476 * y;
        case 5: return -0.7071067811865476 * x +  0.7071067811865476 * y;
        case 6: return  0.7071067811865476 * x + -0.7071067811865476 * y;
        case 7: return -0.7071067811865476 * x + -0.7071067811865476 * y;
        default: return 0;
    }
}

// Базовая функция одиночной октавы шума Перлина (возвращает значение от -1.0 до 1.0):
double Noise_perlin2d(double x, double y, uint64_t seed) {
    double fX_floor = floor(x);
    double fY_floor = floor(y);

    int fX = (int)fX_floor;
    int fY = (int)fY_floor;

    x -= fX_floor;
    y -= fY_floor;

    double u = _perlin_fade_(x);
    double v = _perlin_fade_(y);

    int aa = _perlin_hash_(fX,     fY,     seed);
    int ab = _perlin_hash_(fX,     fY + 1, seed);
    int ba = _perlin_hash_(fX + 1, fY,     seed);
    int bb = _perlin_hash_(fX + 1, fY + 1, seed);

    double low_x  = _perlin_lerp_(u, _perlin_grad_(aa, x, y),       _perlin_grad_(ba, x - 1.0, y));
    double high_x = _perlin_lerp_(u, _perlin_grad_(ab, x, y - 1.0), _perlin_grad_(bb, x - 1.0, y - 1.0));
    double n = _perlin_lerp_(v, low_x, high_x);

    // Масштабируем и мягко контрастируем для получения честного диапазона [-1.0, 1.0]:
    n *= 2.0;
    n = n * (1.5 - 0.5 * n * n);
    return n;
}

// Функция генерации фрактального шума Перлина (от -1.0 до 1.0):
double Noise_perlin_fbm(double x, double y, uint64_t seed, int octaves, double persistence, double lacunarity) {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double max_value = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += Noise_perlin2d(x * frequency, y * frequency, seed + i) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    double result = total / max_value;
    if (result > 1.0)  return 1.0;
    if (result < -1.0) return -1.0;
    return result;
}
