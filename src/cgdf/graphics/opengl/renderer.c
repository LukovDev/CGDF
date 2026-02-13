//
// renderer.c - Реализует функционал рендерера для OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "../vertex.h"
#include "../mesh.h"
#include "../camera.h"
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
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0f);\n\
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
        vec2 coord = gl_PointCoord*2.0f-1.0f;\n\
        if (dot(coord, coord) > 1.0f) discard;  // Отбрасываем всё за пределами круга.\n\
    }\n\
    // Если мы используем текстуру, рисуем с ней, иначе только цвет:\n\
    if (u_use_texture) {\n\
        FragColor = u_color * texture(u_texture, v_texcoord);\n\
    } else if (u_use_normals) {\n\
        FragColor = vec4(v_normal.rgb, 1.0f);\n\
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
layout (location = 0) in vec2 a_position;\n\
layout (location = 1) in vec2 a_texcoord;\n\
layout (location = 2) in vec4 a_color;\n\
out vec2 v_texcoord;\n\
out vec4 v_color;\n\
\n\
void main() {\n\
    gl_Position = u_proj * u_view * vec4(a_position, 0.0f, 1.0f);\n\
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


// -------- Шейдеры 2D освещения: --------


static const char* LIGHT2D_SHADER_VERT = "\
#version 330 core\n\
\n\
layout (location = 0) in vec3 a_position;\n\
\n\
void main() {\n\
    gl_Position = vec4(a_position, 1.0f);\n\
}";

static const char* LIGHT2D_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform sampler2D u_albedo_texture;\n\
uniform sampler2D u_light_texture;\n\
uniform vec3      u_ambient;\n\
uniform float     u_intensity;\n\
uniform vec2      u_resolution;\n\
\n\
out vec4 FragColor;\n\
\n\
void main(void) {\n\
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;\n\
    vec4 albedo = texture(u_albedo_texture, uv);\n\
    vec3 light  = texture(u_light_texture, uv).rgb;\n\
    \n\
    // 2D lighting: final = albedo * (ambient + light * intensity):\n\
    vec3 lit = albedo.rgb * (u_ambient + light * max(u_intensity, 0.0));\n\
    FragColor = vec4(lit, albedo.a);\n\
}";


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
    rnd->shader = NULL;
    rnd->shader_spritebatch = NULL;
    rnd->shader_light2d = NULL;
    rnd->camera = NULL;
    rnd->camera_type = RENDERER_CAMERA_2D;
    rnd->sprite_mesh = NULL;

    // Создаём шейдеры:
    rnd->shader = create_shader(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    rnd->shader_spritebatch = create_shader(rnd, SPRITEBATCH_SHADER_VERT, SPRITEBATCH_SHADER_FRAG, NULL);
    rnd->shader_light2d = create_shader(rnd, LIGHT2D_SHADER_VERT, LIGHT2D_SHADER_FRAG, NULL);
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
    if ((*rnd)->shader_light2d) { Shader_destroy(&(*rnd)->shader_light2d); }

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

    // Компилируем шейдеры:
    if (self->shader) Shader_compile(self->shader);
    if (self->shader_spritebatch) Shader_compile(self->shader_spritebatch);
    if (self->shader_light2d) Shader_compile(self->shader_light2d);

    // Квадрат с текстурой для спрайта:
    const float texcoord[] = {0.0f, 0.0f, 1.0f, 1.0f};
    const Vertex sprite_vertices[] = {
        // vertex, normal, color, texcoord.
        {-1,-1,0,  0,0,0,  1,1,1,1,  texcoord[0], texcoord[3]},  // 0.
        {+1,-1,0,  0,0,0,  1,1,1,1,  texcoord[2], texcoord[3]},  // 1.
        {+1,+1,0,  0,0,0,  1,1,1,1,  texcoord[2], texcoord[1]},  // 2.
        {-1,+1,0,  0,0,0,  1,1,1,1,  texcoord[0], texcoord[1]}   // 3.
    };
    const uint32_t sprite_indices[] = {
        0, 1, 2,  // Triangle 1.
        2, 3, 0   // Triangle 2.
    };

    // Создаём сетку спрайта:
    self->sprite_mesh = Mesh_create(
        sprite_vertices, sizeof(sprite_vertices)/sizeof(Vertex),
        sprite_indices, sizeof(sprite_indices)/sizeof(uint32_t),
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

// Освобождаем кэши:
void Renderer_clear_caches(Renderer *self) {
    if (!self) return;
    // Освобождаем текстурные юниты:
    TexUnits_unbind_all();

    // Освобождаем кэши в шейдерах:
    Shader_clear_caches(self->shader);
    Shader_clear_caches(self->shader_spritebatch);
    Shader_clear_caches(self->shader_light2d);
}

// Получить матрицу вида камеры:
void Renderer_get_view(Renderer *self, mat4 view) {
    glm_mat4_identity(view);
    if (!self || !self->camera) return;
    if (self->camera_type == RENDERER_CAMERA_2D) {
        glm_mat4_copy(((Camera2D*)self->camera)->view, view);
    } else if (self->camera_type == RENDERER_CAMERA_3D) {
        glm_mat4_copy(((Camera3D*)self->camera)->view, view);
    }
}

// Получить матрицу проекции камеры:
void Renderer_get_proj(Renderer *self, mat4 proj) {
    glm_mat4_identity(proj);
    if (!self || !self->camera) return;
    if (self->camera_type == RENDERER_CAMERA_2D) {
        glm_mat4_copy(((Camera2D*)self->camera)->proj, proj);
    } else if (self->camera_type == RENDERER_CAMERA_3D) {
        glm_mat4_copy(((Camera3D*)self->camera)->proj, proj);
    }
}

// Получить матрицу вида и проекции камеры:
void Renderer_get_view_proj(Renderer *self, mat4 view, mat4 proj) {
    Renderer_get_view(self, view);
    Renderer_get_proj(self, proj);
}

// Получить ширину камеры:
int Renderer_get_width(Renderer *self) {
    if (!self || !self->camera) return 0;
    if (self->camera_type == RENDERER_CAMERA_2D) {
        return ((Camera2D*)self->camera)->width;
    } else if (self->camera_type == RENDERER_CAMERA_3D) {
        return ((Camera3D*)self->camera)->width;
    }
    return 0;
}

// Получить высоту камеры:
int Renderer_get_height(Renderer *self) {
    if (!self || !self->camera) return 0;
    if (self->camera_type == RENDERER_CAMERA_2D) {
        return ((Camera2D*)self->camera)->height;
    } else if (self->camera_type == RENDERER_CAMERA_3D) {
        return ((Camera3D*)self->camera)->height;
    }
    return 0;
}
