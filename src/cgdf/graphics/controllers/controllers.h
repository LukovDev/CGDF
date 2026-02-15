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
    Window *window;
    Camera2D *camera;
    Vec2i fixed_mouse_pos;
    Vec2d target_pos;
    float target_zoom;
    float offset_scale;
    float min_zoom;
    float max_zoom;
    float friction;
    bool is_movement;
};


// Простой контроллер для 3D камеры:
struct CameraController3D {
    Window *window;
    Camera3D *camera;
    float mouse_sensitivity;
    float ctrl_speed;
    float speed;
    float shift_speed;
    float friction;
    bool up_is_forward;

    Vec3f up_dir;
    Vec3d target_pos;
    float target_fov;
    bool pressed_pass;
    bool is_pressed;
    bool is_movement;
};


// Орбитальный контроллер для 3D камеры:
struct CameraOrbitController3D {
    Window *window;
    Camera3D *camera;
    float mouse_sensitivity;
    float distance;
    float friction;
    bool up_is_forward;
    bool up_is_fixed;

    Vec3d rotation;
    Vec3d target_pos;
    Vec3d target_rot;
    float target_dst;
    float target_fov;
    bool pressed_pass;
    bool is_pressed;
    bool is_movement;
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
    float distance, float friction, bool up_is_forward, bool up_is_fixed
);

// Уничтожить орбитальный 3D контроллер:
void CameraOrbitController3D_destroy(CameraOrbitController3D **ctrl);

// Обновление контроллера:
void CameraOrbitController3D_update(CameraOrbitController3D *self, float dtime, bool pressed_pass);
