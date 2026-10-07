//
// renderer.c - Реализует функционал рендерера для OpenGL.
//


// Подключаем:
#include <SDL3/SDL.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
#include "../core/gbuffer.h"
#include "../core/light.h"
#include "../core/mesh.h"
#include "../core/camera.h"
#include "../core/shader.h"
#include "../core/texture.h"
#include "../core/vertex.h"
#include "../core/renderer.h"
#include "buffers/buffers.h"
#include "shaders/embeds_shaders.h"
#include "buffer_gc.h"
#include "texunit.h"
#include "gl.h"


// Глобальная конфигурация дебага рендеринга:
RendererDebugConfig g_Renderer_debug_config = {
    .debug_enabled = false,
    .sync = false,
    .level_notify = false,
    .level_low = false,
    .level_medium = true,
    .level_high = true
};


// -------- Вспомогательные структуры: --------


// Структура отладочного сообщения:
typedef struct GLDebugSeenMsg {
    GLuint id;
    GLenum type;
    GLenum severity;
    char msg[1024];
} GLDebugSeenMsg;


// Одна команда отрисовки. Что рисовать, чем и где. Всё копируется в момент вызова:
typedef struct DrawCommand {
    Mesh *mesh;          // Сетка.
    Material *material;  // Материал на момент вызова (а не тот, что будет у сетки потом).
    mat4 transform;      // Матрица модели (копия).
    bool cast_shadow;    // Отбрасывать ли тени.
    bool wireframe;      // Отображать в виде сетки.
} DrawCommand;


// Структура освещения в 3D:
typedef struct Lighting3D {
    Renderer  *renderer;
    BufferFBO *light_fbo;  // Буфер кадра освещения.
    Texture   *light_tex;  // Результат освещения (HDR, RGBA16F).
    // Настройки освещения:
    Vec3f sun_direction;      // Направление солнца.
    Vec3f sun_color;          // Цвет солнца.
    float sun_intensity;      // Интенсивность солнца.
    Vec3f ambient_color;      // Цвет фонового освещения.
    float ambient_intensity;  // Интенсивность фонового освещения.
    // Настройки теней:
    bool      shadows_enabled;   // Включить тени.
    bool      shadows_smooth;    // Мягкие тени.
    uint32_t  shadows_size;      // Размер карты теней (в пикселях, квадрат).
    float     shadows_distance;  // Радиус области вокруг камеры, где считаются тени (в единицах мира).
    BufferFBO *shadows_fbo;      // Буфер кадра карты теней (только глубина).
    Texture   *shadows_tex;      // Карта теней (текстура глубины).
    float shadows_normal_bias;   // Смещение тени по нормали (в текселях теней).
    float shadows_depth_bias;    // Постоянное смещение глубины (в текселях теней).
    float shadows_slope_bias;    // Наклонное смещение глубины (в текселях теней).
} Lighting3D;


// -------- Вспомогательные функции: --------


static void _get_memory_info_(int *total, int *used, int *free) {
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

static const char* _dbg_severity_(GLenum s) {
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

static const char* _dbg_type_(GLenum t) {
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

static bool _gl_debug_seen_(GLuint id, GLenum type, GLenum severity, const char *msg) {
    static GLDebugSeenMsg seen[GL_DEBUG_SEEN_MAX];
    static size_t count = 0;
    static size_t cursor = 0;

    // Проходимся по предыдущим сообщениям и проверяем, было ли оно уже за последние GL_DEBUG_SEEN_MAX сообщений:
    for (size_t i = 0; i < count; i++) {
        if (seen[i].id == id && seen[i].type == type && seen[i].severity == severity && strcmp(seen[i].msg, msg) == 0) {
            return true;
        }
    }

    // Иначе добавляем в кольцевой буфер новое сообщение:
    GLDebugSeenMsg *slot = &seen[cursor];
    slot->id = id;
    slot->type = type;
    slot->severity = severity;
    snprintf(slot->msg, sizeof(slot->msg), "%s", msg);

    if (count < GL_DEBUG_SEEN_MAX) count++;
    cursor = (cursor + 1) % GL_DEBUG_SEEN_MAX;

    return false;
}

// Debug callback:
static void APIENTRY _gl_debug_cb_(
    [[maybe_unused]] GLenum source, GLenum type, GLuint id, GLenum severity,
    [[maybe_unused]] GLsizei length, const GLchar *message, [[maybe_unused]] const void *userParam
) {
    const char *msg = message ? message : "(null)";

    // Если это сообщение совпадает с предыдущими, то пропускаем:
    if (_gl_debug_seen_(id, type, severity, msg)) return;
    log_msg("[GL][%s][%s<id=%u>] %s\n", _dbg_severity_(severity), _dbg_type_(type), id, msg);
}

static void _gl_setup_debug_output_(bool sync, bool notify, bool low, bool medium, bool high) {
    log_msg("[GL] OpenGL debug log level: [NOTIFY=%s, LOW=%s, MEDIUM=%s, HIGH=%s]\n",
        notify ? "ON" : "OFF", low ? "ON" : "OFF", medium ? "ON" : "OFF", high ? "ON" : "OFF"
    );

    // Core 4.3+ (современнее и стабильнее):
    if (GLAD_GL_VERSION_4_3) {
        glEnable(GL_DEBUG_OUTPUT);
        if (sync) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(_gl_debug_cb_, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, nullptr, high);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, nullptr, medium);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, nullptr, low);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, notify);
        log_msg("[GL] OpenGL debug output enabled (CORE 4.3+).\n");
    }

    // Для старых драйверов:
    else if (GLAD_GL_ARB_debug_output) {
        if (sync) glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        else glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB((GLDEBUGPROCARB)_gl_debug_cb_, nullptr);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH,         0, nullptr, high);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM,       0, nullptr, medium);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW,          0, nullptr, low);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, notify);
        log_msg("[GL] OpenGL debug output enabled (ARB DEBUG).\n");
    }
}

