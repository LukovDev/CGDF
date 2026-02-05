//
// renderer.c - Реализует функционал рендерера для OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "../vertex.h"
#include "../mesh.h"
#include "../renderer.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// -------- Стандартные шейдеры рендеринга: --------


static const char* DEFAULT_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_model = mat4(1.0);\n\
uniform mat4 u_view = mat4(1.0);\n\
uniform mat4 u_proj = mat4(1.0);\n\
layout (location = 0) in vec3 a_position;\n\
layout (location = 1) in vec3 a_normal;\n\
layout (location = 2) in vec4 a_color;\n\
layout (location = 3) in vec2 a_texcoord;\n\
out vec2 v_texcoord;\n\
out vec3 v_normal;\n\
out vec4 v_color;\n\
\n\
void main(void) {\n\
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0);\n\
    v_texcoord = a_texcoord;\n\
    v_normal = a_normal;\n\
    v_color = a_color;\n\
}";

static const char* DEFAULT_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform bool u_use_points = false;\n\
uniform bool u_use_texture;\n\
uniform bool u_use_normals;\n\
uniform bool u_use_vcolor;\n\
uniform vec4 u_color = vec4(1.0);\n\
uniform sampler2D u_texture;\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec4 v_color;\n\
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
        FragColor = v_color;\n\
    } else {\n\
        FragColor = u_color;\n\
    }\n\
}";


// -------- Шейдеры пакетной отрисовки спрайтов: --------


static const char* SPRITEBATCH_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_view;\n\
uniform mat4 u_proj;\n\
layout(location = 0) in vec2 a_position;\n\
layout(location = 1) in vec2 a_texcoord;\n\
layout(location = 2) in vec4 a_color;\n\
out vec2 v_texcoord;\n\
out vec4 v_color;\n\
\n\
void main() {\n\
    gl_Position = u_proj * u_view * vec4(a_position, 0.0, 1.0);\n\
    v_texcoord = a_texcoord;\n\
    v_color = a_color;\n\
}";

static const char* SPRITEBATCH_SHADER_FRAG = "\
#version 330 core\n\
uniform bool u_use_texture;\n\
uniform sampler2D u_texture;\n\
in vec2 v_texcoord;\n\
in vec4 v_color;\n\
out vec4 FragColor;\n\
\n\
void main() {\n\
    vec4 color = v_color;\n\
    if (u_use_texture) {\n\
        color *= texture(u_texture, v_texcoord);\n\
    }\n\
    FragColor = color;\n\
}";


// -------- Данные по умолчанию: --------


// Квадрат с текстурой для спрайта:
static const float _texcoord_[] = {0.0f, 0.0f, 1.0f, 1.0f};
static const Vertex _sprite_vertices_[] = {
    // vertex, normal, color, texcoord.
    {-1,-1,0,  0,0,0,  1,1,1,1,  _texcoord_[0], _texcoord_[3]},  // 0.
    {+1,-1,0,  0,0,0,  1,1,1,1,  _texcoord_[2], _texcoord_[3]},  // 1.
    {+1,+1,0,  0,0,0,  1,1,1,1,  _texcoord_[2], _texcoord_[1]},  // 2.
    {-1,+1,0,  0,0,0,  1,1,1,1,  _texcoord_[0], _texcoord_[1]}   // 3.
};
static const uint32_t _sprite_indices_[] = {
    0, 1, 2,  // Triangle 1.
    2, 3, 0   // Triangle 2.
};


// -------- Вспомогательные функции: --------


static inline Shader* create_shader(Renderer *rnd, const char *vert, const char *frag, const char *geom) {
    Shader *shader = Shader_create(rnd, vert, frag, geom);
    if (!shader || Shader_get_error(shader)) {
        log_msg("[!] Error (from Renderer_create): Creating shader failed: %s\n", shader->error);
    }
    return shader;
}


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create() {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Заполняем поля:
    rnd->initialized = false;
    rnd->camera = NULL;
    rnd->shader = NULL;
    rnd->sprite_mesh = NULL;

    // Создаём шейдеры:
    rnd->shader = create_shader(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    rnd->shader_spritebatch = create_shader(rnd, SPRITEBATCH_SHADER_VERT, SPRITEBATCH_SHADER_FRAG, NULL);
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

    // Освобождаем память шейдеров:
    if ((*rnd)->shader) { Shader_destroy(&(*rnd)->shader); }
    if ((*rnd)->shader_spritebatch) { Shader_destroy(&(*rnd)->shader_spritebatch); }

    // Удаляем сетку спрайта:
    Mesh_destroy(&(*rnd)->sprite_mesh);

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

    // Включаем сглаживание линий только если драйвер сообщает поддержку:
    float smooth_line_range[2] = {0.0f, 0.0f};
    glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, smooth_line_range);
    if (smooth_line_range[1] > 0.0f) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);  // Просим использовать максимально качественное сглаживание.
    }

    // Делаем нулевой текстурный юнит привязанным к нулевой текстуре:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Инициализация стеков буферов:
    BufferGC_GL_init();

    // Инициализация текстурных юнитов:
    TextureUnits_init();

    // Компилируем дефолтный шейдер:
    if (self->shader) Shader_compile(self->shader);

    // Компилируем шейдер пакетной отрисовки спрайтов:
    if (self->shader_spritebatch) Shader_compile(self->shader_spritebatch);

    // Создаём сетку спрайта:
    self->sprite_mesh = Mesh_create(
        _sprite_vertices_, sizeof(_sprite_vertices_)/sizeof(Vertex),
        _sprite_indices_, sizeof(_sprite_indices_)/sizeof(uint32_t),
        false
    );

    // На всякий случай, насильно отправляем инструкции видеокарте и ждём ответа:
    glFinish();

    // Поднимаем флаг инициализации:
    self->initialized = true;
}

// Освобождение буферов:
void Renderer_buffers_flush(Renderer *self) {
    if (!self) return;
    BufferGC_GL_flush();
}
