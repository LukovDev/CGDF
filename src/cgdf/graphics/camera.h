//
// camera.h - Определяет функционал 2D и 3D камеры.
//

#pragma once


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include "window.h"


// Объявление структур:
typedef struct Camera2D Camera2D;  // 2D Камера.
typedef struct Camera3D Camera3D;  // 3D Камера.


// Структура 2D камеры:
struct Camera2D {
    mat4 view;        // Матрица вида.
    mat4 proj;        // Матрица проекции.
    Window *window;   // Указатель на окно.
    Vec2d position;   // Позиция камеры.
    float angle;      // Угол наклона камеры.
    float zoom;       // Масштаб камеры.
    float meter;      // Масштаб единицы измерения.
    bool _ui_begin_;  // Отрисовывается ли интерфейс.

    // Можно обратиться как к size.x/y так и к width/height:
    union {
        Vec2i size[2];  // Размер камеры.
        struct {
            int width;   // Ширина камеры.
            int height;  // Высота камеры.
        };
    };
};


// Структура 3D камеры:
struct Camera3D {
    mat4 view;        // Матрица вида.
    mat4 proj;        // Матрица проекции.
    Window *window;   // Указатель на окно.
    Vec3d position;   // Позиция камеры.
    Vec3d rotation;   // Поворот камеры (x=pitch, y=yaw, z=roll).
    Vec3d size;       // Размер камеры.
    float fov;        // Угол обзора.
    float z_far;      // Дальняя плоскость отсечения.
    float z_near;     // Ближняя плоскость отсечения.
    bool is_ortho;    // Ортографическая камера.
    int width;        // Ширина.
    int height;       // Высота.
    float _oldfov_;   // Старый угол обзора.
    float _oldfar_;   // Старая дальняя плоскость отсечения.
    float _oldnear_;  // Старая ближняя плоскость отсечения.
};


// -------- API 2D камеры: --------


// Создать 2D камеру:
Camera2D* Camera2D_create(Window *window, int width, int height, Vec2d position, float angle, float zoom);

// Уничтожить 2D камеру:
void Camera2D_destroy(Camera2D **camera);

// Обновление камеры:
void Camera2D_update(Camera2D *self);

// Изменить размер камеры:
void Camera2D_resize(Camera2D *self, int width, int height);

// Изменить масштаб единицы измерения:
void Camera2D_set_meter(Camera2D *self, float meter);

// Начало отрисовки UI:
void Camera2D_ui_begin(Camera2D *self);

// Конец отрисовки UI:
void Camera2D_ui_end(Camera2D *self);


// -------- API 3D камеры: --------


// Создать 3D камеру:
Camera3D* Camera3D_create(
    Window *window, int width, int height, Vec3d position, Vec3d rotation,
    Vec3d size, float fov, float z_near, float z_far, bool ortho
);

// Уничтожить 3D камеру:
void Camera3D_destroy(Camera3D **camera);

// Обновление камеры:
void Camera3D_update(Camera3D *self);

// Изменить размер камеры:
void Camera3D_resize(Camera3D *self, int width, int height, bool ortho);

// Посмотреть на указанную точку:
void Camera3D_look_at(Camera3D *self, Vec3d target);

// Установить проверку глубины:
void Camera3D_set_depth_test(Camera3D *self, bool enabled);

// Включить или отключить запись глубины:
void Camera3D_set_depth_mask(Camera3D *self, bool enabled);

// Включить или отключить смешивание:
void Camera3D_set_blending(Camera3D *self, bool enabled);

// Установить отсечение граней:
void Camera3D_set_cull_faces(Camera3D *self, bool enabled);

// Отсекать только задние грани:
void Camera3D_set_back_face_culling(Camera3D *self);

// Отсекать только передние грани:
void Camera3D_set_front_face_culling(Camera3D *self);

// Передняя грань против часовой стрелки (CCW):
void Camera3D_set_front_face_onleft(Camera3D *self);

// Передняя грань по часовой стрелке (CW):
void Camera3D_set_front_face_onright(Camera3D *self);

// Установить ортографическую проекцию:
void Camera3D_set_ortho(Camera3D *self, bool enabled);

// Узнать включена ли ортографическая проекция:
bool Camera3D_get_ortho(Camera3D *self);
