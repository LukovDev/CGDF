//
// shader.c - Реализует работу с шейдерами.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
#include "../texture.h"
#include "../shader.h"
#include "texunit.h"
#include "gl.h"


// -------- Вспомогательные функции: --------


static inline ShaderCacheUniformValue* find_cached_uniform(Shader *self, int loc, ShaderCacheUniformType type) {
    for (size_t i = 0; i < Array_len(self->uniform_values); i++) {
        ShaderCacheUniformValue *item = (ShaderCacheUniformValue*)Array_get(self->uniform_values, i);
        if (item->location == loc && item->type == type) return item;
    }
    return NULL;
}

static inline ShaderCacheSampler* find_cached_sampler(Shader *self, int32_t location) {
    for (size_t i = 0; i < Array_len(self->sampler_units); i++) {
        ShaderCacheSampler *item = (ShaderCacheSampler*)Array_get(self->sampler_units, i);
        if (item->location == location) return item;
    }
    return NULL;
}

static inline void clear_caches(Shader *shader, bool destroy_arrays) {
    if (!shader) return;

    // Освобождаем кэш локаций:
    if (shader->uniform_locations) {
        for (size_t i = 0; i < Array_len(shader->uniform_locations); i++) {
            ShaderCacheUniformLocation *u = Array_get(shader->uniform_locations, i);
            if (u->name) mm_free(u->name);
        }
        if (destroy_arrays) { Array_destroy(&shader->uniform_locations); }
        else { Array_clear(shader->uniform_locations, false); }
    }

    // Освобождаем кэш юниформов:
    if (shader->uniform_values) {
        if (destroy_arrays) { Array_destroy(&shader->uniform_values); }
        else { Array_clear(shader->uniform_values, false); }
    }

    // Освобождаем кэш юнитов:
    if (shader->sampler_units) {
        for (size_t i = 0; i < Array_len(shader->sampler_units); i++) {
            ShaderCacheSampler *s = Array_get(shader->sampler_units, i);
            if (s->tex_id) { TexUnits_unbind(s->tex_id); }
        }
        if (destroy_arrays) { Array_destroy(&shader->sampler_units); }
        else { Array_clear(shader->sampler_units, false); }
    }
}

static inline void set_sampler(Shader *self, const char* name, uint32_t tex_id, TextureType type) {
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем самплер в кэше:
    ShaderCacheSampler *s = find_cached_sampler(self, loc);

    // Проверяем, есть ли текстура в стеке юнитов (-1 = нет):
    int uid = TexUnits_get_unit_id(tex_id);

    // Если в кэше есть запись, но текстура реально НЕ привязана - нужно перебиндить:
    if (s && uid == -1) {
        // Убрали извне - забываем старый unit, просто перебиндим:
        uint32_t unit_id = TexUnits_bind(tex_id, type);
        s->tex_id = tex_id;
        glUniform1i(loc, unit_id);
        return;
    }

    // Если в кэше есть запись и tex_id совпадает - ничего делать не нужно:
    if (s && s->tex_id == tex_id) return;

    // Если текстура есть в юнитах, но в кэше нет - создаём запись:
    if (!s && uid != -1) {
        ShaderCacheSampler cache = {
            .location = loc,
            .tex_id = tex_id
        };
        Array_push(self->sampler_units, &cache);
        glUniform1i(loc, uid);
        return;
    }

    // Иначе: запись либо новая, либо изменённая, либо отвязанная:
    uint32_t unit_id = TexUnits_bind(tex_id, type);

    if (!s) {
        ShaderCacheSampler cache = {
            .location = loc,
            .tex_id = tex_id
        };
        Array_push(self->sampler_units, &cache);
    } else { s->tex_id = tex_id; }
    glUniform1i(loc, unit_id);
}

