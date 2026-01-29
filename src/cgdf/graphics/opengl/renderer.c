//
// renderer.c - Реализует функционал рендерера.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "../renderer.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// Стандартные шейдеры рендеринга:
static const char* DEFAULT_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_model = mat4(1.0);\n\
uniform mat4 u_view = mat4(1.0);\n\
uniform mat4 u_proj = mat4(1.0);\n\
\n\
layout (location = 0) in vec3 a_position;\n\
layout (location = 1) in vec3 a_normal;\n\
layout (location = 2) in vec3 a_color;\n\
layout (location = 3) in vec2 a_texcoord;\n\
\n\
out vec2 v_texcoord;\n\
out vec3 v_normal;\n\
out vec3 v_color;\n\
\n\
void main(void) {\n\
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);\n\
    v_texcoord = a_texcoord;\n\
    v_normal = a_normal;\n\
    v_color = a_color;\n\
}\n\
";

static const char* DEFAULT_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform bool u_use_points = false;\n\
uniform bool u_use_texture;\n\
uniform bool u_use_normals;\n\
uniform bool u_use_vcolor;\n\
uniform vec4 u_color = vec4(1.0);\n\
uniform sampler2D u_texture;\n\
\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec3 v_color;\n\
out vec4 FragColor;\n\
\n\
void main(void) {\n\
    // Если мы используем точки для рисования:\n\
    if (u_use_points) {\n\
        vec2 coord = gl_PointCoord*2.0-1.0;\n\
        if (dot(coord, coord) > 1.0) discard;  // Отбрасываем всё за пределами круга.\n\
    }\n\
    // Если мы используем текстуру, рисуем с ней, иначе только цвет:\n\
    if (u_use_texture) {\n\
        FragColor = u_color * texture(u_texture, v_texcoord);\n\
    } else if (u_use_normals) {\n\
        FragColor = vec4(v_normal.rgb, 1.0);\n\
    } else if (u_use_vcolor) {\n\
        FragColor = vec4(v_color.rgb, 1.0);\n\
    } else {\n\
        FragColor = u_color;\n\
    }\n\
}\n\
";


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create() {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Заполняем поля:
    rnd->initialized = false;
    rnd->camera = NULL;
    rnd->shader = NULL;

    // Создаём шейдер:
    Shader *shader = Shader_create(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    if (!shader || Shader_get_error(shader)) {
        log_msg("[!] Error (from Renderer_create): Creating default shader failed: %s\n", shader->error);
        // Самоуничтожение при провале создания шейдера:
        Shader_destroy(&shader);
        mm_free(rnd);
        return NULL;
    }
    rnd->shader = shader;
    return rnd;
}

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd) {
    if (!rnd || !*rnd) return;

    // Уничтожение стеков буферов:
    BufferGC_GL_flush();
    BufferGC_GL_destroy();

    // Уничтожение текстурных юнитов:
    TextureUnits_destroy();

    // Освобождаем память шейдера:
    if ((*rnd)->shader) {
        Shader_destroy(&(*rnd)->shader);
    }

    // Освобождаем память рендерера:
    mm_free(*rnd);
    *rnd = NULL;
}

// Инициализация рендерера:
void Renderer_init(Renderer *self) {
    if (!self || self->initialized) return;

    gl_init();  // Инициализируем OpenGL.
    glEnable(GL_BLEND);  // Включаем смешивание цветов.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Устанавливаем режим смешивания.
    glEnable(GL_PROGRAM_POINT_SIZE);  // Разрешаем установку размера точки через шейдер.

    // Делаем нулевой текстурный юнит привязанным к нулевой текстуре:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Инициализация стеков буферов:
    BufferGC_GL_init();

    // Инициализация текстурных юнитов:
    TextureUnits_init();

    // Компилируем дефолтный шейдер:
    if (self->shader) Shader_compile(self->shader);

    // Поднимаем флаг инициализации:
    self->initialized = true;
}

// Освобождение буферов:
void Renderer_buffers_flush(Renderer *self) {
    if (!self) return;
    BufferGC_GL_flush();
}
