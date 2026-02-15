//
// batch.c - Реализация пакетной отрисовки для OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/mm.h>
#include "../camera.h"
#include "../shader.h"
#include "../batch.h"
#include "buffers/buffers.h"
#include "gl.h"


// Пакетная отрисовка спрайтов:
struct SpriteBatch2D {
    Renderer     *renderer;       // Рендерер.
    BufferVAO    *vao;            // Буфер атрибутов.
    BufferVBO    *vbo;            // Буфер вершин.
    BufferEBO    *ebo;            // Буфер граней.
    SpriteVertex *array;          // Массив вершин.
    uint32_t     sprite_count;    // Сколько спрайтов накоплено.
    uint32_t     vertex_count;    // Сколько вершин реально записано.
    uint32_t     current_tex_id;  // Текущий айди текстуры.
    Vec4f        color;           // Цвета спрайтов.
    Vec4f        texcoord;        // Текстурные координаты спрайтов.
    bool         _is_begin_;      // Внутренний флаг-ключ отрисовки.
};


// Глобальные переменные:
uint32_t BATCH_SPRITES_SIZE = BATCH_MAX_SPRITES;


// -------- Вспомогательные функции: --------


static uint32_t* _create_indices_buffer_(uint32_t size, uint32_t batch_size) {
    /*
        Создаём буфер максимального размера, чтобы из 4 вершины спрайта можно было рисовать 2 треугольника.
        Мы просто дублируем pattern для каждого спрайта на весь буфер. Благодаря этому, мы используем весь
        буфер только один раз и больше не будем в нём менять память. Это производительно.
    */
    uint32_t *indices = (uint32_t*)mm_alloc(size);
    static const uint32_t pattern[6] = {0, 1, 2, 2, 3, 0};
    // Проходимся по спрайтам в буфере:
    for (uint32_t i = 0; i < batch_size; i++) {
        uint32_t *dst = indices + i * BATCH_INDCS_PER_SPRITE;  // Определяем сдвиг для записи.
        // Проходимся по индексам в паттерне и записываем их в буфер:
        for (uint32_t j = 0; j < BATCH_INDCS_PER_SPRITE; j++) {
            dst[j] = i * BATCH_VERTS_PER_SPRITE + pattern[j];
        }
    }
    return indices;  // Не забудьте освободить память!
}


// Отрисовать буфер спрайтов:
static void _batch_flush_(SpriteBatch2D *self) {
    if (!self) return;
    if (!self->_is_begin_ || self->vertex_count == 0) {
        // Экономим вызовы отрисовки (ничего не рисуем) если в буфере нет вершин.
        self->sprite_count = 0;
        self->vertex_count = 0;
        return;
    }

    // Обновляем в шейдере текстуру (предположительно, шейдер уже должен быть активен после вызова SpriteBatch2D_begin):
    Shader *shader = self->renderer->shader_spritebatch2d;
    Shader_set_bool(shader, "u_use_texture", self->current_tex_id != 0 ? true : false);
    Shader_set_tex2d(shader, "u_texture", self->current_tex_id);

    // Обновляем данные в буфере сетки спрайтов (буфер VBO должен быть активен):
    BufferVBO_set_subdata(self->vbo, self->array, 0, self->vertex_count * sizeof(SpriteVertex));

    // Рисуем (буфер VAO должен быть активен):
    glDrawElements(GL_TRIANGLES, BATCH_INDCS_PER_SPRITE * self->sprite_count, GL_UNSIGNED_INT, 0);

    // Сбрасываем данные:
    self->sprite_count = 0;
    self->vertex_count = 0;
}


// -------- API пакетной отрисовки: --------