// Изменить размер текстур 3D освещения:
static inline void _lighting_resize_(Lighting3D *self, int width, int height) {
    if (!self) return;
    Texture_empty(self->light_tex, width, height, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA16F, TEX_DATA_FLOAT);
}

// Установить размер текстуры теней (пересоздаёт):
static void _lighting_set_shadows_size_(Lighting3D *self, int size) {
    if (!self) return;
    Texture_empty(self->shadows_tex, size, size, false, TEX_FORMAT_DEPTH, TEX_INTERNAL_DEPTH32F, TEX_DATA_FLOAT);
    Renderer_set_shadows_smooth(self->renderer, self->shadows_smooth);
    // Включаем аппаратное сравнение глубины:
    Texture_begin(self->shadows_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
    Texture_end(self->shadows_tex);
    self->shadows_size = size;
}

// Матрица камеры солнца для области радиусом R вокруг центра (с привязкой к текселям):
static void _shadows_light_matrix_(Lighting3D *self, vec3 center, float R, int size, mat4 out_view_proj) {
    // Камера солнца стоит против направления лучей и смотрит на центр:
    vec3 dir = {self->sun_direction.x, self->sun_direction.y, self->sun_direction.z};
    glm_vec3_normalize(dir);
    vec3 eye;
    glm_vec3_scale(dir, -2.0f * R, eye);  // Отходим от центра на 2R против лучей.
    glm_vec3_add(center, eye, eye);
    vec3 up = {0.0f, 1.0f, 0.0f};
    if (fabsf(dir[1]) > 0.99f) { up[0] = 0.0f; up[1] = 0.0f; up[2] = 1.0f; }  // Солнце строго сверху.

    // Создаём камеру солнца:
    mat4 light_view, light_proj;
    glm_lookat(eye, center, up, light_view);
    glm_ortho(-R, R, -R, R, 0.1f, 4.0f * R, light_proj);  // Квадрат 2R x 2R, глубина 4R.
    glm_mat4_mul(light_proj, light_view, out_view_proj);

    // Привязка к сетке текселей, чтобы ничего не мерцало и не ползало при движении камеры:
    vec4 origin = {0.0f, 0.0f, 0.0f, 1.0f};  // Любая фиксированная точка мира.
    glm_mat4_mulv(out_view_proj, origin, origin);  // Куда она попала в пространстве карты теней.
    float half_size = (float)size * 0.5f;
    float texel_x = origin[0] * half_size;  // Та же позиция в текселях.
    float texel_y = origin[1] * half_size;
    light_proj[3][0] += (roundf(texel_x) - texel_x) / half_size;
    light_proj[3][1] += (roundf(texel_y) - texel_y) / half_size;
    glm_mat4_mul(light_proj, light_view, out_view_proj);  // Пересобираем итоговую матрицу с поправкой.
}

// create shader:
static inline Shader* _shd_(Renderer *rnd, const char *vert, const char *frag, const char *geom) {
    Shader *shader = Shader_create(rnd, vert, frag, geom);
    if (!shader) {
        log_msg("[E] Renderer_create: Creating shader failed.\n");
    }
    return shader;
}

// Создать шейдеры:
static inline void _create_shaders_(Renderer *rnd) {
    rnd->shader             = _shd_(rnd, (const char*)DEFAULT_SHD_VERT,     (const char*)DEFAULT_SHD_FRAG, nullptr);
    rnd->shader_gbuffer     = _shd_(rnd, (const char*)GBUFFER_SHD_VERT,     (const char*)GBUFFER_SHD_FRAG, nullptr);
    rnd->shader_lighting    = _shd_(rnd, (const char*)LIGHTING_SHD_VERT,    (const char*)LIGHTING_SHD_FRAG, nullptr);
    rnd->shader_shadow      = _shd_(rnd, (const char*)SHADOW_SHD_VERT,      (const char*)SHADOW_SHD_FRAG, nullptr);
    rnd->shader_final       = _shd_(rnd, (const char*)FINAL_SHD_VERT,       (const char*)FINAL_SHD_FRAG, nullptr);
    rnd->shader_light2d     = _shd_(rnd, (const char*)LIGHT2D_SHD_VERT,     (const char*)LIGHT2D_SHD_FRAG, nullptr);
    rnd->shader_spritebatch = _shd_(rnd, (const char*)SPRITEBATCH_SHD_VERT, (const char*)SPRITEBATCH_SHD_FRAG, nullptr);
}

// Освободить шейдеры:
static inline void _destroy_shaders_(Renderer *rnd) {
    Shader_destroy(&rnd->shader);
    Shader_destroy(&rnd->shader_gbuffer);
    Shader_destroy(&rnd->shader_final);
    Shader_destroy(&rnd->shader_lighting);
    Shader_destroy(&rnd->shader_shadow);
    Shader_destroy(&rnd->shader_spritebatch);
    Shader_destroy(&rnd->shader_light2d);
}

// Скомпилировать шейдеры:
static inline void _compile_shaders_(Renderer *rnd) {
    Shader_compile(rnd->shader);
    Shader_compile(rnd->shader_gbuffer);
    Shader_compile(rnd->shader_final);
    Shader_compile(rnd->shader_lighting);
    Shader_compile(rnd->shader_shadow);
    Shader_compile(rnd->shader_spritebatch);
    Shader_compile(rnd->shader_light2d);
}

// Освободить кэш шейдеров:
static inline void _clear_shaders_cache_(Renderer *rnd) {
    Shader_clear_caches(rnd->shader);
    Shader_clear_caches(rnd->shader_gbuffer);
    Shader_clear_caches(rnd->shader_final);
    Shader_clear_caches(rnd->shader_lighting);
    Shader_clear_caches(rnd->shader_shadow);
    Shader_clear_caches(rnd->shader_spritebatch);
    Shader_clear_caches(rnd->shader_light2d);
}


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create(void) {
    Renderer *rnd = (Renderer*)mm_alloc(sizeof(Renderer));

    // Здесь нельзя инициализировать OpenGL зависимости.

    // Заполняем поля:
    rnd->initialized = false;
    rnd->info = (RendererInfo){};
    rnd->camera = nullptr;
    rnd->camera_type = RENDERER_CAMERA_2D;

    // Отрисовка сцены:
    rnd->draw_commands = nullptr;
    rnd->draw_calls_count = 0;
    rnd->gbuffer = nullptr;
    rnd->gbuffer_dirty = false;
    rnd->lighting = nullptr;
    rnd->exposure = 1.0f;
    rnd->tonemap = RENDERER_TONEMAP_ACES;

    // Другое:
    rnd->sprite_mesh = nullptr;
    rnd->fallback_texture = nullptr;
    rnd->fallback_mat = nullptr;
    return rnd;
}

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd) {
    if (!rnd || !*rnd) return;

    // Освобождаем всякое для отрисовки:
    _destroy_shaders_(*rnd);
    Mesh_destroy(&(*rnd)->sprite_mesh);
    GBuffer_destroy(&(*rnd)->gbuffer);
    Material_destroy(&(*rnd)->fallback_mat);
    Array_destroy(&(*rnd)->draw_commands);

    // Удаляем освещение:
    if ((*rnd)->lighting) {
        BufferFBO_destroy(&(*rnd)->lighting->light_fbo);
        BufferFBO_destroy(&(*rnd)->lighting->shadows_fbo);
        Texture_destroy(&(*rnd)->lighting->light_tex);
        Texture_destroy(&(*rnd)->lighting->shadows_tex);
        mm_free((*rnd)->lighting);
        (*rnd)->lighting = nullptr;
    }

    // Уничтожение текстурных юнитов (перед удалением fallback_texture):
    TextureUnits_destroy();

    // Удаляем текстуру заглушку:
    Texture_destroy(&(*rnd)->fallback_texture);

    // Уничтожение стеков буферов (всё накопленное выше и ранее):
    BufferGC_GL_flush();
    BufferGC_GL_destroy();

    // Освобождаем память рендерера:
    mm_free(*rnd);
    *rnd = nullptr;
}

