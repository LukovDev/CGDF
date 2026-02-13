//
// controller2d.c - Реализует контроллер для управления 2D камеры.
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../input.h"
#include "../window.h"
#include "controllers.h"


// Создать 2D контроллер:
CameraController2D* CameraController2D_create(
    Window *window, Camera2D *camera, float offset_scale,
    float min_zoom, float max_zoom, float friction
) {
    if (!window || !camera) return NULL;
    CameraController2D *ctrl = (CameraController2D*)mm_alloc(sizeof(CameraController2D));

    // Заполняем поля:
    ctrl->window = window;
    ctrl->camera = camera;
    ctrl->fixed_mouse_pos = (Vec2i){0, 0};
    ctrl->target_pos = camera->position;
    ctrl->offset_scale = offset_scale;
    ctrl->min_zoom = min_zoom;
    ctrl->max_zoom = max_zoom;
    ctrl->friction = friction;
    ctrl->is_movement = false;
    return ctrl;
}

// Уничтожить 2D контроллер:
void CameraController2D_destroy(CameraController2D **ctrl) {
    if (!ctrl|| !*ctrl) return;
    mm_free(*ctrl);
    *ctrl = NULL;
}

// Обновление контроллера:
void CameraController2D_update(CameraController2D *self, float dtime, bool pressed_pass) {
    if (!self) return;
    Window *window = self->window;
    Camera2D *camera = self->camera;
    Vec2i mouse_pos = Input_get_mouse_pos(window);
    Vec2i mouse_rel = Input_get_mouse_rel(window);
    float mouse_scroll = (float)Input_get_mouse_wheel(window).y;
    bool is_zooming = !pressed_pass || Input_get_mouse_pressed(window)[0];
    float meter = camera->meter;
    const int mouse_pos_offset = 16;  // Область от края окна для телепортации мыши.

    // Если нажимают на колесико мыши:
    if (Input_get_mouse_pressed(window)[1]) {
        float zoom = camera->zoom;
        Vec2i delta_pos = (Vec2i){mouse_pos.x - self->fixed_mouse_pos.x, mouse_pos.y - self->fixed_mouse_pos.y};
        self->target_pos.x += ((delta_pos.x*1.0f/(250.0f*5.0f))*self->offset_scale*zoom*meter)*(dtime*60.0f);
        self->target_pos.y -= ((delta_pos.y*1.0f/(250.0f*5.0f))*self->offset_scale*zoom*meter)*(dtime*60.0f);
    } else if (Input_get_mouse_pressed(window)[2]) {
        is_zooming = true;
        camera->position.x -= (mouse_rel.x * camera->zoom) * meter/100.0f;
        camera->position.y += (mouse_rel.y * camera->zoom) * meter/100.0f;
        self->target_pos = camera->position;
        _check_mouse_pos_(window, camera->width, camera->height, mouse_pos_offset, mouse_pos_offset);
        self->fixed_mouse_pos = (Vec2i){mouse_pos.x, mouse_pos.y};
    } else {
        self->fixed_mouse_pos = (Vec2i){mouse_pos.x, mouse_pos.y};
    }

    // Масштабирование камеры:
    if (is_zooming) camera->zoom -= mouse_scroll * camera->zoom * 0.1f;
    if (camera->zoom*meter < self->min_zoom) camera->zoom = self->min_zoom*1.0f/meter;
    if (camera->zoom       > self->max_zoom) camera->zoom = self->max_zoom;

    // Плавное перемещение камеры:
    float fr = 1.0f - self->friction;
    if (fr > 0.0f) {
        camera->position.x += ((self->target_pos.x - camera->position.x) * 1.0f/fr) * dtime;
        camera->position.y += ((self->target_pos.y - camera->position.y) * 1.0f/fr) * dtime;
    } else {
        camera->position = (Vec2d){self->target_pos.x, self->target_pos.y};
    }

    // Проверяем перемещается камера или нет:
    vec3 diff;
    glm_vec2_sub((vec2){ self->target_pos.x, self->target_pos.y },
                 (vec2){ camera->position.x, camera->position.y }, diff);
    self->is_movement = (roundf(glm_vec2_norm(diff)*10000.0f)/10000.0f > 0.001f);
}
