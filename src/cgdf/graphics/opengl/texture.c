//
// texture.c - Код для работы с текстурами в OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/pixmap.h>
#include <cgdf/core/logger.h>
#include "../core/renderer.h"
#include "../core/texture.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// -------- Вспомогательные функции: --------


// Подбираем формат текстуры (каналы):
static int get_format(TextureFormat format) {
    switch (format) {
        case TEX_FORMAT_R:             return GL_RED;
        case TEX_FORMAT_RG:            return GL_RG;
        case TEX_FORMAT_RGB:           return GL_RGB;
        case TEX_FORMAT_BGR:           return GL_BGR;
        case TEX_FORMAT_RGBA:          return GL_RGBA;
        case TEX_FORMAT_BGRA:          return GL_BGRA;
        case TEX_FORMAT_DEPTH:         return GL_DEPTH_COMPONENT;
        case TEX_FORMAT_DEPTH_STENCIL: return GL_DEPTH_STENCIL;
        default:                       return GL_RGBA;
    }
}

// Подбираем внутренний формат текстуры:
static int get_internal_format(TextureInternalFormat internal) {
    switch (internal) {
        case TEX_INTERNAL_R8:                  return GL_R8;
        case TEX_INTERNAL_RG8:                 return GL_RG8;
        case TEX_INTERNAL_RGB8:                return GL_RGB8;
        case TEX_INTERNAL_RGBA8:               return GL_RGBA8;
        case TEX_INTERNAL_R16F:                return GL_R16F;
        case TEX_INTERNAL_RGB16F:              return GL_RGB16F;
        case TEX_INTERNAL_RGBA16F:             return GL_RGBA16F;
        case TEX_INTERNAL_RGB32F:              return GL_RGB32F;
        case TEX_INTERNAL_RGBA32F:             return GL_RGBA32F;
        case TEX_INTERNAL_SRGB8:               return GL_SRGB8;
        case TEX_INTERNAL_SRGBA8:              return GL_SRGB8_ALPHA8;
        case TEX_INTERNAL_DEPTH16:             return GL_DEPTH_COMPONENT16;
        case TEX_INTERNAL_DEPTH24:             return GL_DEPTH_COMPONENT24;
        case TEX_INTERNAL_DEPTH32F:            return GL_DEPTH_COMPONENT32F;
        case TEX_INTERNAL_DEPTH24_STENCIL8:    return GL_DEPTH24_STENCIL8;
        default:                               return GL_RGBA8;
    }
}

// Подбираем тип данных текстуры:
static int get_data_type(TextureDataType dtype) {
    switch (dtype) {
        case TEX_DATA_UBYTE:   return GL_UNSIGNED_BYTE;
        case TEX_DATA_BYTE:    return GL_BYTE;
        case TEX_DATA_USHORT:  return GL_UNSIGNED_SHORT;
        case TEX_DATA_SHORT:   return GL_SHORT;
        case TEX_DATA_UINT:    return GL_UNSIGNED_INT;
        case TEX_DATA_INT:     return GL_INT;
        case TEX_DATA_FLOAT:   return GL_FLOAT;
        default:               return GL_UNSIGNED_BYTE;
    }
}

// Размер одного пикселя internal format:
static size_t bytes_per_internal(TextureInternalFormat internal)
{
    switch (internal)
    {
        case TEX_INTERNAL_R8:       return 1;
        case TEX_INTERNAL_RG8:      return 2;
        case TEX_INTERNAL_RGB8:     return 3;
        case TEX_INTERNAL_RGBA8:    return 4;

        case TEX_INTERNAL_R16F:     return 2;
        case TEX_INTERNAL_RGB16F:   return 6;
        case TEX_INTERNAL_RGBA16F:  return 8;

        case TEX_INTERNAL_RGB32F:   return 12;
        case TEX_INTERNAL_RGBA32F:  return 16;

        case TEX_INTERNAL_DEPTH16:  return 2;
        case TEX_INTERNAL_DEPTH24:  return 4;
        case TEX_INTERNAL_DEPTH32F: return 4;
        case TEX_INTERNAL_DEPTH24_STENCIL8: return 4;

        case TEX_INTERNAL_SRGB8:  return 3;
        case TEX_INTERNAL_SRGBA8: return 4;
        default:                  return 4;
    }
}

