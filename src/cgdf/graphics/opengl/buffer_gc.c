//
// buffer_gc.c - Создаёт код для отслеживания буферов OpenGL, которые должны быть уничтожены разом.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>
#include <cgdf/core/mm.h>
#include "gl.h"
#include "buffer_gc.h"


// Создаём единую глобальную структуру:
BufferGC_GL buffer_gc_gl = {0};


// Найти максимальный размер стеков:
static inline size_t find_max_stacks_len() {
    size_t max_stack_len = 0;
    if (Array_len(buffer_gc_gl.qbo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.qbo);
    if (Array_len(buffer_gc_gl.ssbo) > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.ssbo);
    if (Array_len(buffer_gc_gl.fbo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.fbo);
    if (Array_len(buffer_gc_gl.rbo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.rbo);
    if (Array_len(buffer_gc_gl.vbo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.vbo);
    if (Array_len(buffer_gc_gl.ebo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.ebo);
    if (Array_len(buffer_gc_gl.vao)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.vao);
    if (Array_len(buffer_gc_gl.tbo)  > max_stack_len) max_stack_len = Array_len(buffer_gc_gl.tbo);
    // ...
    return max_stack_len;
}


// Очистить стек:
static inline void stack_flush(Array *stack) {
    Array_clear(stack, false);
    Array_shrink(stack, ARRAY_SHRINK_FACTOR);
}


// Инициализация стеков буферов:
void BufferGC_GL_init() {
    buffer_gc_gl.qbo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.ssbo = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.fbo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.rbo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.vbo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.ebo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.vao  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    buffer_gc_gl.tbo  = Array_create(sizeof(uint32_t), BUFFER_GC_GL_START_CAPACITY);
    // ...
}


// Уничтожение стеков буферов:
void BufferGC_GL_destroy() {
    Array_destroy(&buffer_gc_gl.qbo);
    Array_destroy(&buffer_gc_gl.ssbo);
    Array_destroy(&buffer_gc_gl.fbo);
    Array_destroy(&buffer_gc_gl.rbo);
    Array_destroy(&buffer_gc_gl.vbo);
    Array_destroy(&buffer_gc_gl.ebo);
    Array_destroy(&buffer_gc_gl.vao);
    Array_destroy(&buffer_gc_gl.tbo);
    // ...
}


// Добавить буфер на уничтожение:
void BufferGC_GL_push(BufferGC_GL_Type type, unsigned int id) {
    switch (type) {
        case BGC_GL_QBO:  Array_push(buffer_gc_gl.qbo,  &id); break;
        case BGC_GL_SSBO: Array_push(buffer_gc_gl.ssbo, &id); break;
        case BGC_GL_FBO:  Array_push(buffer_gc_gl.fbo,  &id); break;
        case BGC_GL_RBO:  Array_push(buffer_gc_gl.rbo,  &id); break;
        case BGC_GL_VBO:  Array_push(buffer_gc_gl.vbo,  &id); break;
        case BGC_GL_EBO:  Array_push(buffer_gc_gl.ebo,  &id); break;
        case BGC_GL_VAO:  Array_push(buffer_gc_gl.vao,  &id); break;
        case BGC_GL_TBO:  Array_push(buffer_gc_gl.tbo,  &id); break;
        // ...
    }
}


// Очистка всех буферов:
void BufferGC_GL_flush() {
    // Находим максимальный размер стека буферов:
    size_t max_stack_len = find_max_stacks_len();

    // Если нет буферов -> выходим:
    if (max_stack_len == 0) return;

    // Очищаем стек буферов QBO:
    if (Array_len(buffer_gc_gl.qbo) > 0) {
        glDeleteQueries(Array_len(buffer_gc_gl.qbo), (uint32_t*)buffer_gc_gl.qbo->data);
        stack_flush(buffer_gc_gl.qbo);
    }
    // Очищаем стек буферов SSBO:
    if (Array_len(buffer_gc_gl.ssbo) > 0) {
        glDeleteBuffers(Array_len(buffer_gc_gl.ssbo), (uint32_t*)buffer_gc_gl.ssbo->data);
        stack_flush(buffer_gc_gl.ssbo);
    }
    // Очищаем стек буферов FBO:
    if (Array_len(buffer_gc_gl.fbo) > 0) {
        glDeleteFramebuffers(Array_len(buffer_gc_gl.fbo), (uint32_t*)buffer_gc_gl.fbo->data);
        stack_flush(buffer_gc_gl.fbo);
    }
    // Очищаем стек буферов RBO:
    if (Array_len(buffer_gc_gl.rbo) > 0) {
        glDeleteRenderbuffers(Array_len(buffer_gc_gl.rbo), (uint32_t*)buffer_gc_gl.rbo->data);
        stack_flush(buffer_gc_gl.rbo);
    }
    // Очищаем стек буферов VBO:
    if (Array_len(buffer_gc_gl.vbo) > 0) {
        glDeleteBuffers(Array_len(buffer_gc_gl.vbo), (uint32_t*)buffer_gc_gl.vbo->data);
        stack_flush(buffer_gc_gl.vbo);
    }
    // Очищаем стек буферов EBO:
    if (Array_len(buffer_gc_gl.ebo) > 0) {
        glDeleteBuffers(Array_len(buffer_gc_gl.ebo), (uint32_t*)buffer_gc_gl.ebo->data);
        stack_flush(buffer_gc_gl.ebo);
    }
    // Очищаем стек буферов VAO:
    if (Array_len(buffer_gc_gl.vao) > 0) {
        glDeleteVertexArrays(Array_len(buffer_gc_gl.vao), (uint32_t*)buffer_gc_gl.vao->data);
        stack_flush(buffer_gc_gl.vao);
    }
    // Очищаем стек буферов TBO:
    if (Array_len(buffer_gc_gl.tbo) > 0) {
        glDeleteTextures(Array_len(buffer_gc_gl.tbo), (uint32_t*)buffer_gc_gl.tbo->data);
        stack_flush(buffer_gc_gl.tbo);
    }

    // ...
}
