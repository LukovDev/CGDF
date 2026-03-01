//
// renderer.h - Создаёт общий апи для работы с рендерером.
//
// В Renderer объявляются различные функции и поля, для работы с графикой.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "mesh.h"
#include "shader.h"
#include "texture.h"


// Объявление структур:
typedef struct Renderer Renderer;          // Рендерер.
typedef struct RendererInfo RendererInfo;  // Информация рендерера.


// Тип используемой камеры:
typedef enum {
    RENDERER_CAMERA_2D,
    RENDERER_CAMERA_3D,
} RendererCameraType;


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
    Shader *shader;                // Дефолтная шейдерная программа.
    Shader *shader_spritebatch2d;  // Шейдер пакетной отрисовки спрайтов.
    Shader *shader_light2d;        // Шейдер 2D освещения.

    void *camera;  // Текущая активная камера.
    RendererCameraType camera_type;  // Тип камеры который используется (для корректировок).

    // Другое:
    Mesh *sprite_mesh;  // Сетка спрайта.
    Texture *fallback_texture;  // Пустая текстура как заглушка для шейдеров.
};


// -------- API рендерера: --------


// Создать рендерер:
Renderer* Renderer_create(void);

// Уничтожить рендерер:
void Renderer_destroy(Renderer **rnd);

// Инициализация рендерера:
void Renderer_init(Renderer *self, bool renderer_debug);

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
