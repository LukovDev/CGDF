//
// controller3d_orbit.c - Реализует контроллер для управления 3D камеры орбитально.
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
    float distance, float friction, bool up_is_forward, bool up_is_fixed
) {
    if (!window || !camera) return NULL;
    CameraOrbitController3D *ctrl = (CameraOrbitController3D*)mm_alloc(sizeof(CameraOrbitController3D));

    // Заполняем поля:
    ctrl->window = window;
    ctrl->camera = camera;
    ctrl->mouse_sensitivity = mouse_sensitivity;
    ctrl->distance = distance;
    ctrl->friction = friction;
    ctrl->up_is_forward = up_is_forward;
    ctrl->up_is_fixed = up_is_fixed;

    ctrl->rotation = (Vec3d){camera->rotation.x, camera->rotation.y, camera->rotation.z};
    ctrl->target_pos = target_pos;
    ctrl->target_rot = (Vec3d){camera->rotation.x, camera->rotation.y, camera->rotation.z};
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

    // Константы управления:
    // const int k_roll_left  = K_LEFT;
    // const int k_roll_right = K_RIGHT;
    const int k_zoom = K_LALT;
    const float m_whl_factor   = 0.1f;  // Фактор уменьшения чувствительности колесика мыши.
    const int mouse_active_key = 2;     // Кнопка мыши для активации управления.
    const int mouse_pos_offset = 16;    // Область от края окна для телепортации мыши.

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
        // Управление с помощью клавиатуры:
        {
            // Управление вращением крена TODO правильно реализовать крен ко вращению камеры:
            // if (Input_get_key_pressed(window)[k_roll_left])  self->rotation.z -= 90.0f * dtime;
            // if (Input_get_key_pressed(window)[k_roll_right]) self->rotation.z += 90.0f * dtime;
        }

        // Управление с помощью мыши:
        {
            float roll = radians(self->rotation.z);
            float cam_dx = cosf(roll) * (float)mouse_rel.x - sinf(roll) * (float)mouse_rel.y;
            float cam_dy = sinf(roll) * (float)mouse_rel.x + cosf(roll) * (float)mouse_rel.y;

            // По горизонтали:
            if (self->rotation.x < -89.9f || self->rotation.x > +89.9f) {
                self->rotation.y -= cam_dx * self->mouse_sensitivity;  // Противоположный диапазон.
            } else self->rotation.y += cam_dx * self->mouse_sensitivity;  // Обычный диапазон.
            self->rotation.x += cam_dy * self->mouse_sensitivity;  // По вертикали.
            _check_mouse_pos_(window, camera->width, camera->height, mouse_pos_offset, mouse_pos_offset);

            // Ограничиваем вращение камеры до -180/180 градусов:
            if (self->rotation.x > +180.0f) { self->rotation.x = -180.0f; self->target_rot.x = -180.0f; }
            if (self->rotation.x < -180.0f) { self->rotation.x = +180.0f; self->target_rot.x = +180.0f; }
            if (self->rotation.y > +180.0f) { self->rotation.y = -180.0f; self->target_rot.y = -180.0f; }
            if (self->rotation.y < -180.0f) { self->rotation.y = +180.0f; self->target_rot.y = +180.0f; }
            if (self->rotation.z > +180.0f) { self->rotation.z = -180.0f; self->target_rot.z = -180.0f; }
            if (self->rotation.z < -180.0f) { self->rotation.z = +180.0f; self->target_rot.z = +180.0f; }

            // Ограничиваем вращение камеры вверх-вниз до -89/89 градусов:
            // (Если установить 90 градусов, могут быть проблемы в рассчетах вектора направления).
            if (!self->up_is_forward) {
                self->rotation.x = glm_clamp(self->rotation.x, -89.9f, +89.9f);
            }
        }
    }

    // Управление обзором камеры:
    if (Input_get_key_pressed(window)[k_zoom]) {
        if (!camera->is_ortho) {
            self->target_fov -= Input_get_mouse_wheel(window).y * m_whl_factor * self->target_fov;
        } else {
            camera->size.x -= Input_get_mouse_wheel(window).y * m_whl_factor * camera->size.x;
            camera->size.y -= Input_get_mouse_wheel(window).y * m_whl_factor * camera->size.y;
            camera->size.z -= Input_get_mouse_wheel(window).y * m_whl_factor * camera->size.z;
        }
    }

    // Если мы не попали на интерфейс:
    if (!pressed_pass && !Input_get_key_pressed(window)[k_zoom]) {
        // Масштабируем расстояние:
        self->target_dst -= Input_get_mouse_wheel(window).y * self->target_dst * m_whl_factor;
    }

    // Плавное масштабирование расстояния, вращение и обзора камеры:
    float fr = 1.0f - self->friction;
    if (fr > 0.0f) {
        self->distance += ((self->target_dst - self->distance) * 1.0f/fr) * dtime;
        self->target_rot.x += ((self->rotation.x - self->target_rot.x) * 1.0f/fr) * dtime;
        self->target_rot.y += ((self->rotation.y - self->target_rot.y) * 1.0f/fr) * dtime;
        self->target_rot.z += ((self->rotation.z - self->target_rot.z) * 1.0f/fr) * dtime;
        camera->fov += ((self->target_fov - camera->fov) * 1.0f/fr) * dtime;
    } else {
        self->distance = self->target_dst;
        self->target_rot = (Vec3d){self->rotation.x, self->rotation.y, self->rotation.z};
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
    camera->rotation = self->target_rot;

    // Проверяем перемещается камера или нет:
    vec3 diff;
    glm_vec3_sub((vec3){ self->rotation.x, self->rotation.y, self->rotation.z },
                 (vec3){ camera->rotation.x, camera->rotation.y, camera->rotation.z }, diff);
    if (roundf(glm_vec3_norm(diff)*1000.0f)/1000.0f > 0.05f || fabs(self->target_dst - self->distance) > 0.001f) {
        self->is_movement = true;
    } else {  self->is_movement = false; }
}
