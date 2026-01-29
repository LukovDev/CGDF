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

    ctrl->up_dir = (Vec3f){0.0f, 1.0f, 0.0f};  // TODO сделать правильно для планет и кастомных направлений пола.
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

    // Константы управления:
    const int k_forward = K_w;
    const int k_back    = K_s;
    const int k_left    = K_a;
    const int k_right   = K_d;
    const int k_up      = K_e;
    const int k_down    = K_q;

    // const int k_roll_left  = K_LEFT;
    // const int k_roll_right = K_RIGHT;
    const int k_zoom = K_LALT;
    const float m_whl_factor   = 0.1f;  // Фактор уменьшения чувствительности колесика мыши.
    const int mouse_pos_offset = 16;    // Область от края окна для телепортации мыши.

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
        // Управление клавиатурой:
        {
            float speed = self->speed * dtime;

            // Если нажимают на левый или правый шифт, ускорить перемещение:
            if (Input_get_key_pressed(window)[K_LSHIFT] || Input_get_key_pressed(window)[K_RSHIFT]) {
                speed = self->shift_speed * dtime;
            } else if (Input_get_key_pressed(window)[K_LCTRL] || Input_get_key_pressed(window)[K_RCTRL]) {
                speed = self->ctrl_speed * dtime;
            }

            // Углы камеры:
            float pitch = radians(camera->rotation.x), yaw = radians(camera->rotation.y);
            vec3 forward = {0.0f, 0.0f, 0.0f};
            vec3 right = {1.0f, 0.0f, 0.0f};
            vec3 up = {self->up_dir.x, self->up_dir.y, self->up_dir.z};

            // Направление по осям:
            // forward:
            glm_vec3_copy((vec3){cosf(pitch) * sinf(-yaw), sinf(pitch), cosf(pitch) * cosf(-yaw)}, forward);
            glm_vec3_normalize(forward);

            // right:
            glm_vec3_cross((vec3){self->up_dir.x, self->up_dir.y, self->up_dir.z}, forward, right);
            glm_vec3_normalize(right);

            // up:
            if (self->up_is_forward) {
                // Это надо чтобы повернуть вектор up относительно вектора forward.
                glm_vec3_inv(forward);
                glm_vec3_cross(right, forward, up);
                glm_vec3_normalize(up);
            } else {
                // Иначе просто делаем так чтобы камера не двигалась по вертикали в зависимости от направления камеры.
                glm_vec3_cross((vec3){self->up_dir.x, self->up_dir.y, self->up_dir.z}, right, forward);
                glm_vec3_normalize(forward);
            }

            // Управление движением:
            if (Input_get_key_pressed(window)[k_forward]) {  // Вперед:
                self->target_pos.x += forward[0] * speed;
                self->target_pos.y += forward[1] * speed;
                self->target_pos.z += forward[2] * speed;
            } if (Input_get_key_pressed(window)[k_back]) {  // Назад:
                self->target_pos.x -= forward[0] * speed;
                self->target_pos.y -= forward[1] * speed;
                self->target_pos.z -= forward[2] * speed;
            } if (Input_get_key_pressed(window)[k_left]) {  // Влево:
                self->target_pos.x -= right[0] * speed;
                self->target_pos.y -= right[1] * speed;
                self->target_pos.z -= right[2] * speed;
            } if (Input_get_key_pressed(window)[k_right]) {  // Вправо:
                self->target_pos.x += right[0] * speed;
                self->target_pos.y += right[1] * speed;
                self->target_pos.z += right[2] * speed;
            } if (Input_get_key_pressed(window)[k_up]) {  // Вверх:
                self->target_pos.x += up[0] * speed;
                self->target_pos.y += up[1] * speed;
                self->target_pos.z += up[2] * speed;
            } if (Input_get_key_pressed(window)[k_down]) {  // Вниз:
                self->target_pos.x -= up[0] * speed;
                self->target_pos.y -= up[1] * speed;
                self->target_pos.z -= up[2] * speed;
            }

            // Управление вращением крена TODO правильно применить крен к векторам направлений:
            // if (Input_get_key_pressed(window)[k_roll_left])  camera->rotation.z -= 90.0f * dtime;
            // if (Input_get_key_pressed(window)[k_roll_right]) camera->rotation.z += 90.0f * dtime;
        }

        // Управление мышью:
        {
            float roll = radians(camera->rotation.z);
            float cam_dx = cosf(roll) * (float)mouse_rel.x - sinf(roll) * (float)mouse_rel.y;
            float cam_dy = sinf(roll) * (float)mouse_rel.x + cosf(roll) * (float)mouse_rel.y;

            // По горизонтали:
            if (camera->rotation.x < -89.9f || camera->rotation.x > +89.9f) {
                camera->rotation.y -= cam_dx * self->mouse_sensitivity;  // Противоположный диапазон.
            } else camera->rotation.y += cam_dx * self->mouse_sensitivity;  // Обычный диапазон.
            camera->rotation.x += cam_dy * self->mouse_sensitivity;  // По вертикали.
            _check_mouse_pos_(window, camera->width, camera->height, mouse_pos_offset, mouse_pos_offset);

            // Ограничиваем вращение камеры до -180/180 градусов:
            camera->rotation.x = wrap_float(camera->rotation.x, -180.0f, +180.0f);
            camera->rotation.y = wrap_float(camera->rotation.y, -180.0f, +180.0f);
            camera->rotation.z = wrap_float(camera->rotation.z, -180.0f, +180.0f);

            // Ограничиваем вращение камеры вверх-вниз до -89/89 градусов:
            // (Если установить 90 градусов, могут быть проблемы в рассчетах вектора направления).
            if (!self->up_is_forward) {
                camera->rotation.x = glm_clamp(camera->rotation.x, -89.9f, +89.9f);
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

    // Плавное перемещение и масштабирование:
    float fr = 1.0f - self->friction;
    if (fr > 0.0f) {
        camera->position.x += ((self->target_pos.x - camera->position.x) * 1.0f/fr) * dtime;
        camera->position.y += ((self->target_pos.y - camera->position.y) * 1.0f/fr) * dtime;
        camera->position.z += ((self->target_pos.z - camera->position.z) * 1.0f/fr) * dtime;
        camera->fov += ((self->target_fov - camera->fov) * 1.0f/fr) * dtime;
    } else {
        camera->position = self->target_pos;
        camera->fov = self->target_fov;
    }

    // Проверяем перемещается камера или нет:
    vec3 diff;
    glm_vec3_sub((vec3){ self->target_pos.x, self->target_pos.y, self->target_pos.z },
                 (vec3){ camera->position.x, camera->position.y, camera->position.z }, diff);
    self->is_movement = (roundf(glm_vec3_norm(diff)*10000.0f)/10000.0f > 0.001f);
}
