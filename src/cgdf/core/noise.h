//
// noise.h - Заголовочный файл функций шума.
//

#pragma once


// Подключаем:
#include "std.h"


// Базовая функция одиночной октавы шума Перлина (возвращает значение от -1.0 до 1.0):
double Noise_perlin2d(double x, double y, uint64_t seed);

// Функция генерации фрактального шума Перлина (от -1.0 до 1.0):
double Noise_perlin_fbm(double x, double y, uint64_t seed, int octaves, double persistence, double lacunarity);
