//
// light.h - Создаёт общий апи для работы с простым освещением.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>


// Объявление структур:
typedef struct Light2D Light2D;    // 2D освещение.
typedef struct Light3D Light3D;    // 3D освещение.
typedef struct Renderer Renderer;  // Повторное локальное определение.

// (Определение структуры Light2D и Light3D находится в реализации).


// -------- API 2D освещения: --------


// Создать 2D освещение:
Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity);

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


// Создать 3D освещение:
Light3D* Light3D_create(Renderer *renderer);

// Уничтожить 3D освещение:
void Light3D_destroy(Light3D **light);

// Изменить размер текстур 3D освещения:
void Light3D_resize(Light3D *self, int width, int height);

// Получить текстуру 3D освещения:
Texture* Light3D_get_light_tex(Light3D *self);

// Установить направление солнца:
void Light3D_set_sun_dir(Light3D *self, Vec3f direction);

// Получить направление солнца:
Vec3f Light3D_get_sun_dir(Light3D *self);

// Установить цвет солнца:
void Light3D_set_sun_color(Light3D *self, Vec3f color);

// Получить цвет солнца:
Vec3f Light3D_get_sun_color(Light3D *self);

// Установить интенсивность солнца:
void Light3D_set_sun_intensity(Light3D *self, float intensity);

// Получить интенсивность солнца:
float Light3D_get_sun_intensity(Light3D *self);

// Установить настройки фонового освещения:
void Light3D_set_ambient(Light3D *self, Vec3f color, Vec3f ground_color, float intensity);

// Отрисовать освещение:
void Light3D_render(Light3D *self, mat4 proj, mat4 view);
