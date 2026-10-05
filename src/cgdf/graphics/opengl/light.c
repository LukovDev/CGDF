//
// light.c - Реализация освещения в OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include "../core/camera.h"
#include "../core/renderer.h"
#include "../core/shader.h"
#include "../core/texture.h"
#include "../core/light.h"
#include "buffers/buffers.h"
#include "gl.h"


// Структура освещения в 2D:
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


// Структура освещения в 3D:
struct Light3D {
    Renderer  *renderer;      // Рендерер.
    BufferFBO *framebuffer;   // Буфер кадра освещения.
    Texture   *light_tex;     // Результат освещения (HDR, RGBA16F).
    Vec3f sun_direction;      // Направление солнца.
    Vec3f sun_color;          // Цвет солнца.
    float sun_intensity;      // Интенсивность солнца.
    Vec3f ambient_color;      // Цвет фонового освещения.
    float ambient_intensity;  // Интенсивность фонового освещения.
};


// Перечисление видов текстур в кадровом буфере:
typedef enum {
    LIGHT2D_TEXTURE_ALBEDO = 0,
    LIGHT2D_TEXTURE_LIGHT,
    LIGHT2D_TEXTURE_COUNT
} Light2DTextureType;


// Перечисление видов текстур в кадровом буфере:
typedef enum {
    LIGHT3D_TEXTURE_LIGHT = 0,
    LIGHT3D_TEXTURE_COUNT
} Light3DTextureType;


// -------- API 2D освещения: --------


// Создать 2D освещение:
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
    light->framebuffer = BufferFBO_create();
    light->_is_scene_begin_ = false;
    light->_is_light_begin_ = false;

    // Обновляем размеры текстур кадрового буфера:
    Light2D_resize(light, width, height);

    // Привязываем текстуры к буферу кадра:
    BufferFBO_begin(light->framebuffer);
    BufferFBO_attach(light->framebuffer, BUFFER_FBO_COLOR, LIGHT2D_TEXTURE_ALBEDO, light->albedo_tex->id);
    BufferFBO_attach(light->framebuffer, BUFFER_FBO_COLOR, LIGHT2D_TEXTURE_LIGHT, light->light_tex->id);
    BufferFBO_apply(light->framebuffer);
    BufferFBO_end(light->framebuffer);
    return light;
}

// Уничтожить 2D освещение:
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