// Получить размер текстуры:
static size_t get_tex_size(Texture *self) {
    if (!self) return 0;
    size_t total = 0;
    int width = self->width, height = self->height;
    while (true) {
        total += width * height * bytes_per_internal(self->internal);
        if (!self->has_mipmap) break;
        if (width == 1 && height == 1) break;
        width  = width  > 1 ? width  / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    return total;
}


// -------- API текстуры: --------


// Создать текстуру:
Texture* Texture_create(Renderer *renderer) {
    if (!renderer) return NULL;

    // Заполняем поля:
    Texture *texture = (Texture*)mm_alloc(sizeof(Texture));
    texture->renderer = renderer;
    texture->id = 0;
    texture->width = 1;
    texture->height = 1;
    texture->channels = 4;
    texture->has_mipmap = false;
    texture->_is_begin_ = false;
    texture->format = TEX_FORMAT_RGBA;
    texture->internal = TEX_INTERNAL_RGBA8;
    texture->dtype = TEX_DATA_UBYTE;
    texture->size = texture->width*texture->height*bytes_per_internal(texture->internal);
    texture->_id_before_begin_ = 0;
    texture->_active_id_before_begin_ = 0;
    return texture;
}

// Уничтожить текстуру:
void Texture_destroy(Texture **texture) {
    if (!texture || !*texture) return;

    // Удаляем саму текстуру:
    Texture_end(*texture);
    BufferGC_GL_push(BGC_GL_TBO, (*texture)->id);  // Добавляем буфер в стек на уничтожение.
    TexUnits_invalidate_texture((*texture)->id);   // Отвязываем текстуру от всех привязанных юнитов.

    // Освобождаем структуру:
    mm_free(*texture);
    *texture = NULL;
}

// Активация текстуры:
void Texture_begin(Texture *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &self->_active_id_before_begin_);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &self->_id_before_begin_);
    if ((uint32_t)self->_id_before_begin_ != self->id) {
        glBindTexture(GL_TEXTURE_2D, self->id);
    }
    self->_is_begin_ = true;
}

// Деактивация текстуры:
void Texture_end(Texture *self) {
    if (!self || !self->_is_begin_) return;
    glActiveTexture(GL_TEXTURE0);
    if ((uint32_t)self->_id_before_begin_ != self->id) {
        glBindTexture(GL_TEXTURE_2D, (uint32_t)self->_id_before_begin_);
    } if (self->_active_id_before_begin_ != GL_TEXTURE0) {
        glActiveTexture(self->_active_id_before_begin_);
    }
    self->_is_begin_ = false;
}

// Сделать пустую текстуру нужного размера:
void Texture_empty(
    Texture *self, int width, int height, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
) {
    if (!self) return;

    width = width <= 0 ? 1 : width;
    height = height <= 0 ? 1 : height;
    Texture_set_data(self, width, height, NULL, use_mipmap, format, internal, dtype);
}

// Загрузить текстуру (из файла):
void Texture_load(Texture *self, const char *filepath, bool use_mipmap) {
    Pixmap *img = Pixmap_load(filepath, PIXMAP_RGBA);
    if (!img) return;

    TextureFormat format = TEX_FORMAT_RGBA;
    TextureInternalFormat internal = TEX_INTERNAL_RGBA8;
    TextureDataType dtype = TEX_DATA_UBYTE;

    // Определяем формат текстуры:
    switch (img->channels) {
        case PIXMAP_R:    { format = TEX_FORMAT_R; break; }
        case PIXMAP_RG:   { format = TEX_FORMAT_RG; break; }
        case PIXMAP_RGB:  { format = TEX_FORMAT_RGB; break; }
        case PIXMAP_RGBA: { format = TEX_FORMAT_RGBA; break; }
        default: break;
    }

    // Определяем внутренний формат текстуры:
    if (img->is_hdr) {
        dtype = TEX_DATA_FLOAT;
        internal = (format == TEX_FORMAT_RGBA) ? TEX_INTERNAL_RGBA16F : TEX_INTERNAL_RGB16F;
    } else {
        switch (format) {
            case TEX_FORMAT_RGBA: internal = TEX_INTERNAL_RGBA8; break;
            case TEX_FORMAT_RGB:  internal = TEX_INTERNAL_RGB8;  break;
            case TEX_FORMAT_RG:   internal = TEX_INTERNAL_RG8;   break;
            default:              internal = TEX_INTERNAL_R8;    break;
        }
    }

    Texture_set_data(self, img->width, img->height, img->data, use_mipmap, format, internal, dtype);
    Pixmap_destroy(&img);
}

