//
// controllers.h - Простые контроллеры камеры.
//

#pragma once


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include "../camera.h"
#include "../window.h"


// Объявление структур:
typedef struct CameraController2D CameraController2D;  // Простой контроллер для 2D камеры.
typedef struct CameraController3D CameraController3D;  // Простой контроллер для 3D камеры.
typedef struct CameraOrbitController3D CameraOrbitController3D;  // Орбитальный контроллер для 3D камеры.


// Простой контроллер для 2D камеры:
struct CameraController2D {
    Window *window;      // Указатель на окно.
    Camera2D *camera;    // Указатель на 2D камеру.
    Vec2i fixed_mouse_pos;  // Прошлая позиция мыши.
    Vec2d target_pos;    // Целевая позиция камеры.
    float target_zoom;   // Целевой масштаб камеры.
    float offset_scale;  // Коэффициент смещения мыши для скольжения через колесико мыши.
    float min_zoom;      // Минимальный масштаб камеры.
    float max_zoom;      // Максимальный масштаб камеры.
    float friction;      // Коэффициент скольжения камеры.
    bool is_movement;    // Перемещается ли камера или нет.
};


// Простой контроллер для 3D камеры:
struct CameraController3D {
    Window *window;      // Указатель на окно.
    Camera3D *camera;    // Указатель на 3D камеру.
    float mouse_sensitivity;  // Коэффициент чувствительности мыши.
    float ctrl_speed;    // Скорость камеры при зажатом CTRL (ед/сек).
    float speed;         // Скорость камеры без клавиш модификаторов (ед/сек).
    float shift_speed;   // Скорость камеры при зажатом SHIFT (ед/сек).
    float friction;      // Коэффициент скольжения камеры.
    bool up_is_forward;  // Вверх - вперед.
    Vec3f up_dir;        // Направление вверх.
    Vec3d target_pos;    // Целевая позиция камеры.
    float target_fov;    // Целевой угол обзора.
    bool pressed_pass;   // Пропуск нажатия (внутренняя логика).
    bool is_pressed;     // Нажата клавиша (внутренняя логика).
    bool is_movement;    // Перемещается ли камера или нет.
};


// Орбитальный контроллер для 3D камеры:
struct CameraOrbitController3D {
    Window *window;      // Указатель на окно.
    Camera3D *camera;    // Указатель на 3D камеру.
    float mouse_sensitivity;  // Коэффициент чувствительности мыши.
    float distance;      // Дистанция камеры от цели.
    float friction;      // Коэффициент скольжения камеры.
    bool up_is_forward;  // Вверх - вперед.
    Vec3d rotation;      // Поворот камеры.
    Vec3d target_pos;    // Целевая позиция камеры.
    Vec3d target_rot;    // Целевой поворот камеры.
    float target_dst;    // Целевая дистанция камеры.
    float target_fov;    // Целевой угол обзора.
    bool pressed_pass;   // Пропуск нажатия (внутренняя логика).
    bool is_pressed;     // Нажата клавиша (внутренняя логика).
    bool is_movement;    // Перемещается ли камера или нет.
};


// -------- Общие вспомогательные функции: --------


// Проверить позицию мыши в диапазоне окна:
static inline void _check_mouse_pos_(Window *window, int width, int height, int x_pos_detect, int y_pos_detect) {
    Vec2i mouse_pos = Input_get_mouse_pos(window);
    Vec2i win_size = (Vec2i){width, height};
    if (mouse_pos.x < x_pos_detect)              Input_set_mouse_pos(window, win_size.x-x_pos_detect, mouse_pos.y);
    if (mouse_pos.y < y_pos_detect)              Input_set_mouse_pos(window, mouse_pos.x, win_size.y-y_pos_detect);
    if (mouse_pos.x > win_size.x - x_pos_detect) Input_set_mouse_pos(window, x_pos_detect, mouse_pos.y);
    if (mouse_pos.y > win_size.y - y_pos_detect) Input_set_mouse_pos(window, mouse_pos.x, y_pos_detect);
}


// -------- API 2D контроллера: --------


// Создать 2D контроллер:
CameraController2D* CameraController2D_create(
    Window *window, Camera2D *camera, float offset_scale,
    float min_zoom, float max_zoom, float friction
);

// Уничтожить 2D контроллер:
void CameraController2D_destroy(CameraController2D **ctrl);

// Обновление контроллера:
void CameraController2D_update(CameraController2D *self, float dtime, bool pressed_pass);


// -------- API 3D контроллера: --------


// Создать 3D контроллер:
CameraController3D* CameraController3D_create(
    Window *window, Camera3D *camera, float mouse_sensitivity, float ctrl_speed,
    float speed, float shift_speed, float friction, bool up_is_forward
);

// Уничтожить 3D контроллер:
void CameraController3D_destroy(CameraController3D **ctrl);

// Обновление контроллера:
void CameraController3D_update(CameraController3D *self, float dtime, bool pressed_pass);


// -------- API 3D орбитального контроллера: --------


// Создать орбитальный 3D контроллер:
CameraOrbitController3D* CameraOrbitController3D_create(
    Window *window, Camera3D *camera, Vec3d target_pos, float mouse_sensitivity,
    float distance, float friction, bool up_is_forward
);

// Уничтожить орбитальный 3D контроллер:
void CameraOrbitController3D_destroy(CameraOrbitController3D **ctrl);

// Обновление контроллера:
void CameraOrbitController3D_update(CameraOrbitController3D *self, float dtime, bool pressed_pass);