// Создать пакетную отрисовку спрайтов:
SpriteBatch2D* SpriteBatch2D_create(Renderer *renderer) {
    if (!renderer) return NULL;
    SpriteBatch2D *batch = (SpriteBatch2D*)mm_alloc(sizeof(SpriteBatch2D));

    // Размер одной вершины в байтах (8 параметров * 4 байта по каждому = 32 байт):
    size_t stride = sizeof(SpriteVertex);

    // Размер буфера спрайтов в байтах:
    // Размер одной вершины в байтах (32) * 4 вершины спрайта * максимальное количество спрайтов (2048).
    // Если пакет равен 2048 спрайтам, то размер буфера: 32 * 4 * 2048 = 262144 байт (256 кб) в озу и в видеопамяти.
    size_t size = stride * BATCH_VERTS_PER_SPRITE * BATCH_SPRITES_SIZE;

    // Размер буфера индексов в байтах:
    // Размер одного индекса (4 байта) * количество индексов на спрайт (6) * размер пакета спрайтов (2048).
    // Если пакет равен 2048 спрайтам, то размер буфера: 4 * 6 * 2048 = 49152 байт (48 кб) в озу и в видеопамяти.
    size_t size_indices = sizeof(uint32_t) * BATCH_INDCS_PER_SPRITE * BATCH_SPRITES_SIZE;

    // Заполняем поля:
    batch->renderer = renderer;
    batch->vao = BufferVAO_create();
    batch->vbo = BufferVBO_create(NULL, size, GL_DYNAMIC_DRAW);
    // batch->ebo настраивается ниже.
    batch->array = (SpriteVertex*)mm_alloc(size);
    batch->sprite_count = 0;
    batch->vertex_count = 0;
    batch->current_tex_id = 0;
    batch->color = (Vec4f){1.0f, 1.0f, 1.0f, 1.0f};
    batch->texcoord = (Vec4f){0.0f, 0.0f, 1.0f, 1.0f};
    batch->_is_begin_ = false;

    // Создаём массив индексов, чтобы из 4 вершины спрайта можно было рисовать 2 треугольника:
    uint32_t *indices = _create_indices_buffer_(size_indices, BATCH_SPRITES_SIZE);
    batch->ebo = BufferEBO_create(indices, size_indices, GL_STATIC_DRAW);
    mm_free(indices);  // Освобождаем массив индексов.
    // Больше трогать память ebo буфера не нужно. Мы заполнили его полностью.

    // Настраиваем буфер:
    BufferVAO_begin(batch->vao);
    BufferEBO_begin(batch->ebo);  // Подключаем к VAO наш буфер индексов.
    BufferVBO_begin(batch->vbo);
    BufferVAO_attrib_pointer(batch->vao, 0, 2, GL_FLOAT, false, stride, offsetof(SpriteVertex, x));  // Позиция.
    BufferVAO_attrib_pointer(batch->vao, 1, 2, GL_FLOAT, false, stride, offsetof(SpriteVertex, u));  // UV.
    BufferVAO_attrib_pointer(batch->vao, 2, 4, GL_FLOAT, false, stride, offsetof(SpriteVertex, r));  // Цвет.
    BufferVBO_end(batch->vbo);
    BufferVAO_end(batch->vao);
    BufferEBO_end(batch->ebo);  // Отвязываем буфер индексов ТОЛЬКО после отвязывания VAO!
    return batch;
}

// Уничтожить пакетную отрисовку спрайтов:
void SpriteBatch2D_destroy(SpriteBatch2D **batch) {
    if (!batch || !*batch) return;

    // Удаляем буферы:
    BufferVAO_destroy(&(*batch)->vao);
    BufferVBO_destroy(&(*batch)->vbo);
    BufferEBO_destroy(&(*batch)->ebo);
    if ((*batch)->array) mm_free((*batch)->array);

    mm_free(*batch);
    *batch = NULL;
}

// Начать отрисовку:
void SpriteBatch2D_begin(SpriteBatch2D *self) {
    if (!self || self->_is_begin_) return;
    self->sprite_count = 0;
    self->vertex_count = 0;
    self->current_tex_id = 0;

    // Обновляем данные в шейдере:
    mat4 view, proj;
    Renderer_get_view_proj(self->renderer, view, proj);
    Shader *shader = self->renderer->shader_spritebatch2d;
    Shader_begin(shader);
    BufferVAO_begin(self->vao);
    BufferEBO_begin(self->ebo);  // Привязываем на всякий случай. Отвязывать не обязательно.
    BufferVBO_begin(self->vbo);
    Shader_set_mat4(shader, "u_view", view);
    Shader_set_mat4(shader, "u_proj", proj);
    self->_is_begin_ = true;
}

// Установить цвет следующим спрайтам:
void SpriteBatch2D_set_color(SpriteBatch2D *self, Vec4f color) {
    if (!self) return;
    self->color = color;
}

// Получить установленный цвет:
Vec4f SpriteBatch2D_get_color(SpriteBatch2D *self) {
    if (!self) return (Vec4f){0.0f, 0.0f, 0.0f, 0.0f};
    return self->color;
}

// Установить текстурные координаты следующим спрайтам:
void SpriteBatch2D_set_texcoord(SpriteBatch2D *self, Vec4f texcoord) {
    if (!self) return;
    self->texcoord = texcoord;
}

