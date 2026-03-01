//
// renderer.c - Реализует функционал рендерера для OpenGL.
//


// Подключаем:
#include <SDL3/SDL.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "../vertex.h"
#include "../mesh.h"
#include "../camera.h"
#include "../renderer.h"
#include "../texture.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// Константы для NVIDIA (NVX_gpu_memory_info):
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_NVX 0x9049

// Константы для AMD (ATI_meminfo):
#define GL_VBO_FREE_MEMORY_ATI 0x87FB


// -------- Стандартные шейдеры рендеринга: --------


static const char* DEFAULT_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_model;\n\
uniform mat4 u_view;\n\
uniform mat4 u_proj;\n\
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
        log_msg("[E] Renderer_create: Creating shader failed: %s\n", shader->error);
    }
    return shader;
}

static void get_memory_info(int *total, int *used, int *free) {
    int total_kb = 0;
    int free_kb = 0;

    // Пытаемся через расширение NVIDIA:
    if (GLAD_GL_NVX_gpu_memory_info) {
        glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_NVX, &total_kb);
        glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_NVX, &free_kb);
    } 
    // Если первое не сработало, пробуем AMD:
    else if (GLAD_GL_ATI_meminfo) {
        int info[4]; 
        glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, info);
        free_kb = info[0];
        // Примечание: Расширение ATI не даёт узнать сколько всего видеопамяти и не даёт необходимой информации.
    }
    if (total) *total = total_kb;
    if (used) *used = total_kb < free_kb ? 0 : total_kb - free_kb;
    if (free) *free = free_kb;
}

static const char* dbg_severity(GLenum s) {
    switch (s) {
        case GL_DEBUG_SEVERITY_HIGH:   return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:    return "LOW";
        #ifdef GL_DEBUG_SEVERITY_NOTIFICATION
            case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFY";
        #endif
        default: return "UNKNOWN_SEVERITY";
    }
}

static const char* dbg_type(GLenum t) {
    switch (t) {
        case GL_DEBUG_TYPE_ERROR:               return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "UNDEFINED";
        case GL_DEBUG_TYPE_PORTABILITY:         return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:         return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:              return "MARKER";
        default: return "OTHER";
    }
}

// Debug callback:
static void APIENTRY gl_debug_cb(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam
) {
    (void)source; (void)type; (void)id; (void)length; (void)userParam;
    log_msg("[GL][%s][%s<id=%u>] %s\n", dbg_severity(severity), dbg_type(type), id, message ? message : "(null).");
}

static void gl_setup_debug_output(bool synchronous, bool notification, bool low, bool medium, bool high) {
    log_msg("[GL] OpenGL debug log level: [NOTIFY=%s, LOW=%s, MEDIUM=%s, HIGH=%s]\n",
        notification ? "ON" : "OFF", low ? "ON" : "OFF", medium ? "ON" : "OFF", high ? "ON" : "OFF"
    );

    // Core 4.3+ (современнее и стабильнее):
    if (GLAD_GL_VERSION_4_3) {
        glEnable(GL_DEBUG_OUTPUT);
        if (synchronous) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_cb, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, NULL, high);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, NULL, medium);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, NULL, low);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, notification);
        log_msg("[GL] OpenGL debug output enabled (CORE 4.3+).\n");
    }

    // Для старых драйверов:
    else if (GLAD_GL_ARB_debug_output) {
        if (synchronous) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_cb, NULL);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, NULL, high);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, NULL, medium);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, NULL, low);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, notification);
        log_msg("[GL] OpenGL debug output enabled (ARB DEBUG).\n");
    }
}


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create() {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Заполняем поля:
    rnd->initialized = false;
    rnd->info = (RendererInfo){ NULL };
    rnd->shader = NULL;
    rnd->shader_spritebatch2d = NULL;
    rnd->shader_light2d = NULL;
    rnd->camera = NULL;
    rnd->camera_type = RENDERER_CAMERA_2D;
    rnd->sprite_mesh = NULL;
    rnd->fallback_texture = NULL;

    // Создаём шейдеры:
    rnd->shader = create_shader(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    rnd->shader_spritebatch2d = create_shader(rnd, SPRITEBATCH_SHADER_VERT, SPRITEBATCH_SHADER_FRAG, NULL);
    rnd->shader_light2d = create_shader(rnd, LIGHT2D_SHADER_VERT, LIGHT2D_SHADER_FRAG, NULL);
    return rnd;
}

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd) {
    if (!rnd || !*rnd) return;

    // Освобождаем память шейдеров:
    if ((*rnd)->shader) { Shader_destroy(&(*rnd)->shader); }
    if ((*rnd)->shader_spritebatch2d) { Shader_destroy(&(*rnd)->shader_spritebatch2d); }
    if ((*rnd)->shader_light2d) { Shader_destroy(&(*rnd)->shader_light2d); }

    // Удаляем сетку спрайта:
    Mesh_destroy(&(*rnd)->sprite_mesh);

    // Удаляем текстуру заглушку:
    Texture_destroy(&(*rnd)->fallback_texture);

    // Уничтожение стеков буферов:
    BufferGC_GL_flush();
    BufferGC_GL_destroy();

    // Уничтожение текстурных юнитов:
    TextureUnits_destroy();

    // Освобождаем память рендерера:
    mm_free(*rnd);
    *rnd = NULL;
}

