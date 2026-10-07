//
// gl.h - Просто подключаем функционал OpenGL.
//

#pragma once

// Подключаем:
#include <stddef.h>
#include "glad/glad.h"


// Константы для NVIDIA (NVX_gpu_memory_info):
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_NVX 0x9049

// Константы для AMD (ATI_meminfo):
#define GL_VBO_FREE_MEMORY_ATI 0x87FB

// Константы для буфера логов OpenGL:
#define GL_DEBUG_SEEN_MAX 64  // Храним последние N сообщений. Если новые будут совпадать со старыми, их не выводим.


// Инициализация OpenGL (true = ошибка, false = успех):
static inline bool gl_init(void) {
    return !gladLoadGL();
}
