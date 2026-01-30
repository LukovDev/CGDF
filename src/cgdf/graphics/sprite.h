//
// sprite.h - Общий функционал для отображения спрайтов.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include "renderer.h"
#include "texture.h"


// Объявление структур:
typedef struct Sprite2D Sprite2D;  // Двумерный спрайт.


// Двумерный спрайт:
struct Sprite2D {
    Renderer *renderer;   // Рендерер (для доступа к графическому апи и шейдеру).
    Texture *texture;     // Текстура спрайта.
    float x, y;           // Координаты.
    float width, height;  // Размеры.
    float angle;          // Угол вращения (против часовой стрелки).
    Vec4f color;          // Вектор цвета (значения от 0.0 до 1.0).
    bool custom_shader;   // Использовать пользовательский шейдер (true) или встроенный (false).
    void (*render) (Sprite2D *self);  // Отрисовать спрайт, на основе данных в структуре.
};


// -------- API спрайта: --------


// Создать спрайт:
Sprite2D* Sprite2D_create(
    Renderer *renderer, Texture *texture,
    float x, float y, float width, float height,
    float angle, Vec4f color, bool custom_shader
);

// Уничтожить спрайт:
void Sprite2D_destroy(Sprite2D **sprite);

// Отрисовать 2D спрайт (без создания экземпляра):
void Sprite2D_render(
    Renderer *renderer, Texture *texture,
    float x, float y, float width, float height,
    float angle, Vec4f color, bool custom_shader
);
