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


// Определения:
#define LIGHTS_BATCH 64  // Источников света за один проход освещения. Должно совпадать с lighting_pass_shader.frag
#define SHADOW_CASCADES_MAX 16  // Должно совпадать с CASCADES_MAX в lighting_pass_shader.frag
#define SHADOW_PCF_RADIUS   2   // Радиус PCF (в текселях).


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


// Источник света. Всё копируется в момент вызова:
typedef struct DrawLighting {
    Vec3f position;     // Позиция света.
    Vec3f direction;    // Направление света.
    float size;         // Размер источника света.
    Vec3f color;        // Цвет света.
    float intensity;    // Интенсивность.
    float radius;       // Радиус.
    float angle;        // Угол конуса от края до края (в градусах).
    float inner_angle;  // Внутренний угол конуса от края до края (в градусах).
    bool cast_shadow;   // Отбрасывать ли тени.
    Light3DType type;   // Тип источника света.
} DrawLighting;


// Источник света в виде, готовом для GPU (4 x vec4 = 64 байта):
typedef struct LightGPU {
    Vec4f pos_radius;  // xyz = позиция, w = радиус.
    Vec4f color;       // rgb = цвет * интенсивность, a = cos внутреннего угла.
    Vec4f spot;        // xyz = направление, w - cos внешнего угла.
    Vec4f params;      // x = размер источника, yzw = запас.
} LightGPU;
static_assert(sizeof(LightGPU) == 64, "renderer.c: LightGPU must be 4 x vec4 without padding.");


