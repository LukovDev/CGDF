//
// renderer.c - Реализует функционал рендерера.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
// #include "buffer_gc.h"
// #include "texunit.h"
#include "../renderer.h"
#include "gl.h"


// Создать рендерер:
Renderer* Renderer_create() {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Заполняем поля:
    rnd->initialized = false;
    // rnd->shader = NULL;
    // rnd->camera = NULL;

    // // Создаём шейдер:
    // ShaderProgram *shader = ShaderProgram_create(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    // if (!shader || shader->get_error(shader)) {
    //     fprintf(stderr, "RENDERER_GL-ERROR: Creating default shader failed: %s\n", shader->error);
    //     // Самоуничтожение при провале создания шейдера:
    //     ShaderProgram_destroy(&shader);
    //     mm_free(rnd);
    //     return NULL;
    // }
    // rnd->shader = shader;

    return rnd;
}

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd) {
    if (!rnd || !*rnd) return;

    // Уничтожение стеков буферов:
    // BufferGC_GL_flush();
    // BufferGC_GL_destroy();

    // Уничтожение текстурных юнитов:
    // TextureUnits_destroy();

    // Освобождаем память шейдера:
    // if ((*rnd)->shader) {
    //     ShaderProgram_destroy(&(*rnd)->shader);
    // }

    // Освобождаем память рендерера:
    mm_free(*rnd);
    *rnd = NULL;
}


// -------- API рендерера: --------


// Инициализация рендерера:
void Renderer_init(Renderer *self) {
    gl_init();  // Инициализируем OpenGL.
    glEnable(GL_BLEND);  // Включаем смешивание цветов.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Устанавливаем режим смешивания.
    glEnable(GL_PROGRAM_POINT_SIZE);  // Разрешаем установку размера точки через шейдер.

    // Делаем нулевой текстурный юнит привязанным к нулевой текстуре:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Поднимаем флаг инициализации:
    self->initialized = true;

    // Инициализация стеков буферов:
    // BufferGC_GL_init();

    // Инициализация текстурных юнитов:
    // TextureUnits_init();

    // Компилируем дефолтный шейдер:
    // self->shader->compile(self->shader);
}

// Освобождение буферов:
void Renderer_buffers_flush(Renderer *self) {
    if (!self) return;
}
