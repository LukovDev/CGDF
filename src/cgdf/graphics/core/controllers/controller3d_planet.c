//
// controller3d_planet.c - Реализует контроллер для управления 3D камеры на сфере (планете).
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../input.h"
#include "../window.h"
#include "controllers.h"


// Создать 3D планетарный контроллер:
CameraPlanetController3D* CameraPlanetController3D_create(
    Window *window, Camera3D *camera, float mouse_sensitivity, float ctrl_speed,
    float speed, float shift_speed, float friction, bool up_is_forward
) {
    if (!window || !camera) return NULL;
    CameraPlanetController3D *ctrl = (CameraPlanetController3D*)mm_alloc(sizeof(CameraPlanetController3D));

    // Заполняем поля:
    ctrl->window = window;
    ctrl->camera = camera;
    ctrl->mouse_sensitivity = mouse_sensitivity;
    ctrl->ctrl_speed = ctrl_speed;
    ctrl->speed = speed;
    ctrl->shift_speed = shift_speed;
    ctrl->friction = friction;
    ctrl->up_is_forward = up_is_forward;

    ctrl->center = (Vec3d){0.0f, 0.0f, 0.0f};
    ctrl->euler = (Vec3d){0.0f, 0.0f, 0.0f};
    ctrl->target_pos = (Vec3d){camera->position.x, camera->position.y, camera->position.z};
    ctrl->target_alt = 20.0f;
    ctrl->target_fov = camera->fov;

    ctrl->pressed_pass = false;
    ctrl->is_pressed = false;
    ctrl->is_movement = false;
    return ctrl;
}

// Уничтожить 3D планетарный контроллер:
void CameraPlanetController3D_destroy(CameraPlanetController3D **ctrl) {
    if (!ctrl|| !*ctrl) return;
    mm_free(*ctrl);
    *ctrl = NULL;
}

// Обновление контроллера:
void CameraPlanetController3D_update(CameraPlanetController3D *self, float dtime, bool pressed_pass) {
    if (!self) return;
    /*
    Window *window = self->window;
    Camera3D *camera = self->camera;
    Vec2i mouse_rel = Input_get_mouse_rel(window);
    bool *keys = Input_get_key_pressed(window);

    // Константы управления:
    const int k_forward = K_w;
    const int k_back    = K_s;
    const int k_left    = K_a;
    const int k_right   = K_d;
    const int k_up      = K_e;
    const int k_down    = K_q;
    const int k_zoom = K_LALT;

    // Кнопка мыши для активации управления:
    #ifdef __APPLE__
    const int mouse_active_key = 0;
    #else
    const int mouse_active_key = 2;
    #endif

    // Получаем нажатие кнопки мыши:
    bool mouse_pressed = Input_get_mouse_pressed(window)[mouse_active_key];

    // Eсли мы зажали ПКМ и не попали на интерфейс, то можем свободно управлять камерой пока не отпустим ПКМ:
    if (mouse_pressed && !pressed_pass && !self->is_pressed) self->is_pressed = true;

    // Если мы попали на интерфейс когда зажали ПКМ, то управлять мы не можем:
    if (mouse_pressed && pressed_pass && !self->is_pressed) self->pressed_pass = true;

    // Если мы отпустили ПКМ, то всё сбрасываем:
    if (!mouse_pressed) {
        self->pressed_pass = false;
        self->is_pressed = false;
    }

    // Управление камерой в случае если мы не попали на интерфейс и зажали ПКМ:
    if (self->is_pressed && !self->pressed_pass) {
        // Вращение головой:
        self->euler.y -= mouse_rel.x * self->mouse_sensitivity;
        self->euler.x -= mouse_rel.y * self->mouse_sensitivity;
        self->euler.x = glm_clamp(self->euler.x, -89.0f, 89.0f);
        _check_mouse_pos_(window, camera->width, camera->height);

        // Вектор вверх на планете:
        vec3 up;
        glm_vec3_sub(
            (vec3){camera->position.x, camera->position.y, camera->position.z},
            (vec3){self->center.x, self->center.y, self->center.z},
            up
        );
        glm_vec3_normalize(up);

        // Тут надо реализовать код который будет вращать камеру относительно центра планеты
        // а также позволять смотреть на небо и на землю (то есть вращать эйлеровые углы)
        // А также чтобы мы могли летать туда куда смотрим. Вверх вниз это у нас будет
        // поднятие вверх и вниз по вектору к центру планеты
        // Мы всегда должны летать по касательной сферы радиусом альтитуды
    }

    // Управление обзором камеры:
    if (keys[k_zoom]) {
        if (!camera->is_ortho) {
            self->target_fov -= Input_get_mouse_wheel(window).y * self->mouse_sensitivity * self->target_fov;
        } else {
            camera->size.x -= Input_get_mouse_wheel(window).y * self->mouse_sensitivity * camera->size.x;
            camera->size.y -= Input_get_mouse_wheel(window).y * self->mouse_sensitivity * camera->size.y;
            camera->size.z -= Input_get_mouse_wheel(window).y * self->mouse_sensitivity * camera->size.z;
        }
    }

    // Плавное перемещение камеры:
    float fr = 1.0f - self->friction;
    if (fr > 0.0f) {
        camera->position.x += ((self->target_pos.x - camera->position.x) * (1.0f / fr)) * dtime;
        camera->position.y += ((self->target_pos.y - camera->position.y) * (1.0f / fr)) * dtime;
        camera->position.z += ((self->target_pos.z - camera->position.z) * (1.0f / fr)) * dtime;
        camera->fov += ((self->target_fov - camera->fov) * 1.0f/fr) * dtime;
    } else {
        camera->position = self->target_pos;
        camera->fov = self->target_fov;
    }

    // Проверка на перемещение камеры:
    vec3 diff;
    glm_vec3_sub(
        (vec3){ self->target_pos.x, self->target_pos.y, self->target_pos.z },
        (vec3){ camera->position.x, camera->position.y, camera->position.z },
        diff
    );
    self->is_movement = (glm_vec3_norm(diff) > 0.001f);
    */
}
