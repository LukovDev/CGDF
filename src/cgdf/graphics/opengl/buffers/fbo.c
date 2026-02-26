//
// fbo.c - Frame Buffer Object.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
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


static void attach_handle_array(BufferFBO *self, uint32_t attachment, uint32_t tex_id) {
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


static void fbo_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height, int64_t mode) {
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


static bool fbo_has_stencil(uint32_t fbo_id) {
    int prev = 0, type = GL_NONE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type
    );
    glBindFramebuffer(GL_FRAMEBUFFER, (uint32_t)prev);
    return type != GL_NONE;
}


static void fbo_check_complete(const char *tag) {
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        log_msg("[E] FBO incomplete (%s): 0x%X\n", tag, st);
    }
}


// -------- Реализация API: --------


// Использовать буфер:
void BufferFBO_begin(BufferFBO *self) {
    if (!self || self->_is_begin_ || self->id == 0) return;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &self->_id_before_begin_);
    if (self->_id_before_begin_ != self->id) {
        glBindFramebuffer(GL_FRAMEBUFFER, self->id);
    }
    self->_is_begin_ = true;
    BufferFBO_apply(self);  // Только после _is_begin_ = true.
}

// Не использовать буфер:
void BufferFBO_end(BufferFBO *self) {
    if (!self || !self->_is_begin_) return;
    if (self->_id_before_begin_ != self->id) {
        glBindFramebuffer(GL_FRAMEBUFFER, (uint32_t)self->_id_before_begin_);
    }
    if (self->_id_before_begin_ == 0) glDrawBuffer(GL_BACK);
    self->_is_begin_ = false;
}

// Очистить буфер:
void BufferFBO_clear(BufferFBO *self, float r, float g, float b, float a) {
    if (!self || !self->_is_begin_) return;
    glClearDepth(1.0f);
    glClearColor(r, g, b, a);
    uint32_t clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    if (fbo_has_stencil(self->id)) clear_mask |= GL_STENCIL_BUFFER_BIT;
    glClear(clear_mask);
}

// Изменить размер кадрового буфера:
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

// Активировать привязку для записи в неё данных:
void BufferFBO_active(BufferFBO *self, uint32_t attachment) {
    if (!self || !self->_is_begin_) return;
    glDrawBuffer(GL_COLOR_ATTACHMENT0+attachment);
    fbo_check_complete("active");
}

// Применить массив привязок для записи данных в них:
void BufferFBO_apply(BufferFBO *self) {
    if (!self || !self->_is_begin_) return;
    size_t len = Array_len(self->attachments);
    if (len == 0) {
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        return; // Нечего проверять: FBO ещё не сконфигурирован.
    }
    glDrawBuffers(len, (const uint32_t*)self->attachments->data);
    fbo_check_complete("apply");
}

// Скопировать цвет и глубину в другой кадровый буфер:
void BufferFBO_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height) {
    if (!self || !self->_is_begin_) return;
    GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    if (fbo_has_stencil(self->id) && fbo_has_stencil(dest_fbo_id)) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    fbo_blit(self, dest_fbo_id, x, y, width, height, mask);
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

// Скопировать только маску в другой кадровый буфер:
void BufferFBO_blit_stencil(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height) {
    if (!self || !self->_is_begin_) return;
    if (fbo_has_stencil(self->id) && fbo_has_stencil(dest_fbo_id)) {
        fbo_blit(self, dest_fbo_id, x, y, width, height, GL_STENCIL_BUFFER_BIT);
    }
}

// Привязать 2D текстуру:
void BufferFBO_attach(BufferFBO *self, BufferFBO_Type type, uint32_t attachment, uint32_t tex_id) {
    if (!self || !self->_is_begin_) return;

    switch (type) {
        // Привязываем глубину:
        case BUFFER_FBO_DEPTH: {
            glEnable(GL_DEPTH_TEST);  // Включаем тест глубины.
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_id, 0);
            fbo_check_complete("attach depth");
        } break;

        // Привязываем глубину и маску:
        case BUFFER_FBO_DEPTH_STENCIL: {
            if (tex_id == 0) {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo_id);
            } else {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex_id, 0);
            }
            fbo_check_complete("attach depth_stencil");
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
            fbo_check_complete("attach color");
        } break;
    }
}
