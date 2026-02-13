//
// light.c - Реализация простого освещения в OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include "../batch.h"
#include "../renderer.h"
#include "../shader.h"
#include "../texture.h"
#include "../light.h"
#include "buffers/buffers.h"
#include "gl.h"


// Структура простого освещения в 2D:
struct Light2D {
    Renderer    *renderer;     // Рендерер.
    Vec3f       ambient;       // Фоновое освещение.
    float       intensity;     // Интенсивность освещения.
    Array       *slights;      // Массив спрайтовых источников света.
    SpriteBatch *batch;        // Пакетная отрисовка спрайтов.
    Texture     *albedo_tex;   // Текстура окружения.
    Texture     *light_tex;    // Текстура освещения.
    BufferFBO   *framebuffer;  // Буфер кадра.
    bool        _is_begin_;
};


// -------- API простого 2D освещения: --------


// Создать простое 2D освещение:
Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity) {
    if (!renderer) return NULL;
    Light2D *light = (Light2D*)mm_alloc(sizeof(Light2D));

    int width = Renderer_get_width(renderer);
    int height = Renderer_get_height(renderer);

    // Заполняем поля:
    light->renderer = renderer;
    light->ambient = ambient;
    light->intensity = intensity;
    light->slights = Array_create(sizeof(void*), ARRAY_DEFAULT_CAPACITY);  // Массив указателей.
    light->batch = SpriteBatch_create(renderer);
    light->albedo_tex = Texture_create(renderer);
    light->light_tex = Texture_create(renderer);
    light->framebuffer = BufferFBO_create(width, height);
    light->_is_begin_ = false;

    // Обновляем размеры текстур и кадрового буфера:
    Light2D_resize(light, width, height);
    return light;
}

// Уничтожить простое 2D освещение:
void Light2D_destroy(Light2D **light) {
    if (!light || !*light) return;

    // Указываем у всех источников света, что освещение уничтожено:
    // Для спрайтовых источников света:
    for (size_t i = 0; i < Array_len((*light)->slights); i++) {
        SpriteLight2D *slight = (SpriteLight2D*)Array_get_ptr((*light)->slights, i);
        if (slight) slight->light = NULL;
    }
    // ...

    // Удаляем прочее:
    Array_destroy(&(*light)->slights);
    SpriteBatch_destroy(&(*light)->batch);
    Texture_destroy(&(*light)->albedo_tex);
    Texture_destroy(&(*light)->light_tex);
    BufferFBO_destroy(&(*light)->framebuffer);

    mm_free(*light);
    *light = NULL;
}

// Начать захватывать отрисовку сцены:
void Light2D_begin(Light2D *self) {
    if (!self || self->_is_begin_) return;

    // Привязываем кадровый буфер:
    BufferFBO_begin(self->framebuffer);

    // Привязываем текстуру окружения и очищаем её:
    BufferFBO_attach(self->framebuffer, BUFFER_FBO_COLOR, 0, self->albedo_tex->id);
    BufferFBO_apply(self->framebuffer);
    BufferFBO_clear(self->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);
    self->_is_begin_ = true;
}

