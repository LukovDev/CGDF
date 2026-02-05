//
// vbo.c - Vertex Buffer Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../gl.h"
#include "../buffer_gc.h"
#include "buffers.h"


// Создать буфер вершин:
BufferVBO* BufferVBO_create(const void* data, const size_t size, int mode) {
    BufferVBO *vbo = (BufferVBO*)mm_alloc(sizeof(BufferVBO));

    // Заполняем поля:
    vbo->id = 0;
    vbo->_is_begin_ = false;
    vbo->_id_before_begin_ = 0;
    glGenBuffers(1, &vbo->id);

    // Заполняем буфер:
    BufferVBO_begin(vbo);
    BufferVBO_set_data(vbo, data, size, mode);
    BufferVBO_end(vbo);
    return vbo;
}

// Уничтожить буфер вершин:
void BufferVBO_destroy(BufferVBO **vbo) {
    if (!vbo || !*vbo) return;

    // Удаляем сам буфер:
    BufferVBO_end(*vbo);
    BufferGC_GL_push(BGC_GL_VBO, (*vbo)->id);  // Добавляем буфер в стек на уничтожение.
    mm_free(*vbo);
    *vbo = NULL;
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferVBO_begin(BufferVBO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &self->_id_before_begin_);
    if (self->_id_before_begin_ != self->id) {
        glBindBuffer(GL_ARRAY_BUFFER, self->id);
    }
    self->_is_begin_ = true;
}

// Не использовать буфер:
void BufferVBO_end(BufferVBO *self) {
    if (!self || !self->_is_begin_) return;
    if (self->_id_before_begin_ != self->id) {
        glBindBuffer(GL_ARRAY_BUFFER, (uint32_t)self->_id_before_begin_);
    }
    self->_is_begin_ = false;
}

// Получить размер буфера:
size_t BufferVBO_get_size(BufferVBO *self) {
    if (!self) return 0;
    bool was_begin = self->_is_begin_;
    if (!was_begin) BufferVBO_begin(self);
    int buffer_size;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    if (!was_begin) BufferVBO_end(self);
    return (size_t)buffer_size;
}

// Установить данные буфера (выделяет новую память и заново всё сохраняет):
void BufferVBO_set_data(BufferVBO *self, const void *data, const size_t size, int mode) {
    if (!self) return;
    glBufferData(GL_ARRAY_BUFFER, size, data, mode);
}

// Изменить данные буфера (не выделяет новую память а просто изменяет данные):
void BufferVBO_set_subdata(BufferVBO *self, const void *data, const size_t offset, const size_t size) {
    if (!self) return;
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}
