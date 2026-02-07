//
// utils.h - Вспомогательные функции в графике.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "window.h"
#include "camera.h"


// Переводит координаты точки на экране, в мировые координаты в 2D пространстве:
static inline Vec2d local_to_global_2d(Camera2D *camera, Vec2i point) {
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