// Изменить размер текстур 2D освещения:
void Light2D_resize(Light2D *self, int width, int height) {
    if (!self) return;
    Texture_empty(self->albedo_tex, width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
    Texture_empty(self->light_tex, width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
}


// -------- API 3D освещения: --------


// Создать 3D освещение:
Light3D* Light3D_create(Renderer *renderer) {
    Light3D *light = (Light3D*)mm_alloc(sizeof(Light3D));

    // Заполняем поля:
    light->renderer = renderer;
    light->framebuffer = BufferFBO_create();
    light->light_tex = Texture_create(renderer);
    light->sun_direction = (Vec3f){-0.57735, -0.57735, -0.57735};
    light->sun_color = (Vec3f){1.0f, 0.96f, 0.9f};
    light->sun_intensity = 3.0f;
    light->ambient_color = (Vec3f){0.6f, 0.7f, 1.0f};
    light->ambient_intensity = 0.15f;

    // Обновляем размеры текстур кадрового буфера:
    Light3D_resize(light, Renderer_get_width(renderer), Renderer_get_height(renderer));

    // Привязываем текстуры к буферу кадра:
    BufferFBO_begin(light->framebuffer);
    BufferFBO_attach(light->framebuffer, BUFFER_FBO_COLOR, LIGHT3D_TEXTURE_LIGHT, light->light_tex->id);
    BufferFBO_apply(light->framebuffer);
    BufferFBO_end(light->framebuffer);
    return light;
}

// Уничтожить 3D освещение:
void Light3D_destroy(Light3D **light) {
    if (!light || !*light) return;

    // Восстанавливаем состояние в случае если проходы рендеринга активны:
    BufferFBO_end((*light)->framebuffer);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Режим перекрытия цвета (для обычного рендеринга).

    // Удаляем:
    Texture_destroy(&(*light)->light_tex);
    BufferFBO_destroy(&(*light)->framebuffer);

    mm_free(*light);
    *light = NULL;
}

// Изменить размер текстур 3D освещения:
void Light3D_resize(Light3D *self, int width, int height) {
    if (!self) return;
    Texture_empty(self->light_tex, width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
}

// Получить текстуру 3D освещения:
Texture* Light3D_get_light_tex(Light3D *self) {
    if (!self) return NULL;
    return self->light_tex;
}

// Установить направление солнца:
void Light3D_set_sun_dir(Light3D *self, Vec3f direction) {
    if (!self) return;
    self->sun_direction = direction;
}

// Получить направление солнца:
Vec3f Light3D_get_sun_dir(Light3D *self) {
    if (!self) return (Vec3f){0};
    return self->sun_direction;
}

// Установить цвет солнца:
void Light3D_set_sun_color(Light3D *self, Vec3f color) {
    if (!self) return;
    self->sun_color = color;
}

// Получить цвет солнца:
Vec3f Light3D_get_sun_color(Light3D *self) {
    if (!self) return (Vec3f){0};
    return self->sun_color;
}

// Установить интенсивность солнца:
void Light3D_set_sun_intensity(Light3D *self, float intensity) {
    if (!self) return;
    self->sun_intensity = intensity;
}

// Получить интенсивность солнца:
float Light3D_get_sun_intensity(Light3D *self) {
    if (!self) return 0.0f;
    return self->sun_intensity;
}

// Установить цвет фонового освещения:
void Light3D_set_ambient_color(Light3D *self, Vec3f color) {
    if (!self) return;
    self->ambient_color = color;
}

// Получить цвет фонового освещения:
Vec3f Light3D_get_ambient_color(Light3D *self) {
    if (!self) return (Vec3f){0};
    return self->ambient_color;
}

// Установить интенсивность фонового освещения:
void Light3D_set_ambient_intensity(Light3D *self, float intensity) {
    if (!self) return;
    self->ambient_intensity = intensity;
}

// Получить интенсивность фонового освещения:
float Light3D_get_ambient_intensity(Light3D *self) {
    if (!self) return 0.0f;
    return self->ambient_intensity;
}

// Отрисовать освещение:
void Light3D_render(Light3D *self, mat4 proj, mat4 view) {
    if (!self || !self->renderer || !self->renderer->camera) return;
    Renderer *rnd = self->renderer;

    Vec3f camera_pos = {0};
    if (rnd->camera_type == RENDERER_CAMERA_2D) {
        Vec2d pos = ((Camera2D*)rnd->camera)->position;
        camera_pos = (Vec3f){pos.x, pos.y, 0.0f};
    } else {
        Vec3d pos = ((Camera3D*)rnd->camera)->position;
        camera_pos = (Vec3f){pos.x, pos.y, pos.z};
    }

    // Обратная матрица (проекция * вид):
    mat4 view_proj, inv_view_proj;
    glm_mat4_mul(proj, view, view_proj);
    glm_mat4_inv(view_proj, inv_view_proj);

    // Включаем и очищаем буфер кадра:
    BufferFBO_begin(self->framebuffer);
    BufferFBO_apply(self->framebuffer);
    BufferFBO_clear(self->framebuffer, 0.0f, 0.0f, 0.0f, 0.0f);

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(rnd, false);
    Renderer_set_blending(rnd, false);
    Renderer_set_cull_mode(rnd, RENDERER_CULL_NONE);

    Shader *sh = rnd->shader_lightning;
    Shader_begin(sh);
    Shader_set_tex2d(sh, "u_albedo_roughness",  GBuffer_get_tex_albedo_roughness(rnd->gbuffer)->id);
    Shader_set_tex2d(sh, "u_normal_ao",         GBuffer_get_tex_normal_ao(rnd->gbuffer)->id);
    Shader_set_tex2d(sh, "u_pbr",               GBuffer_get_tex_pbr_properties(rnd->gbuffer)->id);
    Shader_set_tex2d(sh, "u_emissive",          GBuffer_get_tex_emissive(rnd->gbuffer)->id);
    Shader_set_tex2d(sh, "u_depth",             GBuffer_get_tex_depth(rnd->gbuffer)->id);
    Shader_set_mat4(sh,  "u_inv_view_proj",     inv_view_proj);
    Shader_set_vec3(sh,  "u_camera_pos",        camera_pos);
    Shader_set_vec3(sh,  "u_sun_dir",           self->sun_direction);
    Shader_set_vec3(sh,  "u_sun_color",         self->sun_color);
    Shader_set_float(sh, "u_sun_intensity",     self->sun_intensity);
    Shader_set_vec3(sh,  "u_ambient_color",     self->ambient_color);
    Shader_set_float(sh, "u_ambient_intensity", self->ambient_intensity);
    Mesh_render(rnd->sprite_mesh, false);
    Shader_end(sh);
    BufferFBO_end(self->framebuffer);
}
