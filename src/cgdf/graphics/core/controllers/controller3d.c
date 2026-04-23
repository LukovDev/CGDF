//
// controller3d.c - Реализует контроллер для управления 3D камеры.
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../input.h"
#include "../window.h"
#include "controllers.h"


// Создать 3D контроллер:
CameraController3D* CameraController3D_create(
    Window *window, Camera3D *camera, float mouse_sensitivity, float ctrl_speed,
    float speed, float shift_speed, float friction, bool up_is_forward
) {
    if (!window || !camera) return NULL;
    CameraController3D *ctrl = (CameraController3D*)mm_alloc(sizeof(CameraController3D));

    // Заполняем поля:
    ctrl->window = window;
    ctrl->camera = camera;
    ctrl->mouse_sensitivity = mouse_sensitivity;
    ctrl->ctrl_speed = ctrl_speed;
    ctrl->speed = speed;
    ctrl->shift_speed = shift_speed;
    ctrl->friction = friction;
    ctrl->up_is_forward = up_is_forward;

    ctrl->euler = (Vec3d){0.0, 0.0, 0.0};
    ctrl->target_pos = (Vec3d){camera->position.x, camera->position.y, camera->position.z};
    ctrl->target_fov = camera->fov;

    ctrl->pressed_pass = false;
    ctrl->is_pressed = false;
    ctrl->is_movement = false;
    return ctrl;
}

// Уничтожить 3D контроллер:
void CameraController3D_destroy(CameraController3D **ctrl) {
    if (!ctrl|| !*ctrl) return;
    mm_free(*ctrl);
    *ctrl = NULL;
}

// Обновление контроллера:
void CameraController3D_update(CameraController3D *self, float dtime, bool pressed_pass) {
    if (!self) return;
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
        // Вращение камеры:
        self->euler.y -= mouse_rel.x * self->mouse_sensitivity;
        self->euler.x -= mouse_rel.y * self->mouse_sensitivity;
        self->euler.x = glm_clamp(self->euler.x, -89.9f, 89.9f);
        _check_mouse_pos_(window, camera->width, camera->height);
        Camera3D_set_euler(camera, (Vec3d){self->euler.x, self->euler.y, 0.0});

        // Базис камеры:
        vec3 world_up = { 0.0, 1.0, 0.0 };  // Можно менять на другой вектор, но результат нестабилен.
        Vec3d f = Camera3D_get_forward(camera);
        Vec3d r = Camera3D_get_right(camera);
        Vec3d u = Camera3D_get_up(camera);
        vec3 forward = {f.x, f.y, f.z};
        vec3 right   = {r.x, r.y, r.z};
        vec3 up      = {u.x, u.y, u.z};

        // Вектор, который мы будем использовать для движения вперед:
        vec3 move_forward;

        // Свободный полёт:
        if (self->up_is_forward) {
            glm_vec3_copy(forward, move_forward);
        } else {
            glm_vec3_cross(right, world_up, move_forward);
            glm_vec3_normalize(move_forward);
            glm_vec3_copy(world_up, up);
            glm_vec3_inv(move_forward);
        }

        // Перемещение камеры:
        float speed = self->speed * dtime;
        if (keys[K_LSHIFT] || keys[K_RSHIFT])    speed = self->shift_speed * dtime;
        else if (keys[K_LCTRL] || keys[K_RCTRL]) speed = self->ctrl_speed * dtime;

        // Вперед/Назад:
        if (keys[k_forward]) {
            self->target_pos.x += move_forward[0] * speed;
            self->target_pos.y += move_forward[1] * speed;
            self->target_pos.z += move_forward[2] * speed;
        }
        if (keys[k_back]) {
            self->target_pos.x -= move_forward[0] * speed;
            self->target_pos.y -= move_forward[1] * speed;
            self->target_pos.z -= move_forward[2] * speed;
        }

        // Влево/Вправо:
        if (keys[k_left]) {
            self->target_pos.x -= right[0] * speed;
            self->target_pos.y -= right[1] * speed;
            self->target_pos.z -= right[2] * speed;
        }
        if (keys[k_right]) {
            self->target_pos.x += right[0] * speed;
            self->target_pos.y += right[1] * speed;
            self->target_pos.z += right[2] * speed;
        }

        // Вверх/Вниз:
        if (keys[k_up]) {
            self->target_pos.x += world_up[0] * speed;
            self->target_pos.y += world_up[1] * speed;
            self->target_pos.z += world_up[2] * speed;
        }
        if (keys[k_down]) {
            self->target_pos.x -= world_up[0] * speed;
            self->target_pos.y -= world_up[1] * speed;
            self->target_pos.z -= world_up[2] * speed;
        }
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
}
