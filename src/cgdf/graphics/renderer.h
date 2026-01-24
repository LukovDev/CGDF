//
// renderer.h - Создаёт общий апи для работы с рендерером.
//
// В Renderer объявляются различные функции и поля, для работы с графикой.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
// #include "shader.h"


// Объявление структур:
typedef struct Renderer Renderer;  // Рендерер.


// Рендерер:
struct Renderer {
    bool initialized;       // Флаг инициализации контекста OpenGL.
    // ShaderProgram *shader;  // Дефолтная шейдерная программа.
    // void          *camera;  // Текущая активная камера.
};


// Создать рендерер:
Renderer* Renderer_create(void);

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd);


// -------- API рендерера: --------


// Инициализация рендерера:
void Renderer_init(Renderer *self);

// Освобождение буферов:
void Renderer_buffers_flush(Renderer *self);
