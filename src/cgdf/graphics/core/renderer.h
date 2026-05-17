//
// renderer.h - Создаёт общий апи для работы с рендерингом графики.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include "mesh.h"
#include "shader.h"
#include "texture.h"


// Объявление структур:
typedef struct Renderer Renderer;          // Рендерер.
typedef struct RendererInfo RendererInfo;  // Информация рендерера.
typedef struct RendererDebugConfig RendererDebugConfig;  // Настройка дебага рендеринга.


// Тип используемой камеры:
typedef enum {
    RENDERER_CAMERA_2D,
    RENDERER_CAMERA_3D,
} RendererCameraType;


// Настройка дебага рендеринга:
struct RendererDebugConfig {
    bool debug_enabled;  // Включить дебаг.
    bool sync;           // Синхронизировать поступление сообщений с вызовом API.
    bool level_notify;   // Уровень поступления сообщений: Уведомление.
    bool level_low;      // Уровень поступления сообщений: Низкий.
    bool level_medium;   // Уровень поступления сообщений: Средний.
    bool level_high;     // Уровень поступления сообщений: Высокий.
};


// Глобальная конфигурация дебага рендеринга:
extern RendererDebugConfig g_Renderer_debug_config;


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
    Shader *shader;              // Дефолтная шейдерная программа.
    Shader *shader_model;        // Шейдер модели.
    Shader *shader_spritebatch;  // Шейдер пакетной отрисовки спрайтов.
    Shader *shader_light2d;      // Шейдер 2D освещения.

    // Отрисовка сцены:
    Array *models;  // Массив указателей на модели для отрисовки.

    // Другое:
    Mesh *sprite_mesh;          // Сетка спрайта.
    Texture *fallback_texture;  // Пустая текстура как заглушка для шейдеров.
};


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create(void);

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd);

// Инициализация рендерера:
void Renderer_init(Renderer *self);

// Отрисовать всё что накопили, на экран:
void Renderer_display(Renderer *self);

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

// Установить проверку глубины:
void Renderer_set_depth_test(Renderer *self, bool enabled);

// Включить или отключить запись глубины:
void Renderer_set_depth_mask(Renderer *self, bool enabled);

// Включить или отключить смешивание:
void Renderer_set_blending(Renderer *self, bool enabled);

// Установить отсечение граней:
void Renderer_set_cull_faces(Renderer *self, bool enabled);

// Отсекать только задние грани:
void Renderer_set_back_face_culling(Renderer *self);

// Отсекать только передние грани:
void Renderer_set_front_face_culling(Renderer *self);

// Передняя грань против часовой стрелки (CCW):
void Renderer_set_front_face_onleft(Renderer *self);

// Передняя грань по часовой стрелке (CW):
void Renderer_set_front_face_onright(Renderer *self);

// Установить размер viewport:
void Renderer_set_viewport(Renderer *self, int x, int y, int width, int height);