static inline uint32_t compile_shader(Shader *program, const char* source, GLenum type) {
    if (!source) return 0;
    if (program->error) {
        mm_free(program->error);
        program->error = NULL;
    }

    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        int logLength = 0;
        char* log_msg_str = "No shader source.";
        bool has_source = program->vertex || program->fragment || program->geometry;
        if (has_source) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            log_msg_str = mm_alloc(logLength);
            glGetShaderInfoLog(shader, logLength, NULL, log_msg_str);
        }
        // Тип шейдера:
        const char* type_str = (type == GL_VERTEX_SHADER)   ? "VERTEX"   :
                               (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" :
                               (type == GL_GEOMETRY_SHADER) ? "GEOMETRY" : "UNKNOWN";
        // Сколько надо выделить памяти:
        int needed = snprintf(NULL, 0, "ShaderCompileError (%s):\n%s\n", type_str, log_msg_str);
        program->error = mm_alloc(needed + 1);
        // Форматируем строку:
        sprintf(program->error, "ShaderCompileError (%s):\n%s\n", type_str, log_msg_str);
        log_msg("%s", program->error);
        if (has_source) mm_free(log_msg_str);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


// -------- API шейдера: --------


// Создать шейдерную программу (поля на vert, frag, geom, могут быть освобождены ТОЛЬКО после компиляции шейдера!):
Shader* Shader_create(Renderer *renderer, const char *vert, const char *frag, const char *geom) {
    if (!renderer) return NULL;

    // Создаём шейдер:
    Shader *shader = (Shader*)mm_alloc(sizeof(Shader));

    // Заполняем поля:
    shader->vertex = vert;
    shader->fragment = frag;
    shader->geometry = geom;
    shader->error = NULL;
    shader->id = 0;
    shader->renderer = renderer;
    shader->_is_begin_ = false;
    shader->_id_before_begin_ = 0;
    shader->uniform_locations = Array_create(sizeof(ShaderCacheUniformLocation), 128);
    shader->uniform_values = Array_create(sizeof(ShaderCacheUniformValue), 128);
    shader->sampler_units = Array_create(sizeof(ShaderCacheSampler), 128);
    return shader;
}

// Уничтожить шейдерную программу:
void Shader_destroy(Shader **shader) {
    if (!shader || !*shader) return;

    // Освобождаем кэш:
    clear_caches(*shader, true);

    // Удаляем сам шейдер:
    if ((*shader)->id) {
        glDeleteProgram((*shader)->id);
        (*shader)->id = 0;
    }

    // Освобождаем структуру:
    if ((*shader)->error) mm_free((*shader)->error);
    mm_free(*shader);
    *shader = NULL;
}

// Компиляция шейдеров в программу:
void Shader_compile(Shader *self) {
    if (!self) return;

    uint32_t program = glCreateProgram();
    uint32_t shaders[3] = {0};

    if (!program) {
        // Сколько надо выделить памяти:
        int needed = snprintf(NULL, 0, "ShaderCreateError: The OpenGL context has not been created or is inactive.\n");
        self->error = mm_alloc(needed + 1);
        // Форматируем строку:
        sprintf(self->error, "ShaderCreateError: The OpenGL context has not been created or is inactive.\n");
        log_msg("%s", self->error);
        return;
    }

    // Компилируем шейдеры:
    if (self->vertex)   shaders[0] = compile_shader(self, self->vertex, GL_VERTEX_SHADER);
    if (self->fragment) shaders[1] = compile_shader(self, self->fragment, GL_FRAGMENT_SHADER);
    if (self->geometry) shaders[2] = compile_shader(self, self->geometry, GL_GEOMETRY_SHADER);

    // Линкуем программу:
    for (int i = 0; i < 3; ++i) {
        if (shaders[i]) glAttachShader(program, shaders[i]);
    }
    glLinkProgram(program);

    // Проверяем статус линковки:
    int linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        int logLength = 0;
        char* log_msg_str = "No shader source.";
        bool has_source = self->vertex || self->fragment || self->geometry;
        if (has_source) {
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            log_msg_str = mm_alloc(logLength);
            glGetProgramInfoLog(program, logLength, NULL, log_msg_str);
        }
        // Сколько надо выделить памяти:
        int needed = snprintf(NULL, 0, "ShaderLinkingError:\n%s\n", log_msg_str);
        self->error = mm_alloc(needed + 1);
        // Форматируем строку:
        sprintf(self->error, "ShaderLinkingError:\n%s\n", log_msg_str);
        log_msg("%s", self->error);
        if (has_source) mm_free(log_msg_str);
        glDeleteProgram(program);
        return;
    }

    // Удаляем отдельные шейдеры:
    for (int i = 0; i < 3; ++i) {
        if (shaders[i]) {
            glDetachShader(program, shaders[i]);
            glDeleteShader(shaders[i]);
        }
    }
    self->id = program;

    // Очищаем кэш:
    clear_caches(self, false);
}

