//
// light.h - Создаёт общий апи для работы с простым освещением.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "renderer.h"
#include "texture.h"


// Объявление структур:
typedef struct Light2D Light2D;              // Простое освещение в 2D.
typedef struct SpriteLight2D SpriteLight2D;  // Спрайтовое 2D освещение.


// (Определение структуры Light2D находится в реализации).

// Структура спрайтового освещения в 2D:
struct SpriteLight2D {
    size_t  id;        // Айди источника света в массиве источников света.
    Light2D *light;    // Указатель на 2D освещение.
    Texture *texture;  // Указатель на текстуру света.
    Vec2f   position;  // Позиция источника света.
    Vec2f   size;      // Размер источника света.
    float   angle;     // Угол наклона источника света.
    Vec4f   color;     // Цвет источника света.
};


// -------- API простого 2D освещения: --------


// Создать простое 2D освещение:
Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity);

// Уничтожить простое 2D освещение:
void Light2D_destroy(Light2D **light);

// Начать захватывать отрисовку сцены:
void Light2D_begin(Light2D *self);

// Закончить захватывать отрисовку сцены:
void Light2D_end(Light2D *self);

// Установить фоновый цвет 2D освещения:
void Light2D_set_ambient(Light2D *self, Vec3f ambient);

// Установить интенсивность 2D освещения:
void Light2D_set_intensity(Light2D *self, float intensity);

// Изменить размер 2D освещения:
void Light2D_resize(Light2D *self, int width, int height);


// -------- API спрайтового 2D освещения: --------


// Создать точечное 2D освещение:
SpriteLight2D* SpriteLight2D_create(
    Light2D *light, Texture *texture,
    Vec2f position, Vec2f size,
    float angle, Vec4f color
);

// Уничтожить точечное 2D освещение:
void SpriteLight2D_destroy(SpriteLight2D **spritelight);
