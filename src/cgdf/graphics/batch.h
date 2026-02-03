//
// batch.h - Определяет API для пакетной отрисовки.
//
// Пакетная отрисовка 2D спрайтов работает концептуально также, как в LibGDX.
//
// ВНИМАНИЕ: Старайтесь сувать в пакетную отрисовку как можно меньше разных текстур спрайтов!
// Это влияет на производительность и эффективность пакетной отрисовки.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "texture.h"
#include "renderer.h"


// Определения:

// Малое количество спрайтов на пакет, уменьшит производительность из за частых отрисовок.
// Среднее количество спрайтов на пакет, увеличит производительность, но главное не переборщить (от 1024 до 2048/4096).
// Большое количество спрайтов на пакет, уменьшит производительность из за больших объёмов данных.
#define BATCH_MAX_SPRITES      2048  // Максимальное количество спрайтов в пакете. Рекомендуемый диапазон: 1024 до 4096.
#define BATCH_VERTS_PER_SPRITE 6     // 2 треугольника в спрайте.
#define BATCH_PARAMS_IN_VERTEX 8     // Количество параметров в одной вершине.


// Объявление структур:
typedef struct SpriteBatch SpriteBatch;          // Пакетная отрисовка спрайтов.
typedef struct SpriteVertex SpriteVertex;        // Вершина спрайта.


// Пакетная отрисовка спрайтов:
// (Определение структуры находится в реализации).


// Вершина спрайта:
struct SpriteVertex {
    float x, y;
    float u, v;
    union {
        float r, g, b, a;
        Vec4f color;
    };
};


// -------- API пакетной отрисовки: --------


// Создать пакетную отрисовку спрайтов:
SpriteBatch* SpriteBatch_create(Renderer *renderer);

// Уничтожить пакетную отрисовку спрайтов:
void SpriteBatch_destroy(SpriteBatch **batch);

// Начать отрисовку:
void SpriteBatch_begin(SpriteBatch *batch);

// Установить цвет следующим спрайтам:
void SpriteBatch_set_color(SpriteBatch *batch, Vec4f color);

// Получить установленный цвет:
Vec4f SpriteBatch_get_color(SpriteBatch *batch);

// Установить текстурные координаты следующим спрайтам:
void SpriteBatch_set_texcoord(SpriteBatch *batch, Vec4f texcoord);

// Сбросить текстурные координаты:
void SpriteBatch_reset_texcoord(SpriteBatch *batch);

// Получить текстурные координаты:
Vec4f SpriteBatch_get_texcoord(SpriteBatch *batch);

// Добавить спрайт в пакет данных:
void SpriteBatch_draw(SpriteBatch *batch, Texture *texture, float x, float y, float width, float height, float angle);

// Закончить отрисовку:
void SpriteBatch_end(SpriteBatch *batch);