// Загрузить текстуру (из картинки):
void Texture_load_pixmap(Texture *self, Pixmap *pixmap, bool use_mipmap) {
    if (!self || !pixmap) return;

    // Подбираем формат данных:
    TextureFormat tex_format = TEX_FORMAT_RGBA;
    TextureInternalFormat tex_internal = TEX_INTERNAL_RGBA8;
    switch (pixmap->channels) {
        case PIXMAP_R:    { tex_format = TEX_FORMAT_R;    tex_internal = TEX_INTERNAL_R8; break; }
        case PIXMAP_RG:   { tex_format = TEX_FORMAT_RG;   tex_internal = TEX_INTERNAL_RG8; break; }
        case PIXMAP_RGB:  { tex_format = TEX_FORMAT_RGB;  tex_internal = TEX_INTERNAL_RGB8; break; }
        case PIXMAP_RGBA: { tex_format = TEX_FORMAT_RGBA; tex_internal = TEX_INTERNAL_RGBA8; break; }
        default:          { tex_format = TEX_FORMAT_RGBA; tex_internal = TEX_INTERNAL_RGBA8; break; }
    }

    // Если это HDR:
    if (pixmap->is_hdr) {
        switch (pixmap->channels) {
            case PIXMAP_R:    { tex_internal = TEX_INTERNAL_R16F; break; }
            case PIXMAP_RGB:  { tex_internal = TEX_INTERNAL_RGB16F; break; }
            case PIXMAP_RGBA: { tex_internal = TEX_INTERNAL_RGBA16F; break; }
            default:          { tex_internal = TEX_INTERNAL_RGB16F; break; }
        }
    }

    // Выделяем память под данные:
    Texture_set_data(
        self, pixmap->width, pixmap->height, pixmap->data, use_mipmap,
        tex_format, tex_internal, pixmap->is_hdr ? TEX_DATA_FLOAT : TEX_DATA_UBYTE
    );
}

// Загрузить текстуру (из файла) расширенный режим:
void Texture_load_advanced(
    Texture *self, const char *filepath, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
) {
    PixmapFormat img_channels = PIXMAP_RGBA;
    switch (format) {
        case TEX_FORMAT_R:    { img_channels = PIXMAP_R; break; }
        case TEX_FORMAT_RG:   { img_channels = PIXMAP_RG; break; }
        case TEX_FORMAT_RGB:  { img_channels = PIXMAP_RGB; break; }
        case TEX_FORMAT_RGBA: { img_channels = PIXMAP_RGBA; break; }
        default:              { img_channels = PIXMAP_RGBA; break; }
    }
    Pixmap *img = Pixmap_load(filepath, img_channels);
    if (!self || !img) { if (img) { Pixmap_destroy(&img); } return; }
    Texture_set_data(self, img->width, img->height, img->data, use_mipmap, format, internal, dtype);
    if (img) { Pixmap_destroy(&img); }
}

// Установить данные текстуры:
void Texture_set_data(
    Texture *self, const int width, const int height, const void *data, bool use_mipmap,
    TextureFormat format, TextureInternalFormat internal, TextureDataType dtype
) {
    if (!self) return;

    // Ограничиваем размер текстуры до максимального возможного и минимального:
    int max_tex_size = Renderer_get_max_texture_size(self->renderer);
    if (width > max_tex_size || height > max_tex_size) {
        log_msg(
            "[W] Texture_set_data: The texture (id=%d) is too large. "
            "Requested: [w%d x h%d]. Max texture size: [w%d x h%d].\n",
            self->id, width, height, max_tex_size, max_tex_size
        );
    }
    self->width  = width  <= 0 ? 1 : (width  > max_tex_size ? max_tex_size : width);
    self->height = height <= 0 ? 1 : (height > max_tex_size ? max_tex_size : height);

    // Если текстура еще не создана, то создаем ее:
    if (self->id == 0) glGenTextures(1, &self->id);
    if (self->id == 0) {  // Если она так и не создалась, то выходим:
        log_msg("[E] Texture_set_data: The texture could not be created.\n");
        return;
    }

    // Получаем форматы в OpenGL:
    if (internal == TEX_INTERNAL_RGB32F && dtype != TEX_DATA_FLOAT) dtype = TEX_DATA_FLOAT;
    if (internal == TEX_INTERNAL_RGB8 && format == TEX_FORMAT_RGBA) self->format = TEX_FORMAT_RGB;
    int gl_format    = get_format(format);
    int gl_internal  = get_internal_format(internal);
    int gl_data_type = get_data_type(dtype);

    Texture_begin(self);

    // UNPACK alignment для RGB/BGR:
    int prev_unpack = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    // Загрузка данных текстуры:
    self->format = format;
    self->internal = internal;
    self->dtype = dtype;
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal, self->width, self->height, 0, gl_format, gl_data_type, data);

    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack);
    }

    // Если надо использовать мипмапы, создаём их:
    self->has_mipmap = use_mipmap;
    if (use_mipmap) glGenerateMipmap(GL_TEXTURE_2D);

    // Устанавливаем фильтры по умолчанию:
    Texture_set_linear(self);

    // Получаем размер текстуры:
    self->size = get_tex_size(self);
    Texture_end(self);
}

