//
// ebo.c - Element Buffer Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../gl.h"
#include "../buffer_gc.h"
#include "buffers.h"


// Создать буфер индексов:
BufferEBO* BufferEBO_create(const void* data, const size_t size, int mode) {
    BufferEBO *ebo = (BufferEBO*)mm_alloc(sizeof(BufferEBO));

    // Заполняем поля:
    ebo->id = 0;
    ebo->_is_begin_ = false;
    ebo->_id_before_begin_ = 0;
    glGenBuffers(1, &ebo->id);

    // Заполняем буфер:
    BufferEBO_begin(ebo);
    BufferEBO_set_data(ebo, data, size, mode);
    BufferEBO_end(ebo);
    return ebo;
}

// Уничтожить буфер индексов:
void BufferEBO_destroy(BufferEBO **ebo) {
    if (!ebo || !*ebo) return;

    // Удаляем сам буфер:
    BufferEBO_end(*ebo);
    BufferGC_GL_push(BGC_GL_EBO, (*ebo)->id);  // Добавляем буфер в стек на уничтожение.
    mm_free(*ebo);
    *ebo = NULL;
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferEBO_begin(BufferEBO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &self->_id_before_begin_);
    if (self->_id_before_begin_ != self->id) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->id);
    }
    self->_is_begin_ = true;
}

// Не использовать буфер:
void BufferEBO_end(BufferEBO *self) {
    if (!self || !self->_is_begin_) return;
    if (self->_id_before_begin_ != self->id) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (uint32_t)self->_id_before_begin_);
    }
    self->_is_begin_ = false;
}

// Получить размер буфера:
size_t BufferEBO_get_size(BufferEBO *self) {
    if (!self) return 0;
    bool was_begin = self->_is_begin_;
    if (!was_begin) BufferEBO_begin(self);
    int buffer_size;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    if (!was_begin) BufferEBO_end(self);
    return buffer_size;
}

// Установить данные буфера (выделяет новую память и заново всё сохраняет):
void BufferEBO_set_data(BufferEBO *self, const void *data, const size_t size, int mode) {
    if (!self) return;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, mode);
}

// Изменить данные буфера (не выделяет новую память а просто изменяет данные):
void BufferEBO_set_subdata(BufferEBO *self, const void *data, const size_t offset, const size_t size) {
    if (!self) return;
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
}