// Закончить захватывать отрисовку сцены:
void Light2D_end(Light2D *self) {
    if (!self || !self->_is_begin_) return;
    self->_is_begin_ = false;

    Renderer *rnd = self->renderer;
    if (!rnd) return;

    Vec2f resolution = (Vec2f){Renderer_get_width(rnd), Renderer_get_height(rnd)};

    // Привязываем текстуру освещения и очищаем её:
    BufferFBO_attach(self->framebuffer, BUFFER_FBO_COLOR, 0, self->light_tex->id);
    BufferFBO_apply(self->framebuffer);
    BufferFBO_clear(self->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);

    // Рисуем источники света:

    // Спрайты:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Режим накапливания цвета (для света).
    SpriteBatch_begin(self->batch);
    for (size_t i=0; i < Array_len(self->slights); i++) {
        SpriteLight2D *slight = (SpriteLight2D*)Array_get_ptr(self->slights, i);
        SpriteBatch_set_color(self->batch, slight->color);
        SpriteBatch_draw(self->batch,
            slight->texture,
            slight->position.x, slight->position.y,
            slight->size.x, slight->size.y,
            slight->angle
        );
    }
    SpriteBatch_end(self->batch);

    // Другие виды источников света:
    // ...

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Режим перекрытия цвета (для обычного рендеринга).

    // Отвязываем кадровый буфер:
    BufferFBO_end(self->framebuffer);

    // Рисуем освещение:
    Shader_begin(rnd->shader_light2d);
    Shader_set_tex2d(rnd->shader_light2d, "u_albedo_texture", self->albedo_tex->id);  // Текстура окружения.
    Shader_set_tex2d(rnd->shader_light2d, "u_light_texture",   self->light_tex->id);  // Текстура освещения.
    Shader_set_vec3(rnd->shader_light2d, "u_ambient", self->ambient);       // Фоновое освещение.
    Shader_set_float(rnd->shader_light2d, "u_intensity", self->intensity);  // Яркость всего света.
    Shader_set_vec2(rnd->shader_light2d, "u_resolution", resolution);       // Размер экрана.
    Mesh_render(rnd->sprite_mesh, false);
    Shader_end(rnd->shader_light2d);
}

// Установить фоновый цвет 2D освещения:
void Light2D_set_ambient(Light2D *self, Vec3f ambient) {
    if (!self) return;
    self->ambient = ambient;
}

// Установить интенсивность 2D освещения:
void Light2D_set_intensity(Light2D *self, float intensity) {
    if (!self) return;
    self->intensity = intensity;
}

// Изменить размер 2D освещения:
void Light2D_resize(Light2D *self, int width, int height) {
    if (!self) return;
    Texture_empty(self->albedo_tex, width, height, false, TEX_RGBA8, TEX_DATA_UBYTE);
    Texture_empty(self->light_tex, width, height, false, TEX_RGBA8, TEX_DATA_UBYTE);
    BufferFBO_resize(self->framebuffer, width, height);
}


// -------- API спрайтового 2D освещения: --------


// Создать точечное 2D освещение:
SpriteLight2D* SpriteLight2D_create(
    Light2D *light, Texture *texture,
    Vec2f position, Vec2f size,
    float angle, Vec4f color
) {
    if (!light) return NULL;
    SpriteLight2D *slight = (SpriteLight2D*)mm_alloc(sizeof(SpriteLight2D));

    // Заполняем поля:
    slight->id = 0;  // Пока что 0.
    slight->light = light;
    slight->position = position;
    slight->texture = texture;
    slight->size = size;
    slight->angle = angle;
    slight->color = color;

    // Добавляем этот источник света в массив источников света:
    size_t len = Array_len(slight->light->slights);  // Количество источников света в массиве.
    slight->id = len;  // Теперь айди будет концом массива источников света.
    Array_push(slight->light->slights, &slight);  // Добавляем в конец массива.
    return slight;
}

// Уничтожить точечное 2D освещение:
void SpriteLight2D_destroy(SpriteLight2D **spritelight) {
    if (!spritelight || !*spritelight) return;

    // Удаление источника света из массива источников света:
    if ((*spritelight)->light && (*spritelight)->light->slights) {
        // Для того, чтобы не перебирать и не искать этот источник света в этом массиве,
        // мы используем идентификаторы источников света. Достаточно удалить из массива по идентификатору.
        size_t id = (*spritelight)->id;  // Сохраняем текущий айди для источника света из конца.
        size_t len = Array_len((*spritelight)->light->slights);  // Длина массива на данный момент.

        // Если айди этого источника света находится в массиве:
        if (id < len) {
            // Чтобы не сдвигать все последующие источники света влево, мы удаляем и заменяем последним из массива:
            Array_remove_swap((*spritelight)->light->slights, id, NULL);

            // Теперь в массиве по нашему айди должен находиться другой источник света, либо ничего. Получаем его:
            SpriteLight2D *another_slight = (SpriteLight2D*)Array_get_ptr((*spritelight)->light->slights, id);

            // Если же на этом месте есть другой источник света, то обновляем его айди на этот, для корректности:
            if (another_slight) another_slight->id = id;
        }
    }

    mm_free(*spritelight);
    *spritelight = NULL;
}