// Структура освещения в 3D:
typedef struct Lighting3D {
    Renderer  *renderer;
    BufferFBO *light_fbo;       // Буфер кадра освещения.
    Texture   *light_tex;       // Результат освещения (HDR, RGBA16F).
    Array     *visible_lights;  // Видимые источники текущего кадра (LightGPU), заполняются в проходе подготовки.
    // Настройки освещения:
    Vec3f sun_direction;      // Направление солнца.
    Vec3f sun_color;          // Цвет солнца.
    float sun_intensity;      // Интенсивность солнца.
    Vec3f ambient_color;      // Цвет фонового освещения.
    float ambient_intensity;  // Интенсивность фонового освещения.
    // Настройки теней:
    BufferFBO *shadows_fbo;     // Буфер кадра карты теней (только глубина).
    Texture   *shadows_tex;     // Карта теней (текстура глубины).
    bool  shadows_enabled;      // Включить тени.
    bool  shadows_smooth;       // Мягкие тени.
    int   shadows_size;         // Ширина атласа каскадов (в пикселях).
    float shadows_distance;     // Дальность теней вдоль взгляда камеры (в единицах мира).
    float shadows_normal_bias;  // Смещение тени по нормали (в текселях теней).
    float shadows_depth_bias;   // Постоянное смещение глубины (в текселях теней).
    float shadows_slope_bias;   // Наклонное смещение глубины (в текселях теней).
    // Каскады теней:
    int   shadows_cascades;      // Количество каскадов (1..SHADOW_CASCADES_MAX).
    int   shadows_grid_cols;     // Колонки атласа каскадов.
    int   shadows_grid_rows;     // Строки атласа каскадов.
    int   shadows_tile;          // Размер одного каскада в атласе (в пикселях).
    float shadows_split_lambda;  // Распределение каскадов: 0 = равномерно, 1 = логарифмически.
    float shadows_blend;         // Доля каскада под плавный переход в следующий.
    bool  shadows_debug;         // Отладка каскадов.
    mat4  cascade_matrices[SHADOW_CASCADES_MAX];       // Матрицы каскадов.
    float cascade_radius[SHADOW_CASCADES_MAX];         // Сферы каскадов.
    float cascade_splits[SHADOW_CASCADES_MAX + 1];     // Границы каскадов.
    float cascade_normal_offset[SHADOW_CASCADES_MAX];  // Смещение по нормали каждого каскада (в метрах).
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

// Пересоздать атлас каскадов под текущие размер и количество каскадов:
static void _lighting_update_shadows_atlas_(Lighting3D *self) {
    if (!self) return;

    // Сетка. Сначала по ширине (1x1, 2x1, 2x2, 3x2, 3x3, ...):
    int cols = 1;
    while (cols * cols < self->shadows_cascades) cols++;
    int rows = (self->shadows_cascades + cols - 1) / cols;

    // Ширина атласа не может быть больше максимального размера текстуры:
    int width = self->shadows_size;
    int max_size = Renderer_get_max_texture_size(self->renderer);
    if (width > max_size) {
        log_msg("[W] Shadows atlas size %d is bigger than max texture size %d.\n", width, max_size);
        width = max_size;
    }
    int tile = width / cols;

    // Используем DEPTH16 для глубины теней:
    Texture_empty(self->shadows_tex, tile * cols, tile * rows, false,
                  TEX_FORMAT_DEPTH, TEX_INTERNAL_DEPTH16, TEX_DATA_FLOAT);
    Renderer_set_shadows_smooth(self->renderer, self->shadows_smooth);
    Texture_begin(self->shadows_tex);
    // Включаем аппаратное сравнение:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // Если координаты текстуры выйдут за диапазон, цвет всегда будет белым:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
    Texture_end(self->shadows_tex);

    // Сохраняем параметры:
    self->shadows_grid_cols = cols;
    self->shadows_grid_rows = rows;
    self->shadows_tile = tile;
}

// Границы каскадов по дальности (смесь логарифмического и равномерного деления):
static void _shadows_calc_splits_(float near, float far, float lambda, int count, float *out_splits) {
    out_splits[0] = near;
    for (int i = 1; i <= count; i++) {
        float t = (float)i / (float)count;
        float log_split = near * powf(far / near, t);  // Логарифм: больше деталей вблизи.
        float lin_split = near + (far - near) * t;     // Равномерно: больше деталей вдали.
        out_splits[i] = lambda * log_split + (1.0f - lambda) * lin_split;
    }
}

// Описанная сфера среза пирамиды камеры [n, f] (не зависит от поворота камеры, поэтому тени не мерцают):
static void _shadows_slice_sphere_(
    vec3 cam_pos, vec3 cam_forward, float n, float f, float tan_half_fov, float aspect,
    vec3 out_center, float *out_radius
) {
    float k2 = tan_half_fov * tan_half_fov * (1.0f + aspect * aspect);  // Квадрат тангенса по диагонали.
    float z, r;
    if (k2 >= (f - n) / (f + n)) {  // Широкий срез: центр сферы на дальней плоскости.
        z = f;
        r = f * sqrtf(k2);
    } else {
        z = 0.5f * (f + n) * (1.0f + k2);
        r = 0.5f * sqrtf((f - n) * (f - n) + 2.0f * (f * f + n * n) * k2 + (f + n) * (f + n) * k2 * k2);
    }
    glm_vec3_scale(cam_forward, z, out_center);
    glm_vec3_add(cam_pos, out_center, out_center);
    *out_radius = r;
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

// Сфера целиком за одной из плоскостей пирамиды камеры - её не видно:
static bool _sphere_in_frustum_(vec4 planes[6], vec3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        if (glm_vec3_dot(planes[i], center) + planes[i][3] < -radius) return false;
    }
    return true;
}

// Создать шейдер:
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
    rnd->draw_lightings = nullptr;
    rnd->draw_calls_count = 0;
    rnd->lights_count = 0;
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
    Array_destroy(&(*rnd)->draw_lightings);

    // Удаляем освещение:
    if ((*rnd)->lighting) {
        BufferFBO_destroy(&(*rnd)->lighting->light_fbo);
        BufferFBO_destroy(&(*rnd)->lighting->shadows_fbo);
        Texture_destroy(&(*rnd)->lighting->light_tex);
        Texture_destroy(&(*rnd)->lighting->shadows_tex);
        Array_destroy(&(*rnd)->lighting->visible_lights);
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
    self->draw_lightings = Array_create(sizeof(DrawLighting), ARRAY_DEFAULT_CAPACITY);
    self->gbuffer = GBuffer_create(self, Renderer_get_width(self), Renderer_get_height(self));

    // 3D освещение:
    self->lighting = (Lighting3D*)mm_calloc(1, sizeof(Lighting3D));
    Lighting3D *lt = self->lighting;
    lt->renderer = self;
    lt->light_fbo = BufferFBO_create();
    lt->light_tex = Texture_create(self);
    lt->visible_lights = Array_create(sizeof(LightGPU), ARRAY_DEFAULT_CAPACITY);
    // Настройки освещения (значения по умолчанию):
    lt->sun_direction = (Vec3f){-0.57735f, -0.57735f, -0.57735f};
    lt->sun_color = (Vec3f){1.0f, 0.96f, 0.9f};
    lt->sun_intensity = 10.0f;
    lt->ambient_color = (Vec3f){0.6f, 0.7f, 1.0f};
    lt->ambient_intensity = 0.1f;
    // Настройки теней (значения по умолчанию):
    lt->shadows_fbo = BufferFBO_create();
    lt->shadows_tex = Texture_create(self);
    lt->shadows_enabled = true;      // По умолчанию тени включены.
    lt->shadows_smooth = true;       // По умолчанию мягкие тени.
    lt->shadows_size = 8192;         // Ширина атласа: 4 каскада по 2048.
    lt->shadows_distance = 100.0f;   // Тени до 100 метров вдоль взгляда.
    lt->shadows_normal_bias = 1.5f;  // В среднем надо сдвигать на +- 1 тексель. Берем с запасом.
    lt->shadows_depth_bias = 0.5f;   // Обычно 0.5, поднимать если угри остались. Постоянное смещение.
    lt->shadows_slope_bias = 1.0f;   // Обычно 1-3, растёт с радиусом PCF. Против угрей на склонах.
    // Каскады теней (значения по умолчанию):
    lt->shadows_cascades = 4;          // Количество каскадов.
    lt->shadows_split_lambda = 0.75f;  // Больше к 1 - больше деталей вблизи и меньше в дали.
    lt->shadows_blend = 0.25f;         // 25% от каскада смешивается с другим.
    lt->shadows_debug = false;         // Отладочный режим отрисовки каскадов.
    // Обновляем размеры текстур кадровых буферов:
    _lighting_resize_(lt, Renderer_get_width(self), Renderer_get_height(self));
    _lighting_update_shadows_atlas_(lt);
    // Привязываем текстуру к буферу кадра света:
    BufferFBO_begin(lt->light_fbo);
    BufferFBO_attach(lt->light_fbo, BUFFER_FBO_COLOR, 0, lt->light_tex->id);
    BufferFBO_apply(lt->light_fbo);
    BufferFBO_end(lt->light_fbo);
    // Привязываем текстуру глубины к буферу кадра теней:
    BufferFBO_begin(lt->shadows_fbo);
    BufferFBO_attach(lt->shadows_fbo, BUFFER_FBO_DEPTH, 0, lt->shadows_tex->id);
    BufferFBO_apply(lt->shadows_fbo);
    BufferFBO_end(lt->shadows_fbo);

    // Поднимаем флаг инициализации:
    self->initialized = true;
    log_msg("[I] OpenGL initialized.\n");
}

// Отрисовать всё что накопили, на экран:
void Renderer_display(Renderer *self) {
    if (!self) return;
    if (!self->camera) {
        Array_clear(self->draw_commands, false);
        Array_clear(self->draw_lightings, false);
        return;
    }

    // Размеры окна:
    int width = Renderer_get_width(self);
    int height = Renderer_get_height(self);

    // Сбрасываем счетчики отрисовок:
    self->draw_calls_count = 0;
    self->lights_count = 0;

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
    float camera_near = 0.01f;
    if (Renderer_is_camera_3d(self)) {
        Camera3D *cam = (Camera3D*)self->camera;
        camera_pos = (Vec3f){cam->position.x, cam->position.y, cam->position.z};
        Vec3d f = Camera3D_get_forward(cam);
        camera_forward = (Vec3f){f.x, f.y, f.z};
        camera_ortho = Camera3D_get_ortho(cam);
        camera_near = cam->z_near;
    } else {
        Camera2D *cam = (Camera2D*)self->camera;
        camera_pos = (Vec3f){cam->position.x, cam->position.y, 0.0f};
        camera_near = -1.0f;
    }

    // Освещение и каскады теней:
    Lighting3D *lt = self->lighting;
    int cascades = camera_ortho ? 1 : lt->shadows_cascades;  // Для ортокамеры сферы не подходят. Используем 1 каскад.

    // -------- Проход 0 - Подготовка: --------

    // Подготавливаем матрицы:
    mat4 view, proj, view_proj, inv_view_proj;
    Renderer_get_view_proj(self, view, proj);
    glm_mat4_mul(proj, view, view_proj);
    glm_mat4_inv(view_proj, inv_view_proj);

    // ---- Источники света:

    // Оставляем только те, что достают до видимой области, и упаковываем для GPU:
    vec4 planes[6];  // Плоскости пирамиды камеры.
    glm_frustum_planes(view_proj, planes);
    Array_clear(lt->visible_lights, false);

    // Проходимся по источникам света и отбираем только те, что достают до видимой области:
    for (size_t i = 0; i < Array_len(self->draw_lightings); i++) {
        const DrawLighting *l = (const DrawLighting*)Array_get(self->draw_lightings, i);
        vec3 pos = {l->position.x, l->position.y, l->position.z};
        if (!_sphere_in_frustum_(planes, pos, l->radius)) continue;  // Свет не достаёт до видимой области. Пропускаем.

        // Косинусы краёв конуса для шейдера (smoothstep от внешнего к внутреннему):
        float cos_outer, cos_inner;
        switch (l->type) {
            // Прожектор:
            case LIGHT3D_SPOT: {
                float outer = glm_clamp(l->angle, 0.2f, 179.999f) * 0.5f;        // Весь конус -> половина от оси.
                float inner = glm_clamp(l->inner_angle, 0.0f, l->angle) * 0.5f;  // Не больше внешнего.
                if (inner > outer - 0.05f) inner = outer - 0.05f;                // Края smoothstep не должны совпадать.
                cos_outer = cosf(radians(outer));
                cos_inner = cosf(radians(inner));
            } break;

            // Точечный:
            case LIGHT3D_POINT:
            default: {
                cos_outer = -2.0f;
                cos_inner = -1.0f;
            } break;
        }

        // Упаковываем для GPU:
        LightGPU gpu = {
            .pos_radius = {l->position.x, l->position.y, l->position.z, l->radius},
            .color      = {l->color.x * l->intensity, l->color.y * l->intensity, l->color.z * l->intensity, cos_inner},
            .spot       = {l->direction.x, l->direction.y, l->direction.z, cos_outer},
            .params     = {l->size > 0.001f ? l->size : 0.001f, 0.0f, 0.0f, 0.0f},
        };
        Array_push(lt->visible_lights, &gpu);
    }
    self->lights_count = Array_len(self->draw_lightings);

    // -------- Проход 1 - Тени: --------

    // Считаем тени солнца:
    if (lt->shadows_enabled && lt->sun_intensity > 0.0f && Renderer_is_camera_3d(self)) {
        // Настраиваем состояние рендеринга:
        Renderer_set_depth_test(self, true);
        Renderer_set_depth_mask(self, true);
        Renderer_set_blending(self, false);
        Renderer_set_cull_mode(self, RENDERER_CULL_NONE);  // Тонкие и двусторонние объекты тоже отбрасывают тень.

        Camera3D *cam = (Camera3D*)self->camera;
        int tile = lt->shadows_tile;
        float *splits = lt->cascade_splits;
        vec3 cam_pos = {camera_pos.x, camera_pos.y, camera_pos.z};
        vec3 cam_fwd = {camera_forward.x, camera_forward.y, camera_forward.z};
        float tan_half_fov = tanf(radians(cam->fov) * 0.5f);
        float aspect = (float)width / (float)height;
        float pad = (float)tile / ((float)tile - 2.0f * (SHADOW_PCF_RADIUS + 4.0f));

        // Рассчитываем расположение каскадов:
        _shadows_calc_splits_(cam->z_near, lt->shadows_distance, lt->shadows_split_lambda, cascades, splits);
        for (int c = 0; c < cascades; c++) {
            // Начало каскада сдвигаем назад на полосу перехода предыдущего, чтобы она попала в обе карты:
            float n = (c == 0) ? splits[0] : splits[c] - (splits[c] - splits[c - 1]) * lt->shadows_blend;
            vec3 center;
            float radius;
            _shadows_slice_sphere_(cam_pos, cam_fwd, n, splits[c + 1], tan_half_fov, aspect, center, &radius);
            lt->cascade_radius[c] = radius * pad;
            float texel_world = (2.0f * lt->cascade_radius[c]) / (float)tile;  // Тексель каскада в метрах.
            lt->cascade_normal_offset[c] = lt->shadows_normal_bias * texel_world;
            _shadows_light_matrix_(lt, center, lt->cascade_radius[c], tile, lt->cascade_matrices[c]);
        }

        // Состояние прохода теней:
        BufferFBO_begin(lt->shadows_fbo);
        glClear(GL_DEPTH_BUFFER_BIT);  // Очищает весь атлас (glViewport на очистку не влияет).
        glEnable(GL_DEPTH_CLAMP);      // Объекты перед ближней плоскостью прижимаются к ней, а не обрезаются.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(lt->shadows_slope_bias, 0.0f);

        // Рисуем модели из стека команд на отрисовку:
        Shader_begin(self->shader_shadow);
        for (int c = 0; c < cascades; c++) {
            glViewport((c % lt->shadows_grid_cols) * tile, (c / lt->shadows_grid_cols) * tile, tile, tile);
            Shader_set_mat4(self->shader_shadow, "u_light_view_proj", lt->cascade_matrices[c]);
            for (size_t i = 0; i < Array_len(self->draw_commands); i++) {
                DrawCommand *cmd = (DrawCommand*)Array_get(self->draw_commands, i);
                if (!cmd || !cmd->mesh || !cmd->cast_shadow) continue;  // Если каст теней отключен, пропускаем.
                Shader_set_mat4(self->shader_shadow, "u_model", cmd->transform);
                Mesh_render(cmd->mesh, false);  // wireframe не нужен.
            }
        }
        Shader_end(self->shader_shadow);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_DEPTH_CLAMP);
        BufferFBO_end(lt->shadows_fbo);
        glViewport(0, 0, width, height);
    }

    // -------- Проход 2 - GBuffer: --------

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
        Shader_set_float(self->shader_gbuffer, "u_height_strength", mat->height_strength);
        Shader_set_float(self->shader_gbuffer, "u_pom_min_layers",  mat->height_min_layers);
        Shader_set_float(self->shader_gbuffer, "u_pom_max_layers",  mat->height_max_layers);
        Shader_set_float(self->shader_gbuffer, "u_pom_low",         mat->height_low);
        Shader_set_float(self->shader_gbuffer, "u_pom_high",        mat->height_high);
        Shader_set_float(self->shader_gbuffer, "u_pom_base",        mat->height_base);
        Shader_set_float(self->shader_gbuffer, "u_pom_distance",    mat->height_distance);

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

    // -------- Проход 3 - Освещение: --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, false);
    Renderer_set_depth_mask(self, false);
    Renderer_set_blending(self, false);
    Renderer_set_cull_mode(self, RENDERER_CULL_NONE);

    // Включаем и очищаем буфер кадра:
    BufferFBO_begin(lt->light_fbo);
    BufferFBO_apply(lt->light_fbo);
    BufferFBO_clear(lt->light_fbo, 0.0f, 0.0f, 0.0f, 0.0f);

    // Включаем шейдер и настраиваем:
    Shader_begin(self->shader_lighting);
    Shader_set_vec3(self->shader_lighting,  "u_camera_pos",        camera_pos);
    Shader_set_vec3(self->shader_lighting,  "u_camera_forward",    camera_forward);
    Shader_set_bool(self->shader_lighting,  "u_camera_ortho",      camera_ortho);
    Shader_set_float(self->shader_lighting, "u_camera_near",       camera_near);
    Shader_set_mat4(self->shader_lighting,  "u_inv_view_proj",     inv_view_proj);
    // GBuffer:
    Shader_set_tex2d(self->shader_lighting, "u_albedo_roughness",  GBuffer_get_tex_albedo_roughness(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_normal_ao",         GBuffer_get_tex_normal_ao(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_pbr",               GBuffer_get_tex_pbr_properties(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_emissive",          GBuffer_get_tex_emissive(self->gbuffer)->id);
    Shader_set_tex2d(self->shader_lighting, "u_depth",             GBuffer_get_tex_depth(self->gbuffer)->id);
    // Light:
    Shader_set_vec3(self->shader_lighting,  "u_sun_dir",           lt->sun_direction);
    Shader_set_vec3(self->shader_lighting,  "u_sun_color",         lt->sun_color);
    Shader_set_float(self->shader_lighting, "u_sun_intensity",     lt->sun_intensity);
    Shader_set_vec3(self->shader_lighting,  "u_ambient_color",     lt->ambient_color);
    Shader_set_float(self->shader_lighting, "u_ambient_intensity", lt->ambient_intensity);
    // Shadows:
    Shader_set_tex2d(self->shader_lighting, "u_shadow_map",        lt->shadows_tex->id);
    Shader_set_bool(self->shader_lighting,  "u_shadows_enabled",   lt->shadows_enabled);
    Shader_set_bool(self->shader_lighting,  "u_shadows_smooth",    lt->shadows_smooth);
    Shader_set_bool(self->shader_lighting,  "u_shadows_debug",     lt->shadows_debug);
    Shader_set_int(self->shader_lighting,   "u_shadow_pcf_radius", SHADOW_PCF_RADIUS);
    Shader_set_float(self->shader_lighting, "u_shadow_depth_bias", lt->shadows_depth_bias / (2.0f * lt->shadows_tile));
    // Каскады теней:
    Shader_set_int(self->shader_lighting,   "u_cascade_count",     cascades);
    Shader_set_float(self->shader_lighting, "u_cascade_blend",     lt->shadows_blend);
    Shader_set_ivec2(self->shader_lighting, "u_cascade_grid", (Vec2i){lt->shadows_grid_cols, lt->shadows_grid_rows});
    Shader_set_mat4_array(self->shader_lighting,  "u_cascade_matrices",      lt->cascade_matrices,      cascades);
    Shader_set_float_array(self->shader_lighting, "u_cascade_normal_offset", lt->cascade_normal_offset, cascades);
    Shader_set_float_array(self->shader_lighting, "u_cascade_splits",        lt->cascade_splits,        cascades + 1);

    // Рисуем солнце, фон и свечение:
    Shader_set_bool(self->shader_lighting, "u_base_pass", true);
    Shader_set_int(self->shader_lighting,  "u_light_count", 0);
    Mesh_render(self->sprite_mesh, false);

    // Источники света пачками поверх базового прохода (аддитивное смешивание):
    size_t total = Array_len(lt->visible_lights);
    if (total > 0) {
        Shader_set_bool(self->shader_lighting, "u_base_pass", false);
        Renderer_set_blending(self, true);
        glBlendFunc(GL_ONE, GL_ONE);  // Свет складывается.
        for (size_t offset = 0; offset < total; offset += LIGHTS_BATCH) {
            size_t left = total - offset;
            int count = (int)(left < LIGHTS_BATCH ? left : LIGHTS_BATCH);
            const Vec4f *data = (const Vec4f*)Array_get(lt->visible_lights, offset);
            Shader_set_int(self->shader_lighting, "u_light_count", count);
            Shader_set_vec4_array(self->shader_lighting, "u_lights", data, count * 4);  // По 4 vec4 на источник.
            Mesh_render(self->sprite_mesh, false);
        }

        // Возвращаем обычный режим смешивания (как в Renderer_init):
        Renderer_set_blending(self, false);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    Shader_end(self->shader_lighting);
    BufferFBO_end(lt->light_fbo);

    // -------- Проход 4 - Финал (отрисовка на экран): --------

    // Настраиваем состояние рендеринга:
    Renderer_set_depth_test(self, true);
    Renderer_set_depth_mask(self, true);
    Renderer_set_blending(self, true);
    Renderer_set_cull_mode(self, RENDERER_CULL_NONE);
    glDepthFunc(GL_ALWAYS);  // Пишем глубину сцены в экранный буфер всегда, независимо от того, что в нём было.

    Shader_begin(self->shader_final);
    Shader_set_tex2d(self->shader_final, "u_hdr",      lt->light_tex->id);
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
    Array_clear(self->draw_lightings, false);
}

// Создать команду отрисовки (только для 3D):
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

// Создать источник света (только для 3D):
void Renderer_create_draw_lighting(
    Renderer *self, Vec3f position, Vec3f direction, float size, Vec3f color, float intensity,
    float radius, float angle, float inner_angle, bool cast_shadow, Light3DType type
) {
    if (!self || radius <= 0.0f) return;
    DrawLighting light = {
        .position = position, .direction = Vec3f_norm(direction), .size = size,
        .color = color, .intensity = intensity, .radius = radius, .angle = angle,
        .inner_angle = inner_angle, .cast_shadow = cast_shadow, .type = type
    };
    Array_push(self->draw_lightings, &light);
}

// Получить количество вызовов отрисовки:
size_t Renderer_get_draw_calls_count(Renderer *self) {
    if (!self) return 0;
    return self->draw_calls_count;
}

// Получить только видимое количество источников света:
size_t Renderer_get_visible_lights_count(Renderer *self) {
    if (!self) return 0;
    return Array_len(self->lighting->visible_lights);
}

// Получить количество источников света:
size_t Renderer_get_lights_count(Renderer *self) {
    if (!self) return 0;
    return self->lights_count;
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

// Установить размер атласа каскадов теней:
void Renderer_set_shadows_size(Renderer *self, int size) {
    if (!self || size <= 0) return;
    int max_size = Renderer_get_max_texture_size(self);
    if (size > max_size) size = max_size;
    self->lighting->shadows_size = size;
    _lighting_update_shadows_atlas_(self->lighting);
}

// Получить размер атласа каскадов теней:
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

// Установить количество каскадов теней (пересоздаёт атлас теней):
void Renderer_set_shadows_cascades(Renderer *self, int count) {
    if (!self) return;
    if (count < 1) count = 1;
    if (count > SHADOW_CASCADES_MAX) count = SHADOW_CASCADES_MAX;
    if (count == self->lighting->shadows_cascades) return;
    self->lighting->shadows_cascades = count;
    _lighting_update_shadows_atlas_(self->lighting);
}

// Получить количество каскадов теней:
int Renderer_get_shadows_cascades(Renderer *self) {
    if (!self) return 0;
    return self->lighting->shadows_cascades;
}

// Установить распределение каскадов (0 = равномерно, 1 = логарифмически):
void Renderer_set_shadows_split_lambda(Renderer *self, float lambda) {
    if (!self) return;
    self->lighting->shadows_split_lambda = glm_clamp(lambda, 0.0f, 1.0f);
}

// Получить распределение каскадов:
float Renderer_get_shadows_split_lambda(Renderer *self) {
    if (!self) return 0.0f;
    return self->lighting->shadows_split_lambda;
}

// Установить сглаживание перехода каскадов:
void Renderer_set_shadows_blend(Renderer *self, float blend) {
    if (!self) return;
    self->lighting->shadows_blend = glm_clamp(blend, 0.0f, 0.5f);
}

// Получить сглаживание перехода каскадов:
float Renderer_get_shadows_blend(Renderer *self) {
    if (!self) return 0.0f;
    return self->lighting->shadows_blend;
}

// Включить отладку каскадов теней:
void Renderer_set_shadows_debug(Renderer *self, bool enabled) {
    if (!self) return;
    self->lighting->shadows_debug = enabled;
}

// Получить отладку каскадов теней:
bool Renderer_get_shadows_debug(Renderer *self) {
    if (!self) return false;
    return self->lighting->shadows_debug;
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
