//
// renderer.h - Создаёт общий апи для работы с рендерингом графики.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include "gbuffer.h"
#include "light.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"


// Объявление структур:
typedef struct Camera3D Camera3D;
typedef struct Camera2D Camera2D;
typedef struct Renderer Renderer;  // Рендерер.
typedef struct RendererInfo RendererInfo;  // Информация рендерера.
typedef struct RendererDebugConfig RendererDebugConfig;  // Настройка дебага рендеринга.
typedef struct Lighting3D Lighting3D;  // 3D освещение рендерпайплайна.


// Тип используемой камеры:
typedef enum {
    RENDERER_CAMERA_2D,
    RENDERER_CAMERA_3D,
} RendererCameraType;


// Режим отсечения:
typedef enum {
    RENDERER_CULL_NONE = 0,
    RENDERER_CULL_BACK,
    RENDERER_CULL_FRONT,
    RENDERER_CULL_FRONT_AND_BACK,
    RENDERER_CULL_COUNT
} RendererCullMode;


// В какую сторону отсекать:
typedef enum {
    RENDERER_WINDING_CCW = 0,  // Против часовой стрелки (обычно по умолчанию).
    RENDERER_WINDING_CW,       // По часовой стрелке.
    RENDERER_WINDING_COUNT
} RendererWindingOrder;


// Тип тонмаппинга:
typedef enum {
    RENDERER_TONEMAP_NONE = 0,
    RENDERER_TONEMAP_ACES,
    RENDERER_TONEMAP_COUNT
} RendererTonemapType;


// Настройка дебага рендеринга:
struct RendererDebugConfig {
    bool debug_enabled;  // Включить дебаг.
    bool sync;           // Синхронизировать поступление сообщений с вызовом API.
    bool level_notify;   // Уровень поступления сообщений: Уведомление.
    bool level_low;      // Уровень поступления сообщений: Низкий.
    bool level_medium;   // Уровень поступления сообщений: Средний.
    bool level_high;     // Уровень поступления сообщений: Высокий.
};


// Информация рендерера:
struct RendererInfo {
    char *vendor;    // Производитель видеокарты.
    char *renderer;  // Название видеокарты.
    char *version;   // Версия драйвера.
    char *glsl;      // Версия шейдерного языка.
    int max_texture_size;  // Максимальный размер текстуры.
};


// Рендерер:
struct Renderer {
    bool initialized;   // Флаг инициализации контекста OpenGL.
    RendererInfo info;  // Информация рендерера.
    void *camera;                    // Текущая активная камера.
    RendererCameraType camera_type;  // Тип камеры который используется (для корректировок).

    // Шейдеры:
    Shader *shader;               // Дефолтная шейдерная программа.
    Shader *shader_gbuffer;       // Шейдер gbuffer.
    Shader *shader_lighting;      // Шейдер прохода освещения.
    Shader *shader_shadow;        // Шейдер теней.
    Shader *shader_final;         // Шейдер финального прохода.
    Shader *shader_spritebatch;   // Шейдер пакетной отрисовки спрайтов.
    Shader *shader_light2d;       // Шейдер 2D освещения.

    // Отрисовка сцены:
    Array *draw_commands;         // Массив команд на отрисовку.
    size_t draw_calls_count;      // Количество вызовов отрисовки.
    GBuffer *gbuffer;             // G-Buffer.
    bool gbuffer_dirty;           // Флаг для очистки гбуфера.
    Lighting3D *lighting;         // 3D освещение сцены.
    // Используется в final проходе:
    float exposure;               // Экспозиция освещения.
    RendererTonemapType tonemap;  // Тонмаппинг.

    // Другое:
    Mesh *sprite_mesh;          // Сетка спрайта.
    Texture *fallback_texture;  // Пустая текстура как заглушка для шейдеров.
    Material *fallback_mat;     // Материал по умолчанию.
};


// Глобальная конфигурация дебага рендеринга:
extern RendererDebugConfig g_Renderer_debug_config;


// -------- API рендерера: --------


// Создать рендерер:
[[nodiscard]] Renderer* Renderer_create(void);

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd);

// Инициализация рендерера:
void Renderer_init(Renderer *self);

// Отрисовать всё что накопили, на экран:
void Renderer_display(Renderer *self);

// Создать команду отрисовки:
void Renderer_create_draw_command(
    Renderer *self, Mesh *mesh, Material *material,
    mat4 transform, bool cast_shadow, bool wireframe
);

// Получить количество вызовов отрисовки:
size_t Renderer_get_draw_calls_count(Renderer *self);

// Освобождение буферов:
void Renderer_buffers_flush(Renderer *self);

// Освобождаем кэши:
void Renderer_clear_caches(Renderer *self);

// Получить матрицу вида камеры:
void Renderer_get_view(Renderer *self, mat4 view);

// Получить матрицу проекции камеры:
void Renderer_get_proj(Renderer *self, mat4 proj);

// Получить матрицу вида и проекции камеры:
void Renderer_get_view_proj(Renderer *self, mat4 view, mat4 proj);

