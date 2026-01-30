//
// utils.h - Вспомогательные функции в графике.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "camera.h"


// Переводит координаты точки на экране, в мировые координаты в 2D пространстве:
static inline Vec2d local_to_global_2d(Camera2D *camera, Vec2i point) {
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
