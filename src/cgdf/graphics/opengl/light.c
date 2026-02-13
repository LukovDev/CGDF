//
// light.c - Реализация простого освещения в OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include "../renderer.h"
#include "../shader.h"
#include "../texture.h"
#include "../light.h"
#include "buffers/buffers.h"
#include "gl.h"


// Структура простого освещения в 2D:
struct Light2D {
    Renderer  *renderer;     // Рендерер.
    Vec3f     ambient;       // Фоновое освещение.
    float     intensity;     // Интенсивность освещения.
    Texture   *albedo_tex;   // Текстура окружения.
    Texture   *light_tex;    // Текстура освещения.
    BufferFBO *framebuffer;  // Буфер кадра.
    bool _is_scene_begin_;   // Для конструкции Light2D_scene_begin/Light2D_scene_end.
    bool _is_light_begin_;   // Для конструкции Light2D_light_begin/Light2D_light_end.
};


// Перечисление видов текстур в кадровом буфере:
typedef enum {
    LIGHT2D_TEXTURE_ALBEDO = 0,
    LIGHT2D_TEXTURE_LIGHT,
    LIGHT2D_TEXTURE_COUNT
} Light2DTextureType;


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
    light->albedo_tex = Texture_create(renderer);
    light->light_tex = Texture_create(renderer);
    light->framebuffer = BufferFBO_create(width, height);
    light->_is_scene_begin_ = false;
    light->_is_light_begin_ = false;

    // Обновляем размеры текстур и кадрового буфера:
    Light2D_resize(light, width, height);

    // Привязываем текстуры к буферу кадра:
    BufferFBO_begin(light->framebuffer);
    BufferFBO_attach(light->framebuffer, BUFFER_FBO_COLOR, LIGHT2D_TEXTURE_ALBEDO, light->albedo_tex->id);
    BufferFBO_attach(light->framebuffer, BUFFER_FBO_COLOR, LIGHT2D_TEXTURE_LIGHT, light->light_tex->id);
    BufferFBO_apply(light->framebuffer);
    BufferFBO_end(light->framebuffer);
    return light;
}

// Уничтожить простое 2D освещение:
void Light2D_destroy(Light2D **light) {
    if (!light || !*light) return;

    // Восстанавливаем состояние в случае если проходы рендеринга активны:
    if ((*light)->_is_scene_begin_ || (*light)->_is_light_begin_) {
        BufferFBO_end((*light)->framebuffer);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Режим перекрытия цвета (для обычного рендеринга).
    }

    // Удаляем:
    Texture_destroy(&(*light)->albedo_tex);
    Texture_destroy(&(*light)->light_tex);
    BufferFBO_destroy(&(*light)->framebuffer);

    mm_free(*light);
    *light = NULL;
}

// Начать захватывать отрисовку сцены:
void Light2D_scene_begin(Light2D *self) {
    if (!self || self->_is_scene_begin_) return;
    if (!self->_is_light_begin_) BufferFBO_begin(self->framebuffer);

    // Активируем текстуру для записи в неё:
    BufferFBO_active(self->framebuffer, LIGHT2D_TEXTURE_ALBEDO);
    self->_is_scene_begin_ = true;
}

// Закончить захватывать отрисовку сцены:
void Light2D_scene_end(Light2D *self) {
    if (!self || !self->_is_scene_begin_) return;
    self->_is_scene_begin_ = false;
    if (!self->_is_light_begin_) BufferFBO_end(self->framebuffer);
}

// Начать захватывать отрисовку света:
void Light2D_light_begin(Light2D *self) {
    if (!self || self->_is_light_begin_) return;
    if (!self->_is_scene_begin_) BufferFBO_begin(self->framebuffer);

    // Активируем текстуру для записи в неё:
    BufferFBO_active(self->framebuffer, LIGHT2D_TEXTURE_LIGHT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Режим накапливания цвета (для света).
    self->_is_light_begin_ = true;
}

// Закончить захватывать отрисовку света:
void Light2D_light_end(Light2D *self) {
    if (!self || !self->_is_light_begin_) return;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Режим перекрытия цвета (для обычного рендеринга).
    self->_is_light_begin_ = false;
    if (!self->_is_scene_begin_) BufferFBO_end(self->framebuffer);
}

// Отрисовать освещение (композит двух проходов отрисовки):
void Light2D_render(Light2D *self) {
    if (!self || !self->renderer || self->_is_scene_begin_ || self->_is_light_begin_) return;
    Renderer *rnd = self->renderer;
    Vec2f resolution = (Vec2f){Renderer_get_width(rnd), Renderer_get_height(rnd)};
    Shader_begin(rnd->shader_light2d);
    Shader_set_tex2d(rnd->shader_light2d, "u_albedo_texture", self->albedo_tex->id);  // Текстура окружения.
    Shader_set_tex2d(rnd->shader_light2d, "u_light_texture", self->light_tex->id);    // Текстура освещения.
    Shader_set_vec3(rnd->shader_light2d, "u_ambient", self->ambient);       // Фоновое освещение.
    Shader_set_float(rnd->shader_light2d, "u_intensity", self->intensity);  // Яркость всего света.
    Shader_set_vec2(rnd->shader_light2d, "u_resolution", resolution);       // Размер экрана.
    Mesh_render(rnd->sprite_mesh, false);
    Shader_end(rnd->shader_light2d);

    // Очищаем текстуры:
    BufferFBO_begin(self->framebuffer);
    BufferFBO_apply(self->framebuffer);
    BufferFBO_clear(self->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);
    BufferFBO_end(self->framebuffer);
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
    Texture_empty(self->albedo_tex, width, height, false, TEX_RGBA16F, TEX_DATA_UBYTE);
    Texture_empty(self->light_tex, width, height, false, TEX_RGBA16F, TEX_DATA_UBYTE);
    BufferFBO_resize(self->framebuffer, width, height);
}
