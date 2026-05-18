//
// gbuffer.c - Реализуем функционал для работы с G-Buffer.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../core/renderer.h"
#include "../core/texture.h"
#include "../core/gbuffer.h"
#include "buffers/buffers.h"


// G-Buffer:
struct GBuffer {
    int width, height;          // Размер окна.
    Renderer *renderer;         // Рендерер.
    Texture *albedo_roughness;  // Цвет (RGB) + Шероховатость (A). [RGBA8].
    Texture *normal_ao;         // Нормаль (RGB) + Окклюзия (A). [RGBA16F].
    Texture *pbr_properties;    // Металличность (R) + Карта высоты (G) + Аберрация (B) + Искажение (A). [RGBA16F].
    Texture *emissive;          // Свечение (RGB) + Свободно (A). [RGBA16F].
    Texture *depth;             // Глубина. [DEPTH32F].
    BufferFBO *fbo;             // Буфер кадра.
    bool _is_begin_;            // Флаг использования буфера (внутренняя логика).
};


// Перечисление видов текстур в кадровом буфере:
typedef enum {
    GBUFFER_TEXTURE_ALBEDO_ROUGHNESS = 0,
    GBUFFER_TEXTURE_NORMAL_AO,
    GBUFFER_TEXTURE_PBR_PROPERTIES,
    GBUFFER_TEXTURE_EMISSIVE,
    GBUFFER_TEXTURE_DEPTH,
    GBUFFER_TEXTURE_COUNT,
} GBufferTextureType;


// Создать G-Buffer:
GBuffer* GBuffer_create(Renderer *renderer, int width, int height) {
    GBuffer *gbuffer = (GBuffer*)mm_alloc(sizeof(GBuffer));

    // Заполняем поля:
    gbuffer->renderer = renderer;
    gbuffer->albedo_roughness = Texture_create(renderer);
    gbuffer->normal_ao = Texture_create(renderer);
    gbuffer->pbr_properties = Texture_create(renderer);
    gbuffer->emissive = Texture_create(renderer);
    gbuffer->depth = Texture_create(renderer);
    gbuffer->fbo = BufferFBO_create();
    gbuffer->_is_begin_ = false;
    GBuffer_resize(gbuffer, width, height);
    return gbuffer;
}

// Уничтожить G-Buffer:
void GBuffer_destroy(GBuffer **gbuffer) {
    if (!gbuffer || !*gbuffer) return;

    // Удаляем текстуры G-Buffer:
    if ((*gbuffer)->albedo_roughness) Texture_destroy(&(*gbuffer)->albedo_roughness);
    if ((*gbuffer)->normal_ao)        Texture_destroy(&(*gbuffer)->normal_ao);
    if ((*gbuffer)->pbr_properties)   Texture_destroy(&(*gbuffer)->pbr_properties);
    if ((*gbuffer)->emissive)         Texture_destroy(&(*gbuffer)->emissive);
    if ((*gbuffer)->depth)            Texture_destroy(&(*gbuffer)->depth);
    if ((*gbuffer)->fbo)              BufferFBO_destroy(&(*gbuffer)->fbo);

    mm_free(*gbuffer);
    *gbuffer = NULL;
}


// -------- API G-Buffer: --------


// Начать рендер G-Buffer:
void GBuffer_begin(GBuffer *self) {
    if (!self || self->_is_begin_ || !self->fbo) return;
    BufferFBO_begin(self->fbo);
    BufferFBO_apply(self->fbo);
    BufferFBO_clear(self->fbo, 0.0f, 0.0f, 0.0f, 0.0f);
    self->_is_begin_ = true;
}

// Закончить рендер G-Buffer:
void GBuffer_end(GBuffer *self) {
    if (!self || !self->_is_begin_) return;
    BufferFBO_end(self->fbo);
    self->_is_begin_ = false;
}

// Масштабировать G-Buffer:
void GBuffer_resize(GBuffer *self, int width, int height) {
    if (!self) return;
    self->width = width;
    self->height = height;
    Texture_empty(self->albedo_roughness, width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA8,   TEX_DATA_UBYTE);
    Texture_empty(self->normal_ao,        width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
    Texture_empty(self->pbr_properties,   width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
    Texture_empty(self->emissive,         width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
    Texture_empty(self->depth, width, height, false, TEX_FORMAT_DEPTH, TEX_INTERNAL_DEPTH32F, TEX_DATA_FLOAT);
    BufferFBO_begin(self->fbo);
    BufferFBO_attach(self->fbo, BUFFER_FBO_COLOR, GBUFFER_TEXTURE_ALBEDO_ROUGHNESS, self->albedo_roughness->id);
    BufferFBO_attach(self->fbo, BUFFER_FBO_COLOR, GBUFFER_TEXTURE_NORMAL_AO, self->normal_ao->id);
    BufferFBO_attach(self->fbo, BUFFER_FBO_COLOR, GBUFFER_TEXTURE_PBR_PROPERTIES, self->pbr_properties->id);
    BufferFBO_attach(self->fbo, BUFFER_FBO_COLOR, GBUFFER_TEXTURE_EMISSIVE, self->emissive->id);
    BufferFBO_attach(self->fbo, BUFFER_FBO_DEPTH, GBUFFER_TEXTURE_DEPTH, self->depth->id);
    BufferFBO_apply(self->fbo);
    BufferFBO_end(self->fbo);
}

// Получить текстуру albedo_roughness:
Texture* GBuffer_get_albedo_roughness(GBuffer *self) {
    if (!self) return NULL;
    return self->albedo_roughness;
}

// Получить текстуру normal_ao:
Texture* GBuffer_get_normal_ao(GBuffer *self) {
    if (!self) return NULL;
    return self->normal_ao;
}

// Получить текстуру pbr_properties:
Texture* GBuffer_get_pbr_properties(GBuffer *self) {
    if (!self) return NULL;
    return self->pbr_properties;
}

// Получить текстуру emissive:
Texture* GBuffer_get_emissive(GBuffer *self) {
    if (!self) return NULL;
    return self->emissive;
}

// Получить текстуру depth:
Texture* GBuffer_get_depth(GBuffer *self) {
    if (!self) return NULL;
    return self->depth;
}