// Получить ошибку компиляции или линковки:
const char* Shader_get_error(Shader *self) {
    if (!self) return NULL;
    return self->error;
}

// Активация программы:
void Shader_begin(Shader *self) {
    if (!self) return;
    glGetIntegerv(GL_CURRENT_PROGRAM, &self->_id_before_begin_);
    glUseProgram(self->id);
    self->_is_begin_ = true;
}

// Деактивация программы:
void Shader_end(Shader *self) {
    if (!self) return;
    glUseProgram((uint32_t)self->_id_before_begin_);
    self->_is_begin_ = false;
}

// Получить локацию переменной:
int32_t Shader_get_location(Shader *self, const char* name) {
    if (!self) return -1;

    // Ищем и возвращаем локацию в кэше:
    for (size_t i = 0; i < Array_len(self->uniform_locations); i++) {
        ShaderCacheUniformLocation *u = (ShaderCacheUniformLocation*)Array_get(self->uniform_locations, i);
        if (strcmp(u->name, name) == 0) {
            return u->location;
        }
    }

    // Иначе получаем локацию и добавляем в кэш:
    Shader_begin(self);
    int32_t location = glGetUniformLocation(self->id, name);
    Shader_end(self);
    if (location == -1) return -1;

    ShaderCacheUniformLocation cache = {
        .name = mm_strdup(name),
        .location = location
    };
    Array_push(self->uniform_locations, &cache);
    return location;
}

// Установить значение bool:
void Shader_set_bool(Shader *self, const char* name, bool value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_BOOL);

    // Если нашли:
    if (u) {
        if (u->vbool == value) return;  // Если значение не изменилось - выходим.
        u->vbool = value;  // Если значение изменилось - обновляем.
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_BOOL,
            .location = loc,
            .vbool = value
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform1i(loc, (int)value);
}

// Установить значение int:
void Shader_set_int(Shader *self, const char* name, int value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_INT);

    // Если нашли:
    if (u) {
        if (u->vint == value) return;  // Если значение не изменилось - выходим.
        u->vint = value;  // Если значение изменилось - обновляем.
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_INT,
            .location = loc,
            .vint = value
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform1i(loc, value);
}

// Установить значение float:
void Shader_set_float(Shader *self, const char* name, float value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_FLOAT);

    // Если нашли:
    if (u) {
        if (cmp_float(u->vfloat, value)) return;  // Если значение не изменилось - выходим.
        u->vfloat = value;  // Если значение изменилось - обновляем.
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_FLOAT,
            .location = loc,
            .vfloat = value
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform1f(loc, value);
}

// Установить значение vec2:
void Shader_set_vec2(Shader *self, const char* name, Vec2f value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_VEC2);

    // Если нашли:
    if (u) {
        // Если значение не изменилось - выходим:
        if (cmp_float(u->vec2[0], value.x) && cmp_float(u->vec2[1], value.y)) return;
        // Если значение изменилось - обновляем:
        u->vec2[0] = value.x;
        u->vec2[1] = value.y;
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_VEC2,
            .location = loc,
            .vec2[0] = value.x,
            .vec2[1] = value.y
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform2fv(loc, 1, (float*)&value);
}

