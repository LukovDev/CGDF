//
// buffer_gc.h
//

#pragma once


// Подключаем:
#include <cgdf/core/array.h>


// Определения:
#define BUFFER_GC_GL_START_CAPACITY 1024


// Типы буферов на уничтожение:
typedef enum BufferGC_GL_Type {
    BGC_GL_QBO,
    BGC_GL_SSBO,
    BGC_GL_FBO,
    BGC_GL_RBO,
    BGC_GL_VBO,
    BGC_GL_EBO,
    BGC_GL_VAO,
    BGC_GL_TBO,
    // ...
} BufferGC_GL_Type;


// Объявление структур:
typedef struct BufferGC_GL BufferGC_GL;


// Единица кэша локаций юниформов:
struct BufferGC_GL {
    Array *qbo;
    Array *ssbo;
    Array *fbo;
    Array *rbo;
    Array *vbo;
    Array *ebo;
    Array *vao;
    Array *tbo;
    // ...
};


// Создаём единую глобальную структуру:
extern BufferGC_GL buffer_gc_gl;


// Инициализация стеков буферов:
void BufferGC_GL_init();

// Уничтожение стеков буферов:
void BufferGC_GL_destroy();

// Добавить буфер на уничтожение:
void BufferGC_GL_push(BufferGC_GL_Type type, unsigned int id);

// Очистка всех буферов:
void BufferGC_GL_flush();
