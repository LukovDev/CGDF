//
// qbo.c - Query Buffer Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../gl.h"
#include "../buffer_gc.h"
#include "buffers.h"


// Создать буфер отслеживания:
BufferQBO* BufferQBO_create() {
    BufferQBO *qbo = (BufferQBO*)mm_alloc(sizeof(BufferQBO));

    // Заполняем поля:
    qbo->id = 0;
    qbo->_is_begin_ = false;
    glGenQueries(1, &qbo->id);
    return qbo;
}

// Уничтожить буфер отслеживания:
void BufferQBO_destroy(BufferQBO **qbo) {
    if (!qbo || !*qbo) return;

    // Удаляем сам буфер:
    BufferQBO_end(*qbo);
    BufferGC_GL_push(BGC_GL_QBO, (*qbo)->id);  // Добавляем буфер в стек на уничтожение.
    mm_free(*qbo);
    *qbo = NULL;
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferQBO_begin(BufferQBO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glBeginQuery(GL_PRIMITIVES_GENERATED, self->id);
    self->_is_begin_ = true;
}

// Не использовать буфер:
void BufferQBO_end(BufferQBO *self) {
    if (!self || !self->_is_begin_) return;
    glEndQuery(GL_PRIMITIVES_GENERATED);
    self->_is_begin_ = false;
}

// Получить количество отрисованных примитивов:
uint32_t BufferQBO_get_primitives(BufferQBO *self) {
    if (!self) return 0;
    uint32_t result;
    glGetQueryObjectuiv(self->id, GL_QUERY_RESULT, &result);
    return result;
}
