//
// light.h - Создаёт общий апи для работы с простым освещением.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "renderer.h"


// Объявление структур:
typedef struct Light2D Light2D;  // Простое освещение в 2D.

// (Определение структуры Light2D находится в реализации).


// -------- API простого 2D освещения: --------


// Создать простое 2D освещение:
Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity);

// Уничтожить простое 2D освещение:
void Light2D_destroy(Light2D **light);

// Начать захватывать отрисовку сцены:
void Light2D_scene_begin(Light2D *self);

// Закончить захватывать отрисовку сцены:
void Light2D_scene_end(Light2D *self);

// Начать захватывать отрисовку света:
void Light2D_light_begin(Light2D *self);

// Закончить захватывать отрисовку света:
void Light2D_light_end(Light2D *self);

// Отрисовать освещение (композит двух проходов отрисовки):
void Light2D_render(Light2D *self);

// Установить фоновый цвет 2D освещения:
void Light2D_set_ambient(Light2D *self, Vec3f ambient);

// Установить интенсивность 2D освещения:
void Light2D_set_intensity(Light2D *self, float intensity);

// Изменить размер 2D освещения:
void Light2D_resize(Light2D *self, int width, int height);
