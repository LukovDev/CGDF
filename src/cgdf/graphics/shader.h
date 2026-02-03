//
// shader.h - Определяет общий функционал для работы с шейдерами.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>


// Виды значений юниформов для кэша:
typedef enum ShaderCacheUniformType {
    SHADERCACHE_UNIFORM_BOOL,
    SHADERCACHE_UNIFORM_INT,
    SHADERCACHE_UNIFORM_FLOAT,
    SHADERCACHE_UNIFORM_VEC2,
    SHADERCACHE_UNIFORM_VEC3,
    SHADERCACHE_UNIFORM_VEC4,
} ShaderCacheUniformType;


// Объявление структур:
typedef struct Shader Shader;  // Шейдерная программа.
typedef struct ShaderCacheUniformLocation ShaderCacheUniformLocation;
typedef struct ShaderCacheUniformValue ShaderCacheUniformValue;
typedef struct ShaderCacheSampler ShaderCacheSampler;
typedef struct Renderer Renderer;  // Повторное локальное определение.


// Единица кэша локаций юниформов:
struct ShaderCacheUniformLocation {
    char    *name;     // Имя юниформа.
    int32_t location;  // Позиция в шейдере.
};


// Единица кэша значений юниформов:
struct ShaderCacheUniformValue {
    ShaderCacheUniformType type;  // Тип значения.
    int32_t location;  // Позиция в шейдере.
    union {  // Данные:
        bool vbool;
        int32_t vint;
        float vfloat;
        float vec2[2];
        float vec3[3];
        float vec4[4];
    };
};


// Единица кэша сэмплеров:
struct ShaderCacheSampler {
    int32_t  location;
    uint32_t tex_id;
    uint32_t unit_id;
};


// Структура шейдера:
struct Shader {
    const char* vertex;
    const char* fragment;
    const char* geometry;
    char* error;
    uint32_t id;
    Renderer *renderer;
    bool _is_begin_;
    int32_t _id_before_begin_;

    // Динамические списки для кэша параметров шейдера:
    Array *uniform_locations;  // Кэш позиций uniform.
    Array *uniform_values;     // Кэш значений uniform (всё кроме массивов и матриц).
    Array *sampler_units;      // Кэш привязки текстурных юнитов к названиям униформов.
};


// -------- API шейдера: --------


// Создать шейдерную программу:
Shader* Shader_create(Renderer *renderer, const char *vert, const char *frag, const char *geom);

// Уничтожить шейдерную программу:
void Shader_destroy(Shader **shader);

// Компиляция шейдеров в программу:
void Shader_compile(Shader *self);

// Получить ошибку компиляции или линковки:
const char* Shader_get_error(Shader *self);

// Активация программы:
void Shader_begin(Shader *self);

// Деактивация программы:
void Shader_end(Shader *self);

// Получить локацию переменной:
int32_t Shader_get_location(Shader *self, const char* name);

// Установить значение bool:
void Shader_set_bool(Shader *self, const char* name, bool value);

// Установить значение int:
void Shader_set_int(Shader *self, const char* name, int value);

// Установить значение float:
void Shader_set_float(Shader *self, const char* name, float value);

// Установить значение vec2:
void Shader_set_vec2(Shader *self, const char* name, Vec2f value);

// Установить значение vec3:
void Shader_set_vec3(Shader *self, const char* name, Vec3f value);

// Установить значение vec4:
void Shader_set_vec4(Shader *self, const char* name, Vec4f value);

// Установить значение mat2:
void Shader_set_mat2(Shader *self, const char* name, mat2 value);

// Установить значение mat3:
void Shader_set_mat3(Shader *self, const char* name, mat3 value);

// Установить значение mat4:
void Shader_set_mat4(Shader *self, const char* name, mat4 value);

// Установить значение mat2x3:
void Shader_set_mat2x3(Shader *self, const char* name, mat2x3 value);

// Установить значение mat3x2:
void Shader_set_mat3x2(Shader *self, const char* name, mat3x2 value);

// Установить значение mat2x4:
void Shader_set_mat2x4(Shader *self, const char* name, mat2x4 value);

// Установить значение mat4x2:
void Shader_set_mat4x2(Shader *self, const char* name, mat4x2 value);

// Установить значение mat3x4:
void Shader_set_mat3x4(Shader *self, const char* name, mat3x4 value);

// Установить значение mat4x3:
void Shader_set_mat4x3(Shader *self, const char* name, mat4x3 value);

// Установить 2D текстуру:
void Shader_set_tex2d(Shader *self, const char* name, uint32_t tex_id);

// Установить 3D текстуру:
void Shader_set_tex3d(Shader *self, const char* name, uint32_t tex_id);
