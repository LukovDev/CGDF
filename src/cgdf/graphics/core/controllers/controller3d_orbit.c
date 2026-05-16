//
// controller3d_orbit.c - Реализует контроллер для управления 3D камеры орбитально (вокруг).
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../input.h"
#include "../window.h"
#include "controllers.h"


// Создать орбитальный 3D контроллер:
CameraOrbitController3D* CameraOrbitController3D_create(
    Window *window, Camera3D *camera, Vec3d target_pos, float mouse_sensitivity,
    float distance, float friction
) {
    if (!window || !camera) return NULL;
    CameraOrbitController3D *ctrl = (CameraOrbitController3D*)mm_alloc(sizeof(CameraOrbitController3D));

    // Заполняем поля:
    ctrl->window = window;
    ctrl->camera = camera;
    ctrl->mouse_sensitivity = mouse_sensitivity;
    ctrl->distance = distance;
    ctrl->friction = friction;

    ctrl->euler = (Vec3d){0.0f, 0.0f, 0.0f};
    ctrl->target_pos = target_pos;
    ctrl->target_rot = ctrl->euler;
    ctrl->target_dst = distance;
    ctrl->target_fov = camera->fov;

    ctrl->pressed_pass = false;
    ctrl->is_pressed = false;
    ctrl->is_movement = false;
    return ctrl;
}

// Уничтожить орбитальный 3D контроллер:
void CameraOrbitController3D_destroy(CameraOrbitController3D **ctrl) {
    if (!ctrl|| !*ctrl) return;
    mm_free(*ctrl);
    *ctrl = NULL;
}

// Обновление контроллера:
void CameraOrbitController3D_update(CameraOrbitController3D *self, float dtime, bool pressed_pass) {
    if (!self) return;
    Window *window = self->window;
    Camera3D *camera = self->camera;
    Vec2i mouse_rel = Input_get_mouse_rel(window);
    bool *keys = Input_get_key_pressed(window);

    // Константы управления:
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
        // Управление с помощью мыши:
        float roll = radians(self->euler.z);
        float cam_dx = cosf(roll) * (float)mouse_rel.x - sinf(roll) * (float)mouse_rel.y;
        float cam_dy = sinf(roll) * (float)mouse_rel.x + cosf(roll) * (float)mouse_rel.y;

        // По горизонтали:
        float rot_x = wrap_float(self->euler.x, -180.0f, 180.0f);
        if (rot_x < -89.9f || rot_x > 89.9f) {
            self->euler.y -= cam_dx * self->mouse_sensitivity;     // Противоположный диапазон.
        } else self->euler.y += cam_dx * self->mouse_sensitivity;  // Обычный диапазон.
        self->euler.x += cam_dy * self->mouse_sensitivity;         // По вертикали.
        _check_mouse_pos_(window, camera->width, camera->height);

        // Ограничиваем вращение камеры вверх-вниз до -89/89 градусов:
        self->euler.x = glm_clamp(self->euler.x, -89.9f, +89.9f);
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

    // Если мы не попали на интерфейс:
    if (!pressed_pass && !keys[k_zoom]) {
        // Масштабируем расстояние:
        self->target_dst -= Input_get_mouse_wheel(window).y * self->target_dst * self->mouse_sensitivity;
    }

    // Плавное масштабирование расстояния, вращение и обзора камеры:
    float fr = 1.0f - self->friction;
    if (fr > 0.0f) {
        self->distance += ((self->target_dst - self->distance) * 1.0f/fr) * dtime;
        self->target_rot.x += ((self->euler.x - self->target_rot.x) * 1.0f/fr) * dtime;
        self->target_rot.y += ((self->euler.y - self->target_rot.y) * 1.0f/fr) * dtime;
        self->target_rot.z += ((self->euler.z - self->target_rot.z) * 1.0f/fr) * dtime;
        camera->fov += ((self->target_fov - camera->fov) * 1.0f/fr) * dtime;
    } else {
        self->distance = self->target_dst;
        self->target_rot = (Vec3d){self->euler.x, self->euler.y, self->euler.z};
        camera->fov = self->target_fov;
    }

    // Если включен режим ортогональной проекции:
    if (Camera3D_get_ortho(camera)) {
        camera->size.x = self->distance / 4.0f;
        camera->size.y = self->distance / 4.0f;
        camera->size.z = self->distance / 4.0f;
    } else { camera->size = (Vec3d){1.0f, 1.0f, 1.0f}; }

    // Вычисляем позицию камеры и вращение:
    float pitch = radians(self->target_rot.x), yaw = radians(self->target_rot.y);
    vec3 forward = {cosf(pitch) * sinf(-yaw), sinf(pitch), cosf(pitch) * cosf(-yaw)};
    glm_vec3_normalize(forward);
    camera->position.x = self->target_pos.x + forward[0] * self->distance;
    camera->position.y = self->target_pos.y + forward[1] * self->distance;
    camera->position.z = self->target_pos.z + forward[2] * self->distance;

    // Вращаем камеру:
    Camera3D_look_at(camera, self->target_pos, (Vec3d){0.0f, 1.0f, 0.0f});

    // Проверяем перемещается камера или нет:
    vec3 diff;
    glm_vec3_sub(
        (vec3){ self->target_pos.x, self->target_pos.y, self->target_pos.z },
        (vec3){ camera->position.x, camera->position.y, camera->position.z },
        diff
    );
    float pos_delta = glm_vec3_norm(diff);
    float dst_delta = fabs(self->target_dst - self->distance);
    self->is_movement = (pos_delta > 0.001f || dst_delta > 0.001f);
}
