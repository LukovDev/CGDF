//
// vao.c - Vertex Array Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../gl.h"
#include "../buffer_gc.h"
#include "buffers.h"


// Создать буфер атрибутов:
BufferVAO* BufferVAO_create() {
    BufferVAO *vao = (BufferVAO*)mm_alloc(sizeof(BufferVAO));

    // Заполняем поля:
    vao->id = 0;
    vao->_is_begin_ = false;
    glGenVertexArrays(1, &vao->id);
    return vao;
}

// Уничтожить буфер атрибутов:
void BufferVAO_destroy(BufferVAO **vao) {
    if (!vao || !*vao) return;

    // Удаляем сам буфер:
    BufferVAO_end(*vao);
    BufferGC_GL_push(BGC_GL_VAO, (*vao)->id);  // Добавляем буфер в стек на уничтожение.
    mm_free(*vao);
    *vao = NULL;
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferVAO_begin(BufferVAO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glBindVertexArray(self->id);
    self->_is_begin_ = true;
}

// Не использовать буфер:
void BufferVAO_end(BufferVAO *self) {
    if (!self || !self->_is_begin_) return;
    glBindVertexArray(0);
    self->_is_begin_ = false;
}

// Установить атрибуты вершин:
void BufferVAO_attrib_pointer(
    BufferVAO *self, int loc, size_t count, int type, bool normalize, size_t stride, size_t offset
) {
    if (!self) return;
    glVertexAttribPointer(loc, count, type, normalize ? GL_TRUE : GL_FALSE, stride, (void*)offset);
    glEnableVertexAttribArray(loc);
}
