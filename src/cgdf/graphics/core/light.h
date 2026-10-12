//
// light.h - Создаёт общий апи для работы с простым освещением.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>


// Объявление структур:
typedef struct Light2D Light2D;    // 2D освещение.
typedef struct Renderer Renderer;  // Повторное локальное определение.

// (Определение структуры Light2D находится в реализации).


// Типы 3D источников света:
typedef enum Light3DType {
    LIGHT3D_POINT,
    LIGHT3D_SPOT,
} Light3DType;


// -------- API 2D освещения: --------


// Создать 2D освещение:
[[nodiscard]] Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity);

// Уничтожить 2D освещение:
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

// Изменить размер текстур 2D освещения:
void Light2D_resize(Light2D *self, int width, int height);


// -------- API 3D освещения: --------

// Отрисовать точечный источник света:
void Light3D_point(
    Renderer *self, Vec3f position, float size, Vec3f color,
    float intensity, float radius, bool cast_shadow
);

// Отрисовать прожекторный источник света:
void Light3D_spot(
    Renderer *self, Vec3f position, Vec3f direction, float size, Vec3f color,
    float intensity, float distance, float angle, float inner_angle, bool cast_shadow
);
