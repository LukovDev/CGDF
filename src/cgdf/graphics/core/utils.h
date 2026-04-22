//
// utils.h - Вспомогательные функции в графике.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "window.h"
#include "camera.h"


// Перевести координаты на экране, в мировые координаты в 2D пространстве:
static inline Vec2d screen_to_world2d(Camera2D *camera, Vec2d point) {
    /*
        Сначала получаем позицию левого нижнего угла камеры в мировых координатах:
        - Позиция камеры минус половина видимой области (width/height), с учётом масштаба метра и зума.

        Затем берём координаты точки (point) в пикселях, переводим в мировые единицы
        (метры), учитываем зум камеры и инвертируем Y, потому что на экране Y растёт вниз, а в мире OpenGL - вверх.

        В итоге имеем:
        - Мировую позицию левого нижнего угла камеры (для смещения точки point).
        - Координаты точки в мировых единицах, но относительно камеры (локально).

        Складываем их и получаем итоговую мировую позицию точки.
    */

    // Позиция нижнего левого угла камеры с учётом метра и зума камеры:
    double camera_pos_x = camera->position.x - ((camera->width  * camera->zoom) / 2.0f) * (camera->meter / 100.0f);
    double camera_pos_y = camera->position.y - ((camera->height * camera->zoom) / 2.0f) * (camera->meter / 100.0f);

    // Позиция точки с учётом метра и зума камеры (Y координату точки инвертируем):
    double point_pos_x = (+(point.x                 ) * (camera->meter / 100.0f)) * camera->zoom;
    double point_pos_y = (-(point.y - camera->height) * (camera->meter / 100.0f)) * camera->zoom;

    // Складываем и возвращаем результат:
    return (Vec2d){
        camera_pos_x + point_pos_x,
        camera_pos_y + point_pos_y
    };
}


// Перевести мировые координаты в 2D пространстве, в пространство экрана:
static inline Vec2d world2d_to_screen(Camera2D *camera, Vec2d world_point) {
    // Вычисляем смещение точки относительно позиции камеры:
    double dx = world_point.x - camera->position.x;
    double dy = world_point.y - camera->position.y;

    // Переводим мировые единицы в экранные коэффициенты, учитывая зум и масштаб метра.
    // Делим на (meter/100 * zoom), чтобы понять, сколько "пикселей" в этом расстоянии:
    double screen_rel_x = dx / (camera->zoom * (camera->meter / 100.0f));
    double screen_rel_y = dy / (camera->zoom * (camera->meter / 100.0f));

    // Смещаем результат к центру экрана (так как камера центрирована):
    return (Vec2d){
        (camera->width / 2.0f) + screen_rel_x,
        (camera->height / 2.0f) + screen_rel_y
    };
}


// Перевести мировые координаты в 3D пространстве, в пространство экрана:
static inline Vec2d world3d_to_screen(Camera3D *camera, Vec3d world_pos) {
    vec4 world_v = {(float)world_pos.x, (float)world_pos.y, (float)world_pos.z, 1.0f};
    vec4 clip_coords;

    // Умножаем мировую точку на комбинированную матрицу:
    glm_mat4_mulv(camera->view_proj, world_v, clip_coords);

    // Проверка: находится ли точка за камерой?
    if (clip_coords[3] <= camera->z_near) return (Vec2d){INFINITY, INFINITY};

    float inv_w = 1.0f / clip_coords[3];
    return (Vec2d){
        (clip_coords[0] * inv_w + 1.0f) * 0.5f * (double)camera->width,
        (clip_coords[1] * inv_w + 1.0f) * 0.5f * (double)camera->height
    };
}


// Переводит координаты точки на экране, в мировые координаты на плоскости:
static inline bool camera_screen_to_plane(
    Window *window, mat4 view, mat4 proj, Vec2i mouse_pos,
    Vec3f plane_point, Vec3f plane_normal, Vec3f *out_pos
) {
    if (!window || !out_pos) return false;

    int w, h;
    Window_get_size(window, &w, &h);
    float nx = (2.0f * (float)mouse_pos.x) / (float)w - 1.0f;
    float ny = 1.0f - (2.0f * (float)mouse_pos.y) / (float)h;

    vec4 near_ndc = { nx, ny, -1.0f, 1.0f };
    vec4 far_ndc  = { nx, ny,  1.0f, 1.0f };

    mat4 vp, inv_vp;
    glm_mat4_mul(proj, view, vp);
    glm_mat4_inv(vp, inv_vp); // void

    vec4 near_world, far_world;
    glm_mat4_mulv(inv_vp, near_ndc, near_world);
    glm_mat4_mulv(inv_vp, far_ndc,  far_world);

    glm_vec4_scale(near_world, 1.0f / near_world[3], near_world);
    glm_vec4_scale(far_world,  1.0f / far_world[3],  far_world);

    vec3 ray_origin, ray_dir;
    glm_vec3_copy(near_world, ray_origin);
    glm_vec3_sub(far_world, near_world, ray_dir);
    glm_vec3_normalize(ray_dir);

    vec3 np, pp;
    np[0] = plane_normal.x;
    np[1] = plane_normal.y;
    np[2] = plane_normal.z;
    pp[0] = plane_point.x;
    pp[1] = plane_point.y;
    pp[2] = plane_point.z;

    float denom = glm_vec3_dot(ray_dir, np);
    if (fabsf(denom) < 1e-6f) return false;

    vec3 p0l0;
    glm_vec3_sub(pp, ray_origin, p0l0);
    float t = glm_vec3_dot(p0l0, np) / denom;
    if (t < 0.0f) return false;

    vec3 scaled, out_hit;
    glm_vec3_scale(ray_dir, t, scaled);
    glm_vec3_add(ray_origin, scaled, out_hit);
    *out_pos = (Vec3f){out_hit[0], out_hit[1], out_hit[2]};
    return true;
}