// Установить данные текстуры (подмассив):
void Texture_set_subdata(
    Texture *self, int miplevel, int offset_x, int offset_y, int width, int height,
    TextureFormat format, TextureDataType dtype, const void *data
) {
    if (!self) return;

    // Получаем форматы в OpenGL:
    int gl_format    = get_format(format);
    int gl_data_type = get_data_type(dtype);

    Texture_begin(self);

    // UNPACK alignment для RGB/BGR:
    int prev_unpack = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    self->format = format;
    self->dtype = dtype;
    glTexSubImage2D(GL_TEXTURE_2D, miplevel, offset_x, offset_y, width, height, gl_format, gl_data_type, data);

    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack);
    }

    // Если надо использовать мипмапы, создаём их:
    if (self->has_mipmap) glGenerateMipmap(GL_TEXTURE_2D);

    // Получаем размер текстуры:
    self->size = get_tex_size(self);
    Texture_end(self);
}

// Получить картинку из текстуры:
Pixmap* Texture_get_pixmap(Texture *self, int channels) {
    if (!self) return NULL;

    // Выделяем память под данные (указатель на блок сохраняется в img ниже):
    TextureDataType type = TEX_DATA_UBYTE;
    switch (self->internal) {
        case TEX_INTERNAL_R16F:
        case TEX_INTERNAL_RGB16F:
        case TEX_INTERNAL_RGBA16F:
        case TEX_INTERNAL_RGB32F:
        case TEX_INTERNAL_RGBA32F: { type = TEX_DATA_FLOAT; break; }
        default: break;
    }
    size_t bpp = channels * (type == TEX_DATA_FLOAT ? sizeof(float) : sizeof(uint8_t));
    void* data = mm_alloc(self->width * self->height * bpp);

    // Подбираем формат данных:
    int gl_format;
    switch (channels) {
        case PIXMAP_R:    { gl_format = GL_RED; break; }
        case PIXMAP_RG:   { gl_format = GL_RG; break; }
        case PIXMAP_RGB:  { gl_format = GL_RGB; break; }
        case PIXMAP_RGBA: { gl_format = GL_RGBA; break; }
        default: { gl_format = GL_RGBA; break; }
    }
    Texture_begin(self);
    int32_t prev_pack = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack);
    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    }
    glGetTexImage(GL_TEXTURE_2D, 0, gl_format, get_data_type(type), data);
    if (gl_format == GL_RGB || gl_format == GL_BGR) {
        glPixelStorei(GL_PACK_ALIGNMENT, prev_pack);
    }
    Texture_end(self);

    // Создаём изображение:
    Pixmap* pixmap = mm_alloc(sizeof(Pixmap));

    pixmap->width = self->width;
    pixmap->height = self->height;
    pixmap->channels = channels;
    pixmap->from_stbi = false;
    pixmap->is_hdr = (type == TEX_DATA_FLOAT);
    pixmap->data = data;
    return pixmap;  // Не забудьте уничтожить Pixmap!
}

// Получить размер текстуры:
size_t Texture_get_size(Texture *self) {
    if (!self) return 0;
    return self->size;
}

// Установить фильтрацию текстуры:
void Texture_set_filter(Texture *self, int name, int param) {
    if (!self) return;
    Texture_begin(self);
    glTexParameteri(GL_TEXTURE_2D, name, param);
    Texture_end(self);
}

// Установить повторение текстуры:
void Texture_set_wrap(Texture *self, int axis, int param) {
    if (!self) return;
    Texture_begin(self);
    switch (axis) {
        case GL_TEXTURE_WRAP_S: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
        } break;
        case GL_TEXTURE_WRAP_T: {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);
        } break;
        case GL_TEXTURE_WRAP_R: { // Пригодится для 3D текстур.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, param);
        } break;
        default: {
            // Ничего не делаем.
        } break;
    }
    Texture_end(self);
}

// Установить линейную фильтрацию текстуры:
void Texture_set_linear(Texture *self) {
    if (!self) return;
    if (!self->has_mipmap) Texture_set_filter(self, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    else Texture_set_filter(self, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    Texture_set_filter(self, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Установить пикселизацию текстуры:
void Texture_set_pixelized(Texture *self) {
    if (!self) return;
    if (!self->has_mipmap) Texture_set_filter(self, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    else Texture_set_filter(self, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    Texture_set_filter(self, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
