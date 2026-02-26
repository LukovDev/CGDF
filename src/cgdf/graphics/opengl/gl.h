//
// gl.h - Просто подключаем функционал OpenGL.
//

#pragma once

// Подключаем:
#include "glad/glad.h"

// Инициализация OpenGL (true = ошибка, false = успех):
static inline bool gl_init() {
    return !gladLoadGL();
}
