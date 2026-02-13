//
// buffers.h - Структуры и API буферов.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>


// Типы привязок для кадрового буфера:
typedef enum BufferFBO_Type {
    BUFFER_FBO_COLOR,
    BUFFER_FBO_DEPTH,
    // ...
} BufferFBO_Type;


// Объявление структур:
typedef struct BufferQBO BufferQBO;  // Структура буфера отслеживания.
typedef struct BufferFBO BufferFBO;  // Структура буфера кадра.
typedef struct BufferVBO BufferVBO;  // Структура буфера вершин.
typedef struct BufferEBO BufferEBO;  // Структура буфера индексов.
typedef struct BufferVAO BufferVAO;  // Структура буфера атрибутов.


// -------- QBO Буфер: --------


// Структура буфера отслеживания:
struct BufferQBO {
    uint32_t id;
    bool _is_begin_;
};

// Создать буфер отслеживания:
BufferQBO* BufferQBO_create();

// Уничтожить буфер отслеживания:
void BufferQBO_destroy(BufferQBO **qbo);

// Использовать буфер:
void BufferQBO_begin(BufferQBO *self);

// Не использовать буфер:
void BufferQBO_end(BufferQBO *self);

// Получить количество отрисованных примитивов:
uint32_t BufferQBO_get_primitives(BufferQBO *self);


// -------- FBO Буфер: --------


// Структура буфера кадра:
struct BufferFBO {
    int width;
    int height;
    uint32_t id;
    uint32_t rbo_id;
    bool _is_begin_;
    int32_t _id_before_begin_;
    int32_t _rbo_id_before_begin_;
    int32_t _id_before_read_;
    int32_t _id_before_draw_;
    Array *attachments;
};

// Создать буфер кадра:
BufferFBO* BufferFBO_create(int width, int height);

// Уничтожить буфер кадра:
void BufferFBO_destroy(BufferFBO **fbo);

// Использовать буфер:
void BufferFBO_begin(BufferFBO *self);

// Не использовать буфер:
void BufferFBO_end(BufferFBO *self);

// Очистить буфер:
void BufferFBO_clear(BufferFBO *self, float r, float g, float b, float a);

// Изменить размер кадрового буфера:
void BufferFBO_resize(BufferFBO *self, int width, int height);

// Применить массив привязок для записи данных в них:
void BufferFBO_apply(BufferFBO *self);

// Скопировать цвет и глубину в другой кадровый буфер:
void BufferFBO_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);

// Скопировать только цвет в другой кадровый буфер:
void BufferFBO_blit_color(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);

// Скопировать только глубину в другой кадровый буфер:
void BufferFBO_blit_depth(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);

// Привязать 2D текстуру:
void BufferFBO_attach(BufferFBO *self, BufferFBO_Type type, uint32_t attachment, uint32_t tex_id);


// -------- VBO Буфер: --------


// Структура буфера вершин:
struct BufferVBO {
    uint32_t id;
    bool _is_begin_;
    int32_t _id_before_begin_;
};

// Создать буфер вершин:
BufferVBO* BufferVBO_create(const void* data, const size_t size, int mode);

// Уничтожить буфер вершин:
void BufferVBO_destroy(BufferVBO **vbo);

// Использовать буфер:
void BufferVBO_begin(BufferVBO *self);

// Не использовать буфер:
void BufferVBO_end(BufferVBO *self);

// Получить размер буфера:
size_t BufferVBO_get_size(BufferVBO *self);

// Установить данные буфера (выделяет новую память и заново всё сохраняет):
void BufferVBO_set_data(BufferVBO *self, const void *data, const size_t size, int mode);

// Изменить данные буфера (не выделяет новую память а просто изменяет данные):
void BufferVBO_set_subdata(BufferVBO *self, const void *data, const size_t offset, const size_t size);


// -------- EBO Буфер: --------


// Структура буфера индексов:
struct BufferEBO {
    uint32_t id;
    bool _is_begin_;
    int32_t _id_before_begin_;
};

// Создать буфер индексов:
BufferEBO* BufferEBO_create(const void* data, const size_t size, int mode);

// Уничтожить буфер индексов:
void BufferEBO_destroy(BufferEBO **vbo);

// Использовать буфер:
void BufferEBO_begin(BufferEBO *self);

// Не использовать буфер:
void BufferEBO_end(BufferEBO *self);

// Получить размер буфера:
size_t BufferEBO_get_size(BufferEBO *self);

// Установить данные буфера (выделяет новую память и заново всё сохраняет):
void BufferEBO_set_data(BufferEBO *self, const void *data, const size_t size, int mode);

// Изменить данные буфера (не выделяет новую память а просто изменяет данные):
void BufferEBO_set_subdata(BufferEBO *self, const void *data, const size_t offset, const size_t size);


// -------- VAO Буфер: --------


// Структура буфера атрибутов:
struct BufferVAO {
    uint32_t id;
    bool _is_begin_;
    int32_t _id_before_begin_;
};

// Создать буфер атрибутов:
BufferVAO* BufferVAO_create();

// Уничтожить буфер атрибутов:
void BufferVAO_destroy(BufferVAO **vao);

// Использовать буфер:
void BufferVAO_begin(BufferVAO *self);

// Не использовать буфер:
void BufferVAO_end(BufferVAO *self);

// Установить атрибуты вершин:
void BufferVAO_attrib_pointer(
    BufferVAO *self, uint32_t loc, int count, int type, bool normalize, size_t stride, size_t offset
);

// Установить дивизор:
void BufferVAO_attrib_divisor(
    BufferVAO *self, uint32_t loc, int count, int type, bool normalize,
    size_t stride, size_t offset, uint32_t divisor
);