// Установить значение vec3:
void Shader_set_vec3(Shader *self, const char* name, Vec3f value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_VEC3);

    // Если нашли:
    if (u) {
        // Если значение не изменилось - выходим:
        if (cmp_float(u->vec3[0], value.x) && cmp_float(u->vec3[1], value.y) && cmp_float(u->vec3[2], value.z)) return;
        // Если значение изменилось - обновляем:
        u->vec3[0] = value.x;
        u->vec3[1] = value.y;
        u->vec3[2] = value.z;
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_VEC3,
            .location = loc,
            .vec3[0] = value.x,
            .vec3[1] = value.y,
            .vec3[2] = value.z
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform3fv(loc, 1, (float*)&value);
}

// Установить значение vec4:
void Shader_set_vec4(Shader *self, const char* name, Vec4f value) {
    if (!self || !self->_is_begin_ || !name) return;
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    // Ищем униформу в кэше:
    ShaderCacheUniformValue *u = find_cached_uniform(self, loc, SHADERCACHE_UNIFORM_VEC4);

    // Если нашли:
    if (u) {
        // Если значение не изменилось - выходим:
        if (cmp_float(u->vec4[0], value.x) && cmp_float(u->vec4[1], value.y) &&
            cmp_float(u->vec4[2], value.z) && cmp_float(u->vec4[3], value.w)) return;
        // Если значение изменилось - обновляем:
        u->vec4[0] = value.x;
        u->vec4[1] = value.y;
        u->vec4[2] = value.z;
        u->vec4[3] = value.w;
    } else {  // Иначе добавляем новую запись:
        ShaderCacheUniformValue cache = {
            .type = SHADERCACHE_UNIFORM_VEC4,
            .location = loc,
            .vec4[0] = value.x,
            .vec4[1] = value.y,
            .vec4[2] = value.z,
            .vec4[3] = value.w
        };
        Array_push(self->uniform_values, &cache);
    }
    glUniform4fv(loc, 1, (float*)&value);
}

// Установить значение mat2:
void Shader_set_mat2(Shader *self, const char* name, mat2 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix2fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat3:
void Shader_set_mat3(Shader *self, const char* name, mat3 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat4:
void Shader_set_mat4(Shader *self, const char* name, mat4 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat2x3:
void Shader_set_mat2x3(Shader *self, const char* name, mat2x3 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix2x3fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat3x2:
void Shader_set_mat3x2(Shader *self, const char* name, mat3x2 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix3x2fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat2x4:
void Shader_set_mat2x4(Shader *self, const char* name, mat2x4 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix2x4fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat4x2:
void Shader_set_mat4x2(Shader *self, const char* name, mat4x2 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix4x2fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat3x4:
void Shader_set_mat3x4(Shader *self, const char* name, mat3x4 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix3x4fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить значение mat4x3:
void Shader_set_mat4x3(Shader *self, const char* name, mat4x3 value) {
    if (!self || !self->_is_begin_ || !name) return;  // Кэширование матриц и массивов слишком дорого и сложно.
    int32_t loc = Shader_get_location(self, name);
    glUniformMatrix4x3fv(loc, 1, GL_FALSE, (float*)value);
}

// Установить 2D текстуру:
void Shader_set_tex2d(Shader *self, const char* name, uint32_t tex_id) {
    if (!self || !self->_is_begin_ || !name || !tex_id) return;
    set_sampler(self, name, tex_id, TEX_TYPE_2D);
}

// Установить 3D текстуру:
void Shader_set_tex3d(Shader *self, const char* name, uint32_t tex_id) {
    if (!self || !self->_is_begin_ || !name || !tex_id) return;
    set_sampler(self, name, tex_id, TEX_TYPE_3D);
}
