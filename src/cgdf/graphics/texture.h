//
// texture.h - Определяет функционал текстуры.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/pixmap.h>
#include "renderer.h"


// Типы формата данных текстуры и исходников:
typedef enum TextureFormat {
    TEX_RED,     TEX_RG,
    TEX_RGB,     TEX_RGBA,
    TEX_RGB16F,  TEX_RGBA16F,
    TEX_RGB32F,  TEX_RGBA32F,
    TEX_R16F,    TEX_SRGB,
    TEX_SRGBA,   TEX_BGR,
    TEX_BGRA,    TEX_RGBA8,
    TEX_DEPTH16, TEX_DEPTH24,
    TEX_DEPTH32, TEX_DEPTH32F,
    TEX_DEPTH_COMPONENT, TEX_DEPTH_STENCIL,
} TextureFormat;


// Типы данных используемой в текстуре:
typedef enum TextureDataType {
    TEX_DATA_UBYTE,  TEX_DATA_BYTE,
    TEX_DATA_USHORT, TEX_DATA_SHORT,
    TEX_DATA_UINT,   TEX_DATA_INT,
    TEX_DATA_FLOAT,
} TextureDataType;


// Типы текстур:
typedef enum TextureType {
    TEX_TYPE_2D,
    TEX_TYPE_3D,
} TextureType;


// Объявление структур:
typedef struct Texture Texture;  // Текстура 2D.


// Структура текстуры:
struct Texture {
    Renderer *renderer;
    uint32_t id;
    int width;
    int height;
    int channels;
    bool has_mipmap;
    bool _is_begin_;
    int32_t _id_before_begin_;
    int32_t _active_id_before_begin_;
};


// -------- API текстуры: --------


// Создать текстуру:
Texture* Texture_create(Renderer *renderer);

// Уничтожить текстуру:
void Texture_destroy(Texture **texture);

// Загрузить текстуру (из файла):
void Texture_load(Texture *texture, const char *filepath, bool use_mipmap);

// Активация текстуры:
void Texture_begin(Texture *self);

// Деактивация текстуры:
void Texture_end(Texture *self);

// Загрузить текстуру (из картинки):
void Texture_load_pixmap(Texture *self, Pixmap *pixmap, bool use_mipmap);

// Установить данные текстуры:
void Texture_set_data(Texture *self, const int width, const int height, const void *data, bool use_mipmap,
                      TextureFormat tex_format, TextureFormat data_format, TextureDataType data_type);

// Получить картинку из текстуры:
Pixmap* Texture_get_pixmap(Texture *self, int channels);

// Установить фильтрацию текстуры:
void Texture_set_filter(Texture *self, int name, int param);

// Установить повторение текстуры:
void Texture_set_wrap(Texture *self, int axis, int param);

// Установить линейную фильтрацию текстуры:
void Texture_set_linear(Texture *self);

// Установить пикселизацию текстуры:
void Texture_set_pixelized(Texture *self);
