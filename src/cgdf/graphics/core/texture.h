//
// texture.h - Определяет функционал текстуры.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/pixmap.h>


// Формат текстуры (channels):
typedef enum TextureFormat {
    TEX_FORMAT_R,
    TEX_FORMAT_RG,
    TEX_FORMAT_RGB,
    TEX_FORMAT_BGR,
    TEX_FORMAT_RGBA,
    TEX_FORMAT_BGRA,
    TEX_FORMAT_DEPTH,
    TEX_FORMAT_DEPTH_STENCIL,
} TextureFormat;


// Внутренний формат данных текстуры (internal format):
typedef enum TextureInternalFormat {
    TEX_INTERNAL_R8,
    TEX_INTERNAL_RG8,
    TEX_INTERNAL_RGB8,
    TEX_INTERNAL_RGBA8,

    TEX_INTERNAL_R16F,
    TEX_INTERNAL_RGB16F,
    TEX_INTERNAL_RGBA16F,

    TEX_INTERNAL_RGB32F,
    TEX_INTERNAL_RGBA32F,

    TEX_INTERNAL_SRGB8,
    TEX_INTERNAL_SRGBA8,

    TEX_INTERNAL_DEPTH16,
    TEX_INTERNAL_DEPTH24,
    TEX_INTERNAL_DEPTH32F,

    TEX_INTERNAL_DEPTH24_STENCIL8,
} TextureInternalFormat;


// Тип данных в текстуре:
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
typedef struct Renderer Renderer;
typedef struct Texture Texture;  // Текстура 2D.


// Структура текстуры:
struct Texture {
    Renderer *renderer;  // Указатель на рендерер.
    uint32_t id;         // Айди текстуры.
    int width;           // Ширина текстуры.
    int height;          // Высота текстуры.
    int channels;        // Количество каналов текстуры.
    bool has_mipmap;     // Наличие мипмапов.
    bool _is_begin_;     // Признак активности текстуры (внутренняя логика).
    TextureFormat format;              // Формат текстуры (каналы).
    TextureInternalFormat internal;    // Внутренний формат текстуры.
    TextureDataType dtype;             // Тип данных текстуры.
    size_t size;                       // Размер текстуры в байтах в VRAM.
    int32_t _id_before_begin_;         // Прошлые айди состояния (внутренняя логика).
    int32_t _active_id_before_begin_;  // Прошлые айди состояния (внутренняя логика).
};


// -------- API текстуры: --------


// Создать текстуру:
Texture* Texture_create(Renderer *renderer);

// Уничтожить текстуру:
void Texture_destroy(Texture **texture);

// Активация текстуры:
void Texture_begin(Texture *self);

// Деактивация текстуры:
void Texture_end(Texture *self);

// Сделать пустую текстуру нужного размера:
void Texture_empty(
    Texture *self, int width, int height, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
);

// Загрузить текстуру (из файла):
void Texture_load(Texture *self, const char *filepath, bool use_mipmap);

// Загрузить текстуру (из картинки):
void Texture_load_pixmap(Texture *self, Pixmap *pixmap, bool use_mipmap);

// Загрузить текстуру (из файла) расширенный режим:
void Texture_load_advanced(
    Texture *self, const char *filepath, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
);

// Установить данные текстуры:
void Texture_set_data(
    Texture *self, const int width, const int height, const void *data, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
);

// Установить данные текстуры (подмассив):
void Texture_set_subdata(
    Texture *self, int miplevel, int offset_x, int offset_y, int width, int height,
    TextureFormat format, TextureDataType dtype, const void *data
);

// Получить картинку из текстуры:
Pixmap* Texture_get_pixmap(Texture *self, int channels);

// Получить размер текстуры:
size_t Texture_get_size(Texture *self);

// Установить фильтрацию текстуры:
void Texture_set_filter(Texture *self, int name, int param);

// Установить повторение текстуры:
void Texture_set_wrap(Texture *self, int axis, int param);

// Установить линейную фильтрацию текстуры:
void Texture_set_linear(Texture *self);

// Установить пикселизацию текстуры:
void Texture_set_pixelized(Texture *self);
