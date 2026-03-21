//
// texture.c - Код для работы с текстурами в OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/pixmap.h>
#include <cgdf/core/logger.h>
#include "../renderer.h"
#include "../texture.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// -------- Вспомогательные функции: --------


// Подбираем формат текстуры (internalformat):
static int get_tex_format(TextureFormat format) {
    int gl_tex_format = GL_RGBA;
    switch (format) {
        case TEX_RED:      { gl_tex_format = GL_RED; break; }
        case TEX_RG:       { gl_tex_format = GL_RG; break; }
        case TEX_RGB:      { gl_tex_format = GL_RGB; break; }
        case TEX_RGBA:     { gl_tex_format = GL_RGBA; break; }
        case TEX_RGB16F:   { gl_tex_format = GL_RGB16F; break; }
        case TEX_RGBA16F:  { gl_tex_format = GL_RGBA16F; break; }
        case TEX_RGB32F:   { gl_tex_format = GL_RGB32F; break; }
        case TEX_RGBA32F:  { gl_tex_format = GL_RGBA32F; break; }
        case TEX_R16F:     { gl_tex_format = GL_R16F; break; }
        case TEX_SRGB:     { gl_tex_format = GL_SRGB8; break; }
        case TEX_SRGBA:    { gl_tex_format = GL_SRGB8_ALPHA8; break; }
        case TEX_BGR:      { gl_tex_format = GL_RGB; break; }
        case TEX_BGRA:     { gl_tex_format = GL_RGBA; break; }
        case TEX_RGBA8:    { gl_tex_format = GL_RGBA8; break; }
        case TEX_DEPTH16:  { gl_tex_format = GL_DEPTH_COMPONENT16; break; }
        case TEX_DEPTH24:  { gl_tex_format = GL_DEPTH_COMPONENT24; break; }
        case TEX_DEPTH32:  { gl_tex_format = GL_DEPTH_COMPONENT32; break; }
        case TEX_DEPTH32F: { gl_tex_format = GL_DEPTH_COMPONENT32F; break; }
        case TEX_DEPTH_COMPONENT: { gl_tex_format = GL_DEPTH_COMPONENT16; break; }
        case TEX_DEPTH_STENCIL:   { gl_tex_format = GL_DEPTH24_STENCIL8; break; }
    }
    return gl_tex_format;
}

// Подбираем формат внешних данных:
static int get_data_format(TextureFormat format) {
    int gl_data_format = GL_RGBA;
    switch (format) {
        case TEX_RED:     { gl_data_format = GL_RED; break; }
        case TEX_RG:      { gl_data_format = GL_RG; break; }
        case TEX_RGB:     { gl_data_format = GL_RGB; break; }
        case TEX_RGBA:    { gl_data_format = GL_RGBA; break; }
        case TEX_RGB16F:
        case TEX_RGB32F:
        case TEX_SRGB:    { gl_data_format = GL_RGB; break; }
        case TEX_RGBA8:
        case TEX_RGBA16F:
        case TEX_RGBA32F:
        case TEX_SRGBA:   { gl_data_format = GL_RGBA; break; }
        case TEX_BGR:     { gl_data_format = GL_BGR; break; }
        case TEX_BGRA:    { gl_data_format = GL_BGRA; break; }
        case TEX_R16F:    { gl_data_format = GL_RED; break; }
        case TEX_DEPTH16:
        case TEX_DEPTH24:
        case TEX_DEPTH32:
        case TEX_DEPTH32F:
        case TEX_DEPTH_COMPONENT: { gl_data_format = GL_DEPTH_COMPONENT; break; }
        case TEX_DEPTH_STENCIL:   { gl_data_format = GL_DEPTH_STENCIL; break; }
    }
    return gl_data_format;
}

// Подбираем тип данных:
static int get_data_type(TextureDataType dtype) {
    int gl_data_type;
    switch (dtype) {
        case TEX_DATA_UBYTE:  { gl_data_type = GL_UNSIGNED_BYTE; break; }
        case TEX_DATA_BYTE:   { gl_data_type = GL_BYTE; break; }
        case TEX_DATA_USHORT: { gl_data_type = GL_UNSIGNED_SHORT; break; }
        case TEX_DATA_SHORT:  { gl_data_type = GL_SHORT; break; }
        case TEX_DATA_UINT:   { gl_data_type = GL_UNSIGNED_INT; break; }
        case TEX_DATA_INT:    { gl_data_type = GL_INT; break; }
        case TEX_DATA_FLOAT:  { gl_data_type = GL_FLOAT; break; }
        default:              { gl_data_type = GL_UNSIGNED_BYTE; break; }
    }
    return gl_data_type;
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

// Сделать пустую текстуру нужного размера:
void Texture_empty(Texture *self, int width, int height, bool use_mipmap, TextureFormat format, TextureDataType dtype) {
    if (!self) return;

    width = width <= 0 ? 1 : width;
    height = height <= 0 ? 1 : height;
    char *null_data = mm_calloc(1, (size_t)width * (size_t)height * 4);
    Texture_set_data(self, width, height, null_data, use_mipmap, format, format, dtype);
    mm_free(null_data);
}

// Загрузить текстуру (из файла):
void Texture_load(Texture *self, const char *filepath, bool use_mipmap) {
    Pixmap *img = Pixmap_load(filepath, PIXMAP_RGBA);
    if (!self || !img) { if (img) { Pixmap_destroy(&img); } return; }
    Texture_set_data(self, img->width, img->height, img->data, use_mipmap, TEX_RGBA, TEX_RGBA, TEX_DATA_UBYTE);
    if (img) { Pixmap_destroy(&img); }
}

// Активация текстуры:
void Texture_begin(Texture *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &self->_id_before_begin_);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &self->_active_id_before_begin_);
    glActiveTexture(GL_TEXTURE0);
    if (self->_id_before_begin_ != self->id) {
        glBindTexture(GL_TEXTURE_2D, self->id);
    }
    self->_is_begin_ = true;
}