// Инициализация рендерера:
void Renderer_init(Renderer *self, bool renderer_debug) {
    if (!self || self->initialized) return;

    // Инициализируем OpenGL:
    if (gl_init()) {
        log_msg("[E] Renderer_init: Initializing OpenGL failed: %s\n", SDL_GetError());
        return;
    }

    // Получаем базовые данные рендеринга:
    int max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    self->info = (RendererInfo){
        .vendor   = (char*)glGetString(GL_VENDOR),
        .renderer = (char*)glGetString(GL_RENDERER),
        .version  = (char*)glGetString(GL_VERSION),
        .glsl     = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION),
        .max_texture_size = max_texture_size
    };

    // Инициализируем логирование OpenGL:
    if (renderer_debug) {
        gl_setup_debug_output(false, false, false, true, true);

        // Логируем как мы получаем данные о памяти:
        if (GLAD_GL_NVX_gpu_memory_info) log_msg("[GL] Used memory info: GL_NVX_gpu_memory_info\n");
        else if (GLAD_GL_ATI_meminfo)    log_msg("[GL] Used memory info: GL_ATI_meminfo\n");
        else log_msg("[GL] Memory info not supported.\n");

        // Логируем служебную информацию:
        log_msg("[GL] VENDOR: %s\n", Renderer_get_vendor(self));
        log_msg("[GL] RENDERER: %s\n", Renderer_get_renderer(self));
        log_msg("[GL] VERSION: %s\n", Renderer_get_version(self));
        log_msg("[GL] GLSL: %s\n", Renderer_get_glsl(self));
        log_msg("[GL] MAX TEXTURE SIZE: w%d x h%d px.\n", max_texture_size, max_texture_size);
        log_msg("[GL] TOTAL MEMORY: %d MB\n", Renderer_get_total_memory(self)/1024);
        log_msg("[GL] USED MEMORY: %d MB\n", Renderer_get_used_memory(self)/1024);
        log_msg("[GL] FREE MEMORY: %d MB\n", Renderer_get_free_memory(self)/1024);
    }

    glEnable(GL_BLEND);  // Включаем смешивание цветов.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Устанавливаем режим смешивания.
    glEnable(GL_PROGRAM_POINT_SIZE);  // Разрешаем установку размера точки через шейдер.

    /* ВЫРЕЗАНО:
    // Включаем сглаживание линий только если драйвер сообщает поддержку:
    float smooth_line_range[2] = {0.0f, 0.0f};
    glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, smooth_line_range);
    if (smooth_line_range[1] > 0.0f) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);  // Просим использовать максимально качественное сглаживание.
    }
    */

    // Инициализация стеков буферов:
    BufferGC_GL_init();

    // Компилируем шейдеры:
    if (self->shader) Shader_compile(self->shader);
    if (self->shader_spritebatch2d) Shader_compile(self->shader_spritebatch2d);
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

    // Текстура-заглушка:
    self->fallback_texture = Texture_create(self);
    Texture_empty(self->fallback_texture, 1, 1, false, TEX_RGBA8, TEX_DATA_UBYTE);  // 1x1 пустая текстура.

    // Инициализация текстурных юнитов:
    TextureUnits_init(self);

    // Поднимаем флаг инициализации:
    self->initialized = true;
    log_msg("[I] OpenGL initialized.\n");
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
    Shader_clear_caches(self->shader_spritebatch2d);
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

// Получить производителя видеокарты:
const char* Renderer_get_vendor(Renderer *self) {
    if (!self) return "null";
    return self->info.vendor;
}

// Получить название видеокарты:
const char* Renderer_get_renderer(Renderer *self) {
    if (!self) return "null";
    return self->info.renderer;
}

// Получить версию драйвера:
const char* Renderer_get_version(Renderer *self) {
    if (!self) return "null";
    return self->info.version;
}

// Получить версию шейдерного языка:
const char* Renderer_get_glsl(Renderer *self) {
    if (!self) return "null";
    return self->info.glsl;
}

// Получить максимальный размер текстуры:
int Renderer_get_max_texture_size(Renderer *self) {
    if (!self) return 0;
    return self->info.max_texture_size;
}

// Получить сколько всего видеопамяти есть (в килобайтах):
int Renderer_get_total_memory(Renderer *self) {
    if (!self) return 0;
    int total;
    get_memory_info(&total, NULL, NULL);
    return total;
}

// Сколько используется видеопамяти (в килобайтах):
int Renderer_get_used_memory(Renderer *self) {
    if (!self) return 0;
    int used;
    get_memory_info(NULL, &used, NULL);
    return used;
}

// Сколько свободно видеопамяти (в килобайтах):
int Renderer_get_free_memory(Renderer *self) {
    if (!self) return 0;
    int free;
    get_memory_info(NULL, NULL, &free);
    return free;
}
