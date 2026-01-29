//
// fbo.c - Frame Buffer Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/array.h>
#include "../gl.h"
#include "../buffer_gc.h"
#include "buffers.h"


// Создать буфер кадра:
BufferFBO* BufferFBO_create(int width, int height) {
    BufferFBO *fbo = (BufferFBO*)mm_alloc(sizeof(BufferFBO));

    // Заполняем поля:
    fbo->width = width;
    fbo->height = height;
    fbo->id = 0;
    fbo->rbo_id = 0;
    fbo->_is_begin_ = false;
    fbo->_id_before_begin_ = 0;
    fbo->_rbo_id_before_begin_ = 0;
    fbo->_id_before_read_ = 0;
    fbo->_id_before_draw_ = 0;
    fbo->attachments = Array_create(sizeof(int), 16);

    // Создаём фреймбуфер и буфер рендера:
    glGenFramebuffers(1, &fbo->id);
    glGenRenderbuffers(1, &fbo->rbo_id);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &fbo->_rbo_id_before_begin_);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->rbo_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo->width, fbo->height);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->_rbo_id_before_begin_);
    return fbo;
}

// Уничтожить буфер кадра:
void BufferFBO_destroy(BufferFBO **fbo) {
    if (!fbo || !*fbo) return;

    // Удаляем сам буфер:
    BufferFBO_end(*fbo);
    BufferGC_GL_push(BGC_GL_FBO, (*fbo)->id);      // Добавляем буфер в стек на уничтожение.
    BufferGC_GL_push(BGC_GL_RBO, (*fbo)->rbo_id);  // Добавляем буфер в стек на уничтожение.
    Array_destroy(&(*fbo)->attachments);
    mm_free(*fbo);
    *fbo = NULL;
}


// -------- Вспомогательные функции: --------


static inline void attach_handle_array(BufferFBO *self, uint32_t attachment, uint32_t tex_id) {
    // Если текстура не равна нулю, то добавляем ее в список привязок:
    if (tex_id != 0) {
        // Проверяем есть ли в массиве уже эта привязка, и если есть - перезаписываем:
        for (size_t i=0; i < Array_len(self->attachments); i++) {
            uint32_t attach = *(uint32_t*)Array_get(self->attachments, i);
            if (attach == GL_COLOR_ATTACHMENT0+attachment) {  // Если нашли то перезаписываем:
                Array_set(self->attachments, i, &(uint32_t){GL_COLOR_ATTACHMENT0+attachment});
                return;
            }
        }
        // Если не нашли то добавляем в конец:
        Array_push(self->attachments, &(uint32_t){GL_COLOR_ATTACHMENT0+attachment});
    } else {  // Иначе значит что текстура отвязывается, по этому ищем и удаляем привязку:
        for (size_t i=0; i < Array_len(self->attachments); i++) {
            uint32_t attach = *(uint32_t*)Array_get(self->attachments, i);
            if (attach == GL_COLOR_ATTACHMENT0+attachment) {
                Array_remove(self->attachments, i, NULL);
                break;
            }
        }
    }
}


static inline void fbo_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height, int64_t mode) {
    // Сохраняем состояние буферов:
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &self->_id_before_read_);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &self->_id_before_draw_);

    // Настраиваем и отображаем:
    glBindFramebuffer(GL_READ_FRAMEBUFFER, self->id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_fbo_id);
    glBlitFramebuffer(
        x, y, width, height,
        x, y, width, height,
        mode, GL_NEAREST
    );

    // Восстанавливаем состояние буферов:
    glBindFramebuffer(GL_READ_FRAMEBUFFER, self->_id_before_read_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, self->_id_before_draw_);
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferFBO_begin(BufferFBO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &self->_id_before_begin_);
    glBindFramebuffer(GL_FRAMEBUFFER, self->id);
    BufferFBO_apply(self);
    self->_is_begin_ = true;
}

// Не использовать буфер:
void BufferFBO_end(BufferFBO *self) {
    if (!self || !self->_is_begin_) return;
    glBindFramebuffer(GL_FRAMEBUFFER, (uint32_t)self->_id_before_begin_);
    if (self->_id_before_begin_ == 0) glDrawBuffer(GL_BACK);
    self->_is_begin_ = false;
}

// Очистить буфер:
void BufferFBO_clear(BufferFBO *self, float r, float g, float b, float a) {
    if (!self || !self->_is_begin_) return;
    glClearDepth(1.0f);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Изменить размер текстур:
void BufferFBO_resize(BufferFBO *self, int width, int height) {
    if (!self) return;
    self->width = width;
    self->height = height;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &self->_rbo_id_before_begin_);
    glBindRenderbuffer(GL_RENDERBUFFER, self->rbo_id);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        self->width, self->height
    );
    glBindRenderbuffer(GL_RENDERBUFFER, self->_rbo_id_before_begin_);
}

// Применить массив привязок для записи данных в них:
void BufferFBO_apply(BufferFBO *self) {
    if (!self || !self->_is_begin_) return;
    glDrawBuffers(Array_len(self->attachments), (const uint32_t*)self->attachments->data);
}

// Скопировать цвет и глубину в другой кадровый буфер:
void BufferFBO_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height) {
    if (!self || !self->_is_begin_) return;
    fbo_blit(self, dest_fbo_id, x, y, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Скопировать только цвет в другой кадровый буфер:
void BufferFBO_blit_color(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height) {
    if (!self || !self->_is_begin_) return;
    fbo_blit(self, dest_fbo_id, x, y, width, height, GL_COLOR_BUFFER_BIT);
}

// Скопировать только глубину в другой кадровый буфер:
void BufferFBO_blit_depth(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height) {
    if (!self || !self->_is_begin_) return;
    fbo_blit(self, dest_fbo_id, x, y, width, height, GL_DEPTH_BUFFER_BIT);
}

// Привязать 2D текстуру:
void BufferFBO_attach(BufferFBO *self, BufferFBO_Type type, uint32_t attachment, uint32_t tex_id) {
    if (!self || !self->_is_begin_) return;

    switch (type) {
        // Привязываем глубину:
        case BUFFER_FBO_DEPTH: {
            glEnable(GL_DEPTH_TEST);  // Включаем тест глубины.

            // Если текстура глубины не задана, используем рендербуфер:
            if (tex_id == 0) {
                glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    GL_RENDERBUFFER,
                    self->rbo_id
                );
            } else {  // Иначе используем текстуру:
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_2D,
                    tex_id, 0
                );
            }
        } break;

        // Привязываем цвет:
        case BUFFER_FBO_COLOR:
        default: {
            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + attachment,
                GL_TEXTURE_2D,
                tex_id, 0
            );
            attach_handle_array(self, attachment, tex_id);
        } break;
    }
}