// Деактивация текстуры:
void Texture_end(Texture *self) {
    if (!self || !self->_is_begin_) return;
    if (self->_active_id_before_begin_ != 0) {
        glActiveTexture(self->_active_id_before_begin_);
    }
    if (self->_id_before_begin_ != self->id) {
        glBindTexture(GL_TEXTURE_2D, (uint32_t)self->_id_before_begin_);
    }
    self->_is_begin_ = false;
}

// Загрузить текстуру (из картинки):
void Texture_load_pixmap(Texture *self, Pixmap *pixmap, bool use_mipmap) {
    if (!self || !pixmap) return;

    // Подбираем формат данных:
    TextureFormat tex_format;
    switch (pixmap->channels) {
        case 1:  { tex_format = TEX_RED; break; }
        case 2:  { tex_format = TEX_RG; break; }
        case 3:  { tex_format = TEX_RGB; break; }
        case 4:  { tex_format = TEX_RGBA; break; }
        default: { tex_format = TEX_RGBA; break; }
    }

    // Выделяем память под данные:
    Texture_set_data(
        self, pixmap->width, pixmap->height, pixmap->data, use_mipmap,
        tex_format, tex_format, TEX_DATA_UBYTE
    );
}

// Установить данные текстуры:
void Texture_set_data(
    Texture *self, const int width, const int height, const void *data, bool use_mipmap,
    TextureFormat tex_format, TextureFormat data_format, TextureDataType data_type
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

    // Подбираем формат текстуры (internalformat):
    int gl_tex_format = get_tex_format(tex_format);

    // Подбираем формат внешних данных:
    int gl_data_format = get_data_format(data_format);

    // Подбираем тип данных:
    int gl_data_type = get_data_type(data_type);

    // Перепроверка:
    if (gl_tex_format == GL_RGB16F || gl_tex_format == GL_RGBA16F) gl_data_type = GL_HALF_FLOAT;
    else if (gl_tex_format == GL_RGB32F || gl_tex_format == GL_RGBA32F) gl_data_type = GL_FLOAT;

    Texture_begin(self);

    // UNPACK alignment для RGB/BGR:
    int prev_unpack = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    if (gl_data_format == GL_RGB || gl_data_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    // Загрузка данных текстуры:
    glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_format, self->width, self->height, 0, gl_data_format, gl_data_type, data);

    if (gl_data_format == GL_RGB || gl_data_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack);
    }

    // Если надо использовать мипмапы, создаём их:
    self->has_mipmap = use_mipmap;
    if (use_mipmap) glGenerateMipmap(GL_TEXTURE_2D);

    // Устанавливаем фильтры по умолчанию:
    Texture_set_linear(self);
    Texture_end(self);
}

// Установить данные текстуры (подмассив):
void Texture_set_subdata(
    Texture *self, int miplevel, int offset_x, int offset_y, int width, int height,
    TextureFormat data_format, TextureDataType data_type, const void *data
) {
    if (!self) return;

    // Подбираем формат внешних данных:
    int gl_data_format = get_data_format(data_format);

    // Подбираем тип данных:
    int gl_data_type = get_data_type(data_type);

    Texture_begin(self);

    // UNPACK alignment для RGB/BGR:
    int prev_unpack = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    if (gl_data_format == GL_RGB || gl_data_format == GL_BGR) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glTexSubImage2D(GL_TEXTURE_2D, miplevel, offset_x, offset_y, width, height, gl_data_format, gl_data_type, data);

    // Если надо использовать мипмапы, создаём их:
    if (self->has_mipmap) glGenerateMipmap(GL_TEXTURE_2D);

    Texture_end(self);
}

// Получить картинку из текстуры:
Pixmap* Texture_get_pixmap(Texture *self, int channels) {
    if (!self) return NULL;

    // Выделяем память под данные (указатель на блок сохраняется в img ниже):
    unsigned char* data = mm_alloc(self->width * self->height * channels);

    // Подбираем формат данных:
    int gl_data_format;
    switch (channels) {
        case 1:  { gl_data_format = GL_RED; break; }
        case 2:  { gl_data_format = GL_RG; break; }
        case 3:  { gl_data_format = GL_RGB; break; }
        case 4:  { gl_data_format = GL_RGBA; break; }
        default: { gl_data_format = GL_RGBA; break; }
    }
    Texture_begin(self);
    int32_t prev_pack = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack);
    if (gl_data_format == GL_RGB || gl_data_format == GL_BGR) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    }
    glGetTexImage(GL_TEXTURE_2D, 0, gl_data_format, GL_UNSIGNED_BYTE, data);
    if (gl_data_format == GL_RGB || gl_data_format == GL_BGR) {
        glPixelStorei(GL_PACK_ALIGNMENT, prev_pack);
    }
    Texture_end(self);

    // Создаём изображение:
    Pixmap* pixmap = mm_alloc(sizeof(Pixmap));

    pixmap->width = self->width;
    pixmap->height = self->height;
    pixmap->channels = channels;
    pixmap->from_stbi = false;
    pixmap->data = data;
    return pixmap;  // Не забудьте уничтожить Pixmap!
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
