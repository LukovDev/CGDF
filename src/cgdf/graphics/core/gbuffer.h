//
// gbuffer.h - Определяем функционал для работы с G-Buffer.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include "texture.h"


// Объявление структур:
typedef struct Renderer Renderer;
typedef struct GBuffer GBuffer;  // G-Buffer.

// (Определение структуры находится в реализации).


// -------- API G-Buffer: --------


// Создать G-Buffer:
GBuffer* GBuffer_create(Renderer *renderer, int width, int height);

// Уничтожить G-Buffer:
void GBuffer_destroy(GBuffer **gbuffer);

// Начать рендер G-Buffer:
void GBuffer_begin(GBuffer *self);

// Закончить рендер G-Buffer:
void GBuffer_end(GBuffer *self);

// Масштабировать G-Buffer:
void GBuffer_resize(GBuffer *self, int width, int height);

// Получить текстуру albedo_roughness:
Texture* GBuffer_get_tex_albedo_roughness(GBuffer *self);

// Получить текстуру normal_ao:
Texture* GBuffer_get_tex_normal_ao(GBuffer *self);

// Получить текстуру pbr_properties:
Texture* GBuffer_get_tex_pbr_properties(GBuffer *self);

// Получить текстуру emissive:
Texture* GBuffer_get_tex_emissive(GBuffer *self);

// Получить текстуру depth:
Texture* GBuffer_get_tex_depth(GBuffer *self);