// Это камера 2D?:
bool Renderer_is_camera_2d(Renderer *self);

// Это камера 3D?:
bool Renderer_is_camera_3d(Renderer *self);

// Получить 2D камеру:
Camera2D* Renderer_get_camera_2d(Renderer *self);

// Получить 3D камеру:
Camera3D* Renderer_get_camera_3d(Renderer *self);

// Получить ширину камеры:
int Renderer_get_width(Renderer *self);

// Получить высоту камеры:
int Renderer_get_height(Renderer *self);

// Получить производителя видеокарты:
const char* Renderer_get_vendor(Renderer *self);

// Получить название видеокарты:
const char* Renderer_get_renderer(Renderer *self);

// Получить версию драйвера:
const char* Renderer_get_version(Renderer *self);

// Получить версию шейдерного языка:
const char* Renderer_get_glsl(Renderer *self);

// Получить максимальный размер текстуры:
int Renderer_get_max_texture_size(Renderer *self);

// Получить сколько всего видеопамяти есть (в килобайтах):
int Renderer_get_total_memory(Renderer *self);

// Сколько используется видеопамяти (в килобайтах):
int Renderer_get_used_memory(Renderer *self);

// Сколько свободно видеопамяти (в килобайтах):
int Renderer_get_free_memory(Renderer *self);

// Установить камеру:
void Renderer_set_camera(Renderer *self, void *camera, RendererCameraType type);

// Установить проверку глубины:
void Renderer_set_depth_test(Renderer *self, bool enabled);

// Включить или отключить запись глубины:
void Renderer_set_depth_mask(Renderer *self, bool enabled);

// Включить или отключить смешивание:
void Renderer_set_blending(Renderer *self, bool enabled);

// Установить отсечение граней:
void Renderer_set_cull_mode(Renderer *self, RendererCullMode mode);

// Установить направление отсечения граней:
void Renderer_set_front_face(Renderer *self, RendererWindingOrder order);

// Установить размер viewport:
void Renderer_set_viewport(Renderer *self, int x, int y, int width, int height);

// Получить текстуру albedo_roughness:
Texture* Renderer_get_tex_albedo_roughness(Renderer *self);

// Получить текстуру normal_ao:
Texture* Renderer_get_tex_normal_ao(Renderer *self);

// Получить текстуру pbr_properties:
Texture* Renderer_get_tex_pbr_properties(Renderer *self);

// Получить текстуру emissive:
Texture* Renderer_get_tex_emissive(Renderer *self);

// Получить текстуру depth:
Texture* Renderer_get_tex_depth(Renderer *self);

// Получить текстуру освещения:
Texture* Renderer_get_tex_light(Renderer *self);

// Получить текстуру теней:
Texture* Renderer_get_tex_shadows(Renderer *self);

// Установить размер карты теней:
void Renderer_set_shadows(Renderer *self, bool enabled);

// Включены ли тени:
bool Renderer_get_shadows(Renderer *self);

// Сделать тени мягкими или жесткими:
void Renderer_set_shadows_smooth(Renderer *self, bool smooth);

// Мягкие ли тени:
bool Renderer_get_shadows_smooth(Renderer *self);

// Установить размер карты теней:
void Renderer_set_shadows_size(Renderer *self, int size);

// Получить размер карты теней:
int Renderer_get_shadows_size(Renderer *self);

// Установить дальность теней:
void Renderer_set_shadows_distance(Renderer *self, float distance);

// Получить дальность теней:
float Renderer_get_shadows_distance(Renderer *self);

// Установить экспозицию:
void Renderer_set_exposure(Renderer *self, float exposure);

// Получить экспозицию:
float Renderer_get_exposure(Renderer *self);

// Установить тонмаппинг:
void Renderer_set_tonemap(Renderer *self, RendererTonemapType tonemap);

// Получить тонмаппинг:
RendererTonemapType Renderer_get_tonemap(Renderer *self);

// Установить направление солнца:
void Renderer_set_sun_dir(Renderer *self, Vec3f direction);

// Получить направление солнца:
Vec3f Renderer_get_sun_dir(Renderer *self);

// Установить цвет солнца:
void Renderer_set_sun_color(Renderer *self, Vec3f color);

// Получить цвет солнца:
Vec3f Renderer_get_sun_color(Renderer *self);

// Установить интенсивность солнца:
void Renderer_set_sun_intensity(Renderer *self, float intensity);

// Получить интенсивность солнца:
float Renderer_get_sun_intensity(Renderer *self);

// Установить цвет фонового освещения:
void Renderer_set_ambient_color(Renderer *self, Vec3f color);

// Получить цвет фонового освещения:
Vec3f Renderer_get_ambient_color(Renderer *self);

// Установить интенсивность фонового освещения:
void Renderer_set_ambient_intensity(Renderer *self, float intensity);

// Получить интенсивность фонового освещения:
float Renderer_get_ambient_intensity(Renderer *self);