// Сбросить текстурные координаты:
void SpriteBatch2D_reset_texcoord(SpriteBatch2D *self) {
    if (!self) return;
    self->texcoord = (Vec4f){0.0f, 0.0f, 1.0f, 1.0f};
}

// Получить текстурные координаты:
Vec4f SpriteBatch2D_get_texcoord(SpriteBatch2D *self) {
    if (!self) return (Vec4f){0.0f, 0.0f, 1.0f, 1.0f};
    return self->texcoord;
}

// Добавить спрайт в пакет данных:
void SpriteBatch2D_draw(
    SpriteBatch2D *self, Texture *texture,
    float x, float y, float width, float height,
    float angle
) {
    if (!self || !self->_is_begin_) return;
    uint32_t tex_id = texture ? texture->id : 0;

    // Если текущая текстура не совпадает с предыдущей, то отрисовываем всё что накопили:
    if (tex_id != self->current_tex_id) {
        _batch_flush_(self);
        self->current_tex_id = tex_id;
    }

    // Если превышен лимит спрайтов, то отрисовываем все что накопили:
    if (self->sprite_count >= BATCH_SPRITES_SIZE) {
        _batch_flush_(self);
    };

    // Временные вершины (квадрат из 4 вершин по 2 координаты):
    float v[4*2];

    // Если вращаем спрайт:
    if (angle != 0.0f) {
        // Подготовка значений:
        float center_x      = x + (width  / 2.0f);
        float center_y      = y + (height / 2.0f);
        float angle_rad     = -(angle * (GLM_PIf / 180.0f));
        float angle_rad_sin = sinf(angle_rad);
        float angle_rad_cos = cosf(angle_rad);

        // Предварительные смещения:
        float dx1 = x - center_x;
        float dy1 = y - center_y;
        float dx2 = x + width - center_x;
        float dy2 = y - center_y;
        float dx3 = x + width - center_x;
        float dy3 = y + height - center_y;
        float dx4 = x - center_x;
        float dy4 = y + height - center_y;

        // Возвращаем 4 вершины спрайта:
        // Нижний левый угол:
        v[0] = dx1 * angle_rad_cos - dy1 * angle_rad_sin + center_x;
        v[1] = dx1 * angle_rad_sin + dy1 * angle_rad_cos + center_y;
        // Нижний правый угол:
        v[2] = dx2 * angle_rad_cos - dy2 * angle_rad_sin + center_x;
        v[3] = dx2 * angle_rad_sin + dy2 * angle_rad_cos + center_y;
        // Верхний правый угол:
        v[4] = dx3 * angle_rad_cos - dy3 * angle_rad_sin + center_x;
        v[5] = dx3 * angle_rad_sin + dy3 * angle_rad_cos + center_y;
        // Верхний левый угол:
        v[6] = dx4 * angle_rad_cos - dy4 * angle_rad_sin + center_x;
        v[7] = dx4 * angle_rad_sin + dy4 * angle_rad_cos + center_y;
    } else {
        // Нижний левый угол:
        v[0] = x;
        v[1] = y;
        // Нижний правый угол:
        v[2] = x + width;
        v[3] = y;
        // Верхний правый угол:
        v[4] = x + width;
        v[5] = y + height;
        // Верхний левый угол:
        v[6] = x;
        v[7] = y + height;
    }

    // Добавляем сетку в буфер:
    Vec4f tc = self->texcoord;
    uint32_t base = self->vertex_count;  // Смещение в массиве вершин.
    self->array[base+0] = (SpriteVertex){v[0], v[1], tc.x, tc.w, .color=self->color};  // 1.
    self->array[base+1] = (SpriteVertex){v[2], v[3], tc.z, tc.w, .color=self->color};  // 2.
    self->array[base+2] = (SpriteVertex){v[4], v[5], tc.z, tc.y, .color=self->color};  // 3.
    self->array[base+3] = (SpriteVertex){v[6], v[7], tc.x, tc.y, .color=self->color};  // 4.

    // Обновляем счётчики:
    self->vertex_count += 4;  // Записали 4 вершины.
    self->sprite_count += 1;  // Записали 1 спрайт.
}

// Закончить отрисовку:
void SpriteBatch2D_end(SpriteBatch2D *self) {
    if (!self || !self->_is_begin_) return;
    _batch_flush_(self);
    BufferVBO_end(self->vbo);
    BufferVAO_end(self->vao);
    Shader_end(self->renderer->shader_spritebatch2d);
    self->_is_begin_ = false;
}
