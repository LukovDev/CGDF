//
// batch.h - Определяет API для пакетной отрисовки.
//
// Пакетная отрисовка 2D спрайтов работает концептуально также, как в LibGDX.
//
// ВНИМАНИЕ: Старайтесь сувать в пакетную отрисовку как можно меньше разных текстур спрайтов!
// При смене текстуры, происходит отрисовка пакета. Чем чаще сменяются текстуры, тем больше
// вызовов отрисовки. В случае переполнения буфера пакета, он отрисовывается и сбрасывается.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "texture.h"
#include "renderer.h"


// Определения:

// BATCH_MAX_SPRITES:
// Малое количество спрайтов на пакет, уменьшит производительность из за частых отрисовок.
// Среднее количество спрайтов на пакет, увеличит производительность, но главное не переборщить (от 1024 до 2048/4096).
// Большое количество спрайтов на пакет, уменьшит производительность из за больших объёмов данных.
extern uint32_t BATCH_SPRITES_SIZE;  // Глобальная переменная (редактируется. По умолчанию равно BATCH_MAX_SPRITES).
#define BATCH_MAX_SPRITES      2048  // Объём буфера пакета в спрайтах (константа).
#define BATCH_VERTS_PER_SPRITE 4     // Квадрат из 4 вершин.
#define BATCH_INDCS_PER_SPRITE 6     // 6 индексов для создания спрайта.
#define BATCH_PARAMS_IN_VERTEX 8     // Количество параметров в одной вершине.


// Объявление структур:
typedef struct SpriteBatch SpriteBatch;    // Пакетная отрисовка спрайтов.
typedef struct SpriteVertex SpriteVertex;  // Вершина спрайта.


// Пакетная отрисовка спрайтов:
// (Определение структуры находится в реализации).


// Вершина спрайта:
struct SpriteVertex {
    float x, y;            // Позиция вершины       (loc=0, vec2 a_position).
    float u, v;            // Текстурные координаты (loc=1, vec2 a_texcoord).
    union {                // Цвет вершины          (loc=2, vec4 a_color).
        float r, g, b, a;  // Канал цвета.
        Vec4f color;       // Цвет.
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