// Инициализация рендерера:
void Renderer_init(Renderer *self) {
    if (!self || self->initialized) return;

    // Здесь можно инициализировать OpenGL зависимости.

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
    if (g_Renderer_debug_config.debug_enabled) {
        _gl_setup_debug_output_(
            g_Renderer_debug_config.sync,
            g_Renderer_debug_config.level_notify,
            g_Renderer_debug_config.level_low,
            g_Renderer_debug_config.level_medium,
            g_Renderer_debug_config.level_high
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

    // Настройка OpenGL:
    glEnable(GL_BLEND);  // Включаем смешивание цветов.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Устанавливаем режим смешивания.
    glEnable(GL_PROGRAM_POINT_SIZE);  // Разрешаем установку размера точки через шейдер.

    // Инициализация стеков буферов:
    BufferGC_GL_init();

    // Создаём и компилируем шейдеры:
    _create_shaders_(self);
    _compile_shaders_(self);

    // Квадрат с текстурой для спрайта:
    const float texcoord[] = {0.0f, 0.0f, 1.0f, 1.0f};
    const Vertex sprite_vertices[] = {
        // vertex  normal   color           texcoord
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
        false, nullptr
    );

    // Текстура-заглушка:
    self->fallback_texture = Texture_create(self);
    Texture_empty(self->fallback_texture, 1, 1, false, TEX_FORMAT_RGBA, TEX_INTERNAL_RGBA8, TEX_DATA_UBYTE);

    // Материал-заглушка:
    self->fallback_mat = Material_create_default(nullptr);

    // Инициализация текстурных юнитов:
    TextureUnits_init(self);

    // Отрисовка сцены:
    self->draw_commands = Array_create(sizeof(DrawCommand), ARRAY_DEFAULT_CAPACITY);
    self->gbuffer = GBuffer_create(self, Renderer_get_width(self), Renderer_get_height(self));

    // 3D освещение:
    self->lighting = (Lighting3D*)mm_alloc(sizeof(Lighting3D));
    self->lighting->renderer = self;
    self->lighting->light_fbo = BufferFBO_create();
    self->lighting->light_tex = Texture_create(self);
    // Настройки освещения:
    self->lighting->sun_direction = (Vec3f){-0.57735, -0.57735, -0.57735};
    self->lighting->sun_color = (Vec3f){1.0f, 0.96f, 0.9f};
    self->lighting->sun_intensity = 10.0f;
    self->lighting->ambient_color = (Vec3f){0.6f, 0.7f, 1.0f};
    self->lighting->ambient_intensity = 0.1f;
    // Настройки теней:
    self->lighting->shadows_enabled = true;    // По умолчанию тени включены.
    self->lighting->shadows_smooth = true;     // По умолчанию мягкие тени.
    self->lighting->shadows_size = 2048;       // 2048 размер текстуры теней.
    self->lighting->shadows_distance = 10.0f;  // 10 метров вокруг камеры тени наивысшего качества.
    self->lighting->shadows_fbo = BufferFBO_create();
    self->lighting->shadows_tex = Texture_create(self);
    self->lighting->shadows_normal_bias = 1.5f;  // В среднем надо сдвигать на +- 1 тексель. Берем с запасом.
    self->lighting->shadows_depth_bias = 0.0f;   // Обычно 0, поднимать если угри остались.
    self->lighting->shadows_slope_bias = 3.0f;   // Обычно 1.5-3, растёт с радиусом PCF.
    // Обновляем размеры текстур кадровых буферов:
    _lighting_resize_(self->lighting, Renderer_get_width(self), Renderer_get_height(self));
    _lighting_set_shadows_size_(self->lighting, self->lighting->shadows_size);
    // Привязываем текстуру к буферу кадра света:
    BufferFBO_begin(self->lighting->light_fbo);
    BufferFBO_attach(self->lighting->light_fbo, BUFFER_FBO_COLOR, 0, self->lighting->light_tex->id);
    BufferFBO_apply(self->lighting->light_fbo);
    BufferFBO_end(self->lighting->light_fbo);
    // Привязываем текстуру глубины к буферу кадра теней:
    BufferFBO_begin(self->lighting->shadows_fbo);
    BufferFBO_attach(self->lighting->shadows_fbo, BUFFER_FBO_DEPTH, 0, self->lighting->shadows_tex->id);
    BufferFBO_apply(self->lighting->shadows_fbo);
    BufferFBO_end(self->lighting->shadows_fbo);

    // Поднимаем флаг инициализации:
    self->initialized = true;
    log_msg("[I] OpenGL initialized.\n");
}

// Отрисовать всё что накопили, на экран:
void Renderer_display(Renderer *self) {
    if (!self) return;
    if (!self->camera) {
        Array_clear(self->draw_commands, false);
        return;
    }

    // Размеры окна:
    int width = Renderer_get_width(self);
    int height = Renderer_get_height(self);

    // Сбрасываем счетчик отрисовок:
    self->draw_calls_count = 0;

    // Для очистки gbuffer при пустом стеке команд:
    if (!Array_len(self->draw_commands)) {
        if (self->gbuffer_dirty) {
            self->gbuffer_dirty = false;
            GBuffer_resize(self->gbuffer, width, height);
        }
        return;
    }

    // Настраиваем камеру:
    Vec3f camera_pos = {};
    Vec3f camera_forward = {0.0f, 0.0f, -1.0f};
    bool camera_ortho = false;
    if (Renderer_is_camera_2d(self)) {
        Vec2d pos = ((Camera2D*)self->camera)->position;
        camera_pos = (Vec3f){pos.x, pos.y, 0.0f};
    } else {
        Vec3d pos = ((Camera3D*)self->camera)->position;
        camera_pos = (Vec3f){pos.x, pos.y, pos.z};
        Vec3d f = Camera3D_get_forward((Camera3D*)self->camera);
        camera_forward = (Vec3f){f.x, f.y, f.z};
        camera_ortho = Camera3D_get_ortho((Camera3D*)self->camera);
    }

    // Подготавливаем матрицы:
    mat4 view, proj, view_proj, inv_view_proj, light_view_proj;
    Renderer_get_view_proj(self, view, proj);
    glm_mat4_mul(proj, view, view_proj);
    glm_mat4_inv(view_proj, inv_view_proj);
    glm_mat4_identity(light_view_proj);

    // -------- Проход 0 - Тени: --------

    if (self->lighting->shadows_enabled) {
        // Настраиваем состояние рендеринга:
        Renderer_set_depth_test(self, true);
        Renderer_set_depth_mask(self, true);
        Renderer_set_blending(self, false);
        Renderer_set_cull_mode(self, RENDERER_CULL_NONE);  // Тонкие и двусторонние объекты тоже отбрасывают тень.

        // Центр теней не ровно в позиции камеры, а дальше по направлению взгляда наполовину:
        float R = self->lighting->shadows_distance;
        vec3 center = {
            camera_pos.x + camera_forward.x * R * 0.5f,
            camera_pos.y + camera_forward.y * R * 0.5f,
            camera_pos.z + camera_forward.z * R * 0.5f
        };
        _shadows_light_matrix_(self->lighting, center, R, self->lighting->shadows_size, light_view_proj);

        // Состояние прохода теней:
        BufferFBO_begin(self->lighting->shadows_fbo);
        glViewport(0, 0, self->lighting->shadows_size, self->lighting->shadows_size);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);  // Смещение глубины против "теневых угрей":
        glPolygonOffset(self->lighting->shadows_slope_bias, 0.0f);  // Сильнее на наклонных к солнцу поверхностях.

        // Рисуем модели из стека команд на отрисовку:
        Shader_begin(self->shader_shadow);
        Shader_set_mat4(self->shader_shadow, "u_light_view_proj", light_view_proj);
        for (size_t i = 0; i < Array_len(self->draw_commands); i++) {
            DrawCommand *cmd = (DrawCommand*)Array_get(self->draw_commands, i);
            if (!cmd || !cmd->mesh || !cmd->cast_shadow) continue;  // Если каст теней отключен, пропускаем.
            Shader_set_mat4(self->shader_shadow, "u_model", cmd->transform);
            Mesh_render(cmd->mesh, false);  // wireframe не нужен.
        }
        Shader_end(self->shader_shadow);
        glDisable(GL_POLYGON_OFFSET_FILL);
        BufferFBO_end(self->lighting->shadows_fbo);
        glViewport(0, 0, width, height);  // Возвращаем размер экрана.
    }

    // -------- Проход 1 - GBuffer: --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, true);
    Renderer_set_depth_mask(self, true);
    Renderer_set_blending(self, false);
    Renderer_set_cull_mode(self, RENDERER_CULL_NONE);

    // Используем G-Buffer:
    GBuffer_begin(self->gbuffer);
    self->gbuffer_dirty = true;

    // Настраиваем шейдер моделей:
    Shader_begin(self->shader_gbuffer);
    Shader_set_mat4(self->shader_gbuffer, "u_view_proj", view_proj);
    Shader_set_vec3(self->shader_gbuffer, "u_camera_pos", camera_pos);
    Shader_set_bool(self->shader_gbuffer, "u_camera_ortho", camera_ortho);
    Shader_set_vec3(self->shader_gbuffer, "u_camera_forward", camera_forward);

    // Проходимся по командам отрисовки:
    for (size_t i = 0; i < Array_len(self->draw_commands); i++) {
        DrawCommand *cmd = (DrawCommand*)Array_get(self->draw_commands, i);
        if (!cmd || !cmd->mesh) continue;
        Material *mat = cmd->material;
        Shader_set_mat4(self->shader_gbuffer, "u_model", cmd->transform);

        // 1. Карта цвета:
        bool use_albedo_tex = (mat->albedo_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_albedo", use_albedo_tex);
        if (use_albedo_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_albedo", mat->albedo_map->id);
        Shader_set_vec4(self->shader_gbuffer, "u_albedo", mat->albedo);

        // 2. Карта нормалей:
        bool use_normal_tex = (mat->normal_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_normal", use_normal_tex);
        if (use_normal_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_normal", mat->normal_map->id);
        Shader_set_float(self->shader_gbuffer, "u_normal_strength", mat->normal_strength);

        // 3. Карта окклюзии (AO):
        bool use_ao_tex = (mat->occlusion_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_occlusion", use_ao_tex);
        if (use_ao_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_occlusion", mat->occlusion_map->id);
        Shader_set_float(self->shader_gbuffer, "u_ao", mat->ao);

        // 4. Карта матовости:
        bool use_roughness_tex = (mat->roughness_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_roughness", use_roughness_tex);
        if (use_roughness_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_roughness", mat->roughness_map->id);
        Shader_set_float(self->shader_gbuffer, "u_roughness", mat->roughness);

        // 5. Карта металлика:
        bool use_metallic_tex = (mat->metallic_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_metallic", use_metallic_tex);
        if (use_metallic_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_metallic", mat->metallic_map->id);
        Shader_set_float(self->shader_gbuffer, "u_metallic", mat->metallic);

        // 6. Карта свечения:
        bool use_emissive_tex = (mat->emissive_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_emissive", use_emissive_tex);
        if (use_emissive_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_emissive", mat->emissive_map->id);
        Shader_set_vec3(self->shader_gbuffer, "u_emissive_color", mat->emissive_color);
        Shader_set_float(self->shader_gbuffer, "u_emissive_strength", mat->emissive_strength);

        // 7. Карта высоты (параллакс):
        bool use_height_tex = (mat->height_map != nullptr);
        Shader_set_bool(self->shader_gbuffer, "u_use_tex_height", use_height_tex);
        if (use_height_tex) Shader_set_tex2d(self->shader_gbuffer, "u_tex_height", mat->height_map->id);
        Shader_set_float(self->shader_gbuffer, "u_height_strength",    mat->height_strength);
        Shader_set_float(self->shader_gbuffer, "u_pom_min_layers",     mat->height_min_layers);
        Shader_set_float(self->shader_gbuffer, "u_pom_max_layers",     mat->height_max_layers);
        Shader_set_bool(self->shader_gbuffer,  "u_pom_cutoff_enabled", mat->height_cutoff_enabled);
        Shader_set_float(self->shader_gbuffer, "u_pom_low",            mat->height_low);
        Shader_set_float(self->shader_gbuffer, "u_pom_high",           mat->height_high);

        // 8. Остальные параметры:
        Shader_set_vec3(self->shader_gbuffer, "u_ambient", mat->ambient);
        Shader_set_float(self->shader_gbuffer, "u_alpha_cutoff", mat->alpha_cutoff);
        Shader_set_float(self->shader_gbuffer, "u_distortion", mat->distortion);
        Shader_set_float(self->shader_gbuffer, "u_distortion_aberration", mat->distortion_aberration);

        Renderer_set_cull_mode(self, mat->double_sided ? RENDERER_CULL_NONE : RENDERER_CULL_BACK);
        Mesh_render(cmd->mesh, cmd->wireframe);
        self->draw_calls_count++;
    }

    // Конец отрисовки моделей:
    Shader_end(self->shader_gbuffer);
    GBuffer_end(self->gbuffer);

    // -------- Проход 2 - Освещение: --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, false);
    Renderer_set_depth_mask(self, false);
    Renderer_set_blending(self, false);
    Renderer_set_cull_mode(self, RENDERER_CULL_NONE);

    // Включаем и очищаем буфер кадра:
    BufferFBO_begin(self->lighting->light_fbo);
    BufferFBO_apply(self->lighting->light_fbo);
    BufferFBO_clear(self->lighting->light_fbo, 0.0f, 0.0f, 0.0f, 0.0f);

    // Включаем шейдер и настраиваем:
    Shader_begin(self->shader_lighting);
    Shader_set_vec3(self->shader_lighting,  "u_camera_pos",        camera_pos);
    Shader_set_vec3(self->shader_lighting,  "u_camera_forward",    camera_forward);
    Shader_set_bool(self->shader_lighting,  "u_camera_ortho",      camera_ortho);
    Shader_set_mat4(self->shader_lighting,  "u_inv_view_proj",     inv_view_proj);
    Shader_set_mat4(self->shader_lighting,  "u_light_view_proj",   light_view_proj);
    Shader_set_tex2d(self->shader_lighting, "u_albedo_roughness",  GBuffer_get_tex_albedo_roughness(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_normal_ao",         GBuffer_get_tex_normal_ao(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_pbr",               GBuffer_get_tex_pbr_properties(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_emissive",          GBuffer_get_tex_emissive(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_depth",             GBuffer_get_tex_depth(self->gbuffer)->id);
    Shader_set_vec3(self->shader_lighting,  "u_sun_dir",           self->lighting->sun_direction);
    Shader_set_vec3(self->shader_lighting,  "u_sun_color",         self->lighting->sun_color);
    Shader_set_float(self->shader_lighting, "u_sun_intensity",     self->lighting->sun_intensity);
    Shader_set_vec3(self->shader_lighting,  "u_ambient_color",     self->lighting->ambient_color);
    Shader_set_float(self->shader_lighting, "u_ambient_intensity", self->lighting->ambient_intensity);
    Shader_set_bool(self->shader_lighting,  "u_shadows_enabled",   self->lighting->shadows_enabled);
    Shader_set_bool(self->shader_lighting,  "u_shadows_smooth",    self->lighting->shadows_smooth);
    Shader_set_tex2d(self->shader_lighting, "u_shadow_map",        self->lighting->shadows_tex->id);
    // Передаём смещение теней:
    float R = self->lighting->shadows_distance;
    float texel_world = (2.0f * R) / (float)self->lighting->shadows_size;
    float normal_offset = self->lighting->shadows_normal_bias * texel_world;
    float depth_bias = (self->lighting->shadows_depth_bias * texel_world) / (4.0f * R);
    Shader_set_float(self->shader_lighting, "u_shadow_normal_offset", normal_offset);
    Shader_set_float(self->shader_lighting, "u_shadow_depth_bias", depth_bias);

    // Рисуем всё на весь экран:
    Mesh_render(self->sprite_mesh, false);
    Shader_end(self->shader_lighting);
    BufferFBO_end(self->lighting->light_fbo);

    // -------- Финальный проход (на экран): --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, true);
    Renderer_set_depth_mask(self, true);
    Renderer_set_blending(self, true);
    Renderer_set_cull_mode(self, RENDERER_CULL_NONE);
    glDepthFunc(GL_ALWAYS);  // Пишем глубину сцены в экранный буфер всегда, независимо от того, что в нём было.

    Shader_begin(self->shader_final);
    Shader_set_tex2d(self->shader_final, "u_hdr",      self->lighting->light_tex->id);
    Shader_set_tex2d(self->shader_final, "u_depth",    GBuffer_get_tex_depth(self->gbuffer)->id);
    Shader_set_float(self->shader_final, "u_exposure", self->exposure);
    Shader_set_bool(self->shader_final,  "u_aces_tm",  self->tonemap == RENDERER_TONEMAP_ACES ? true : false);
    Mesh_render(self->sprite_mesh, false);
    Shader_end(self->shader_final);
    glDepthFunc(GL_LESS);  // Возвращаем обычный тест глубины.

    // -------- Восстанавливаем параметры для остальной отрисовки: --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, true);
    Renderer_set_depth_mask(self, true);
    Renderer_set_blending(self, true);
    Renderer_set_cull_mode(self, RENDERER_CULL_BACK);

    // Очищаем список команд отрисовки:
    Array_clear(self->draw_commands, false);
}

// Создать команду отрисовки:
void Renderer_create_draw_command(
    Renderer *self, Mesh *mesh, Material *material,
    mat4 transform, bool cast_shadow, bool wireframe
) {
    if (!self || !mesh) return;
    if (!material) material = self->fallback_mat;
    DrawCommand cmd = { .mesh = mesh, .material = material, .cast_shadow = cast_shadow, .wireframe = wireframe };
    glm_mat4_copy(transform, cmd.transform);  // Копируем матрицу.
    Array_push(self->draw_commands, &cmd);
}

// Получить количество вызовов отрисовки:
size_t Renderer_get_draw_calls_count(Renderer *self) {
    if (!self) return 0;
    return self->draw_calls_count;
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
    _clear_shaders_cache_(self);
}

// Получить матрицу вида камеры:
void Renderer_get_view(Renderer *self, mat4 view) {
    glm_mat4_identity(view);
    if (!self || !self->camera) return;
    if (Renderer_is_camera_2d(self)) glm_mat4_copy(((Camera2D*)self->camera)->view, view);
    if (Renderer_is_camera_3d(self)) glm_mat4_copy(((Camera3D*)self->camera)->view, view);
}

// Получить матрицу проекции камеры:
void Renderer_get_proj(Renderer *self, mat4 proj) {
    glm_mat4_identity(proj);
    if (!self || !self->camera) return;
    if (Renderer_is_camera_2d(self)) glm_mat4_copy(((Camera2D*)self->camera)->proj, proj);
    if (Renderer_is_camera_3d(self)) glm_mat4_copy(((Camera3D*)self->camera)->proj, proj);
}

// Получить матрицу вида и проекции камеры:
void Renderer_get_view_proj(Renderer *self, mat4 view, mat4 proj) {
    Renderer_get_view(self, view);
    Renderer_get_proj(self, proj);
}

// Это камера 2D?:
bool Renderer_is_camera_2d(Renderer *self) {
    if (!self) return false;
    return self->camera_type == RENDERER_CAMERA_2D;
}

// Это камера 3D?:
bool Renderer_is_camera_3d(Renderer *self) {
    if (!self) return false;
    return self->camera_type == RENDERER_CAMERA_3D;
}

// Получить 2D камеру:
Camera2D* Renderer_get_camera_2d(Renderer *self) {
    if (!self || !Renderer_is_camera_2d(self)) return nullptr;
    return (Camera2D*)self->camera;
}

// Получить 3D камеру:
Camera3D* Renderer_get_camera_3d(Renderer *self) {
    if (!self || !Renderer_is_camera_3d(self)) return nullptr;
    return (Camera3D*)self->camera;
}

// Получить ширину камеры:
int Renderer_get_width(Renderer *self) {
    if (!self || !self->camera) return 0;
    if (Renderer_is_camera_2d(self)) return ((Camera2D*)self->camera)->width;
    if (Renderer_is_camera_3d(self)) return ((Camera3D*)self->camera)->width;
    return 0;
}

// Получить высоту камеры:
int Renderer_get_height(Renderer *self) {
    if (!self || !self->camera) return 0;
    if (Renderer_is_camera_2d(self)) return ((Camera2D*)self->camera)->height;
    if (Renderer_is_camera_3d(self)) return ((Camera3D*)self->camera)->height;
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
    _get_memory_info_(&total, nullptr, nullptr);
    return total;
}

// Сколько используется видеопамяти (в килобайтах):
int Renderer_get_used_memory(Renderer *self) {
    if (!self) return 0;
    int used;
    _get_memory_info_(nullptr, &used, nullptr);
    return used;
}

// Сколько свободно видеопамяти (в килобайтах):
int Renderer_get_free_memory(Renderer *self) {
    if (!self) return 0;
    int free;
    _get_memory_info_(nullptr, nullptr, &free);
    return free;
}

// Установить камеру:
void Renderer_set_camera(Renderer *self, void *camera, RendererCameraType type) {
    if (!self) return;
    self->camera = camera;
    self->camera_type = type;
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
void Renderer_set_cull_mode(Renderer *self, RendererCullMode mode) {
    if (!self) return;

    if (mode == RENDERER_CULL_NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        switch (mode) {
            case RENDERER_CULL_BACK:           glCullFace(GL_BACK);           break;
            case RENDERER_CULL_FRONT:          glCullFace(GL_FRONT);          break;
            case RENDERER_CULL_FRONT_AND_BACK: glCullFace(GL_FRONT_AND_BACK); break;
            default: break;
        }
    }
}

// Установить направление отсечения граней:
void Renderer_set_front_face(Renderer *self, RendererWindingOrder order) {
    if (!self) return;

    switch (order) {
        case RENDERER_WINDING_CCW: glFrontFace(GL_CCW); break;
        case RENDERER_WINDING_CW:  glFrontFace(GL_CW);  break;
        default: break;
    }
}

// Установить размер viewport:
void Renderer_set_viewport(Renderer *self, int x, int y, int width, int height) {
    if (!self) return;
    glViewport(x, y, width, height);
    GBuffer_resize(self->gbuffer, width, height);  // Меняем размер G-Buffer.
    _lighting_resize_(self->lighting, width, height);
}

// Получить текстуру albedo_roughness:
Texture* Renderer_get_tex_albedo_roughness(Renderer *self) {
    if (!self) return nullptr;
    return GBuffer_get_tex_albedo_roughness(self->gbuffer);
}

// Получить текстуру normal_ao:
Texture* Renderer_get_tex_normal_ao(Renderer *self) {
    if (!self) return nullptr;
    return GBuffer_get_tex_normal_ao(self->gbuffer);
}

// Получить текстуру pbr_properties:
Texture* Renderer_get_tex_pbr_properties(Renderer *self) {
    if (!self) return nullptr;
    return GBuffer_get_tex_pbr_properties(self->gbuffer);
}

// Получить текстуру emissive:
Texture* Renderer_get_tex_emissive(Renderer *self) {
    if (!self) return nullptr;
    return GBuffer_get_tex_emissive(self->gbuffer);
}

// Получить текстуру depth:
Texture* Renderer_get_tex_depth(Renderer *self) {
    if (!self) return nullptr;
    return GBuffer_get_tex_depth(self->gbuffer);
}

// Получить текстуру освещения:
Texture* Renderer_get_tex_light(Renderer *self) {
    if (!self) return nullptr;
    return self->lighting->light_tex;
}

// Получить текстуру теней:
Texture* Renderer_get_tex_shadows(Renderer *self) {
    if (!self) return nullptr;
    return self->lighting->shadows_tex;
}

// Включить или выключить тени:
void Renderer_set_shadows(Renderer *self, bool enabled) {
    if (!self) return;
    self->lighting->shadows_enabled = enabled;
}

// Включены ли тени:
bool Renderer_get_shadows(Renderer *self) {
    if (!self) return false;
    return self->lighting->shadows_enabled;
}

// Сделать тени мягкими или жесткими:
void Renderer_set_shadows_smooth(Renderer *self, bool smooth) {
    if (!self) return;
    if (smooth) {
        Texture_set_linear(self->lighting->shadows_tex);
    } else Texture_set_pixelized(self->lighting->shadows_tex);
    self->lighting->shadows_smooth = smooth;
}

// Мягкие ли тени:
bool Renderer_get_shadows_smooth(Renderer *self) {
    if (!self) return false;
    return self->lighting->shadows_smooth;
}

// Установить размер карты теней:
void Renderer_set_shadows_size(Renderer *self, int size) {
    if (!self) return;
    _lighting_set_shadows_size_(self->lighting, size);
}

// Получить размер карты теней:
int Renderer_get_shadows_size(Renderer *self) {
    if (!self) return 0;
    return self->lighting->shadows_size;
}

// Установить дальность теней:
void Renderer_set_shadows_distance(Renderer *self, float distance) {
    if (!self) return;
    self->lighting->shadows_distance = distance;
}

// Получить дальность теней:
float Renderer_get_shadows_distance(Renderer *self) {
    if (!self) return 0.0f;
    return self->lighting->shadows_distance;
}

// Установить смещение теней (в текселях):
void Renderer_set_shadows_bias(Renderer *self, float normal_bias, float depth_bias, float slope_bias) {
    if (!self) return;
    self->lighting->shadows_normal_bias = normal_bias;
    self->lighting->shadows_depth_bias = depth_bias;
    self->lighting->shadows_slope_bias = slope_bias;
}

// Получить смещение теней (в текселях):
void Renderer_get_shadows_bias(Renderer *self, float *normal_bias, float *depth_bias, float *slope_bias) {
    if (!self) return;
    if (normal_bias) *normal_bias = self->lighting->shadows_normal_bias;
    if (depth_bias)  *depth_bias  = self->lighting->shadows_depth_bias;
    if (slope_bias)  *slope_bias  = self->lighting->shadows_slope_bias;
}

// Установить экспозицию:
void Renderer_set_exposure(Renderer *self, float exposure) {
    if (!self) return;
    self->exposure = exposure;
}

// Получить экспозицию:
float Renderer_get_exposure(Renderer *self) {
    if (!self) return 0.0f;
    return self->exposure;
}

// Установить тонмаппинг:
void Renderer_set_tonemap(Renderer *self, RendererTonemapType tonemap) {
    if (!self) return;
    self->tonemap = tonemap;
}

// Получить тонмаппинг:
RendererTonemapType Renderer_get_tonemap(Renderer *self) {
    if (!self) return RENDERER_TONEMAP_NONE;
    return self->tonemap;
}

// Установить направление солнца:
void Renderer_set_sun_dir(Renderer *self, Vec3f direction) {
    if (!self) return;
    self->lighting->sun_direction = direction;
}

// Получить направление солнца:
Vec3f Renderer_get_sun_dir(Renderer *self) {
    if (!self) return (Vec3f){};
    return self->lighting->sun_direction;
}

// Установить цвет солнца:
void Renderer_set_sun_color(Renderer *self, Vec3f color) {
    if (!self) return;
    self->lighting->sun_color = color;
}

// Получить цвет солнца:
Vec3f Renderer_get_sun_color(Renderer *self) {
    if (!self) return (Vec3f){};
    return self->lighting->sun_color;
}

// Установить интенсивность солнца:
void Renderer_set_sun_intensity(Renderer *self, float intensity) {
    if (!self) return;
    self->lighting->sun_intensity = intensity;
}

// Получить интенсивность солнца:
float Renderer_get_sun_intensity(Renderer *self) {
    if (!self) return 0.0f;
    return self->lighting->sun_intensity;
}

// Установить цвет фонового освещения:
void Renderer_set_ambient_color(Renderer *self, Vec3f color) {
    if (!self) return;
    self->lighting->ambient_color = color;
}

// Получить цвет фонового освещения:
Vec3f Renderer_get_ambient_color(Renderer *self) {
    if (!self) return (Vec3f){};
    return self->lighting->ambient_color;
}

// Установить интенсивность фонового освещения:
void Renderer_set_ambient_intensity(Renderer *self, float intensity) {
    if (!self) return;
    self->lighting->ambient_intensity = intensity;
}

// Получить интенсивность фонового освещения:
float Renderer_get_ambient_intensity(Renderer *self) {
    if (!self) return 0.0f;
    return self->lighting->ambient_intensity;
}
