//
// gl.h - Просто подключаем функционал OpenGL.
//

#pragma once

// Подключаем:
#include "glad/glad.h"

// Инициализация OpenGL:
static inline bool gl_init(void) {
    return !gladLoadGL();
}
