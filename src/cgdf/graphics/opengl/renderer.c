//
// renderer.c - Реализует функционал рендерера для OpenGL.
//


// Подключаем:
#include <SDL3/SDL.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "../core/vertex.h"
#include "../core/mesh.h"
#include "../core/camera.h"
#include "../core/renderer.h"
#include "../core/texture.h"
#include "shaders/default_shader.h"
#include "shaders/model_shader.h"
#include "shaders/spritebatch_shader.h"
#include "shaders/light2d_shader.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// Константы для NVIDIA (NVX_gpu_memory_info):
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_NVX 0x9049

// Константы для AMD (ATI_meminfo):
#define GL_VBO_FREE_MEMORY_ATI 0x87FB


// Глобальная конфигурация дебага рендеринга:
RendererDebugConfig Renderer_debug_config = {
    .debug_enabled = false,
    .sync = false,
    .level_notify = false,
    .level_low = false,
    .level_medium = true,
    .level_high = true
};


// -------- Вспомогательные функции: --------


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
    static GLuint last_id = 0;
    static GLenum last_type = 0;
    static GLenum last_severity = 0;
    static char last_msg[1024] = {0};

    const char *msg = message ? message : "(null)";

    // Если лог полностью такой же, не выводим:
    if (id == last_id && type == last_type && severity == last_severity && strcmp(msg, last_msg) == 0) return;

    last_id = id;
    last_type = type;
    last_severity = severity;
    snprintf(last_msg, sizeof(last_msg), "%s", msg);

    log_msg("[GL][%s][%s<id=%u>] %s\n", dbg_severity(severity), dbg_type(type), id, msg);
}

static void gl_setup_debug_output(bool sync, bool notify, bool low, bool medium, bool high) {
    log_msg("[GL] OpenGL debug log level: [NOTIFY=%s, LOW=%s, MEDIUM=%s, HIGH=%s]\n",
        notify ? "ON" : "OFF", low ? "ON" : "OFF", medium ? "ON" : "OFF", high ? "ON" : "OFF"
    );

    // Core 4.3+ (современнее и стабильнее):
    if (GLAD_GL_VERSION_4_3) {
        glEnable(GL_DEBUG_OUTPUT);
        if (sync) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_cb, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, NULL, high);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, NULL, medium);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, NULL, low);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, notify);
        log_msg("[GL] OpenGL debug output enabled (CORE 4.3+).\n");
    }

    // Для старых драйверов:
    else if (GLAD_GL_ARB_debug_output) {
        if (sync) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB((GLDEBUGPROCARB)gl_debug_cb, NULL);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, NULL, high);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, NULL, medium);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, NULL, low);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, notify);
        log_msg("[GL] OpenGL debug output enabled (ARB DEBUG).\n");
    }
}

static inline Shader* create_shader(Renderer *rnd, const char *vert, const char *frag, const char *geom) {
    Shader *shader = Shader_create(rnd, vert, frag, geom);
    if (!shader || Shader_get_error(shader)) {
        log_msg("[E] Renderer_create: Creating shader failed: %s\n", shader->error);
    }
    return shader;
}

// Создать шейдеры:
static inline void create_shaders(Renderer *rnd) {
    rnd->shader             = create_shader(rnd, DEFAULT_SHADER_VERT, DEFAULT_SHADER_FRAG, NULL);
    rnd->shader_model       = create_shader(rnd, MODEL_SHADER_VERT, MODEL_SHADER_FRAG, NULL);
    rnd->shader_spritebatch = create_shader(rnd, SPRITEBATCH_SHADER_VERT, SPRITEBATCH_SHADER_FRAG, NULL);
    rnd->shader_light2d     = create_shader(rnd, LIGHT2D_SHADER_VERT, LIGHT2D_SHADER_FRAG, NULL);
}

// Скомпилировать шейдеры:
static inline void compile_shaders(Renderer *rnd) {
    if (rnd->shader)             Shader_compile(rnd->shader);
    if (rnd->shader_model)       Shader_compile(rnd->shader_model);
    if (rnd->shader_spritebatch) Shader_compile(rnd->shader_spritebatch);
    if (rnd->shader_light2d)     Shader_compile(rnd->shader_light2d);
}

// Освободить шейдеры:
static inline void destroy_shaders(Renderer *rnd) {
    if (rnd->shader)             Shader_destroy(&rnd->shader);
    if (rnd->shader_model)       Shader_destroy(&rnd->shader_model);
    if (rnd->shader_spritebatch) Shader_destroy(&rnd->shader_spritebatch);
    if (rnd->shader_light2d)     Shader_destroy(&rnd->shader_light2d);
}


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create(void) {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Заполняем поля:
    rnd->initialized = false;
    rnd->info = (RendererInfo){ NULL };
    rnd->camera = NULL;
    rnd->camera_type = RENDERER_CAMERA_2D;
    rnd->sprite_mesh = NULL;
    rnd->fallback_texture = NULL;

    rnd->shader = NULL;
    rnd->shader_model = NULL;
    rnd->shader_spritebatch = NULL;
    rnd->shader_light2d = NULL;

    // Создаём шейдеры:
    create_shaders(rnd);
    return rnd;
}

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd) {
    if (!rnd || !*rnd) return;

    // Освобождаем память шейдеров:
    destroy_shaders(*rnd);

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
void Renderer_init(Renderer *self) {
    if (!self || self->initialized) return;

    // Инициализируем OpenGL:
    if (gl_init()) {
        log_msg("[E] Renderer_init: Initializing OpenGL failed: %s\n", SDL_GetError());
        self->initialized = false;
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
    if (Renderer_debug_config.debug_enabled) {
        gl_setup_debug_output(
            Renderer_debug_config.sync,
            Renderer_debug_config.level_notify,
            Renderer_debug_config.level_low,
            Renderer_debug_config.level_medium,
            Renderer_debug_config.level_high
        );

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
    compile_shaders(self);

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
        false, NULL
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

// Установить проверку глубины:
void Renderer_set_depth_test(Renderer *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

// Включить или отключить запись глубины:
void Renderer_set_depth_mask(Renderer *self, bool enabled) {
    if (!self) return;
    enabled ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
}

// Включить или отключить смешивание:
void Renderer_set_blending(Renderer *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

// Установить отсечение граней:
void Renderer_set_cull_faces(Renderer *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
}

// Отсекать только задние грани:
void Renderer_set_back_face_culling(Renderer *self) {
    if (!self) return;
    glCullFace(GL_BACK);
}

// Отсекать только передние грани:
void Renderer_set_front_face_culling(Renderer *self) {
    if (!self) return;
    glCullFace(GL_FRONT);
}

// Передняя грань против часовой стрелки (CCW):
void Renderer_set_front_face_onleft(Renderer *self) {
    if (!self) return;
    glFrontFace(GL_CCW);  // Против часовой стрелки.
}

// Передняя грань по часовой стрелке (CW):
void Renderer_set_front_face_onright(Renderer *self) {
    if (!self) return;
    glFrontFace(GL_CW);  // По часовой стрелке.
}

// Установить размер viewport:
void Renderer_set_viewport(Renderer *self, int x, int y, int width, int height) {
    if (!self) return;
    glViewport(x, y, width, height);
}
