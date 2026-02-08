//
// shader.c - Реализует работу с шейдерами для OpenGL.
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


static ShaderCacheUniformValue* _find_cached_uniform_(Shader *self, int loc, ShaderCacheUniformType type) {
    for (size_t i = 0; i < Array_len(self->uniform_values); i++) {
        ShaderCacheUniformValue *item = (ShaderCacheUniformValue*)Array_get(self->uniform_values, i);
        if (item->location == loc && item->type == type) return item;
    }
    return NULL;
}

static ShaderCacheSampler* _find_cached_sampler_(Shader *self, int32_t location) {
    for (size_t i = 0; i < Array_len(self->sampler_units); i++) {
        ShaderCacheSampler *item = (ShaderCacheSampler*)Array_get(self->sampler_units, i);
        if (item->location == location) return item;
    }
    return NULL;
}

static const char* _shader_type_str_(GLenum type) {
    switch (type) {
        case GL_VERTEX_SHADER:   return "Vertex";
        case GL_FRAGMENT_SHADER: return "Fragment";
        case GL_GEOMETRY_SHADER: return "Geometry";
        default:                 return "Unknown";
    }
}

static void _cleanup_shaders_(uint32_t program, uint32_t shaders[3], bool attached[3]) {
    for (int i = 0; i < 3; ++i) {
        if (shaders[i]) {
            if (program && attached[i]) glDetachShader(program, shaders[i]);
            glDeleteShader(shaders[i]);
        }
    }
    if (program) glDeleteProgram(program);
}

static char* _get_shader_info_log_(GLuint obj, bool is_program) {
    GLint len = 0;
    if (is_program) glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
    else glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);

    if (len <= 1) return mm_strdup("No shader source.");

    char *buf = mm_alloc(len);
    if (is_program) glGetProgramInfoLog(obj, len, NULL, buf);
    else glGetShaderInfoLog(obj, len, NULL, buf);
    return buf;
}

static void _shader_set_error_(Shader *self, const char *fmt, ...) {
    if (!self) return;
    if (self->error) { mm_free(self->error); self->error = NULL; }

    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    self->error = mm_alloc(needed + 1);

    va_start(ap, fmt);
    vsnprintf(self->error, needed + 1, fmt, ap);
    va_end(ap);

    log_msg("%s", self->error);
}

static void _clear_caches_(Shader *shader, bool destroy_arrays) {
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
        // Освобождаем юниты текстур:
        TexUnits_release_shader(shader->id);
        if (destroy_arrays) { Array_destroy(&shader->sampler_units); }
        else { Array_clear(shader->sampler_units, false); }
    }
}

static uint32_t _compile_shader_(Shader *program, const char* source, GLenum type) {
    const char* type_str = _shader_type_str_(type);

    if (!source) {
        _shader_set_error_(program, "ShaderCompileError (%s): source is NULL\n", type_str);
        return 0;
    }

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
        char *log_msg_str = _get_shader_info_log_(shader, false);
        _shader_set_error_(program, "ShaderCompileError (%s):\n%s\n", type_str, log_msg_str);
        mm_free(log_msg_str);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static void _set_sampler_(Shader *self, const char* name, uint32_t tex_id, TextureType type) {
    int32_t loc = Shader_get_location(self, name);
    if (loc < 0) return; // Униформа не найдена.

    /*
    Как работает? Специально для этого был реализован менеджер управления
    текстурными юнитами (texunit.c/.h -> TexUnits).

    Концептуально работает так:
    Шейдер при установке текстуры юниформе проверяет у себя в кэше:
    - Если нет кэша -> выделяем юнит, привязываем юнит к юниформе, создаём кэш, привязываем текстуру.
    - Если кэш есть и текстуры совпадают -> ничего не делаем.
    - Если кэш есть, но текстуры не совпадают -> перепривязываем текстуру к юниту и обновляем кэш.
    При удалении шейдера: Делаем запрос на освобождение выделенных для шейдера юнитов.
    */

    /* (копия пометки из texunit.c):
    ВНИМАНИЕ:
        Мы резервируем нулевой юнит.
        Нельзя шейдерам использовать нулевой юнит glActiveTexture(GL_TEXTURE0).
        Потому что нулевой юнит общий, и зарезервирован глобально для всех вызовов привязки текстур.
        Если вы всё равно укажете нулевой юнит для шейдера, то скорее всего, шейдер может
        получить другую текстуру, из за возможных вызовов привязки других текстур к этому юниту.
    */

    // Ищем сэмплер в кэше:
    ShaderCacheSampler *smp = _find_cached_sampler_(self, loc);

    // 1. Первый раз видим эту юниформу -> запрашиваем резервирование юнита:
    if (!smp) {
        // Выделяем юнит:
        int unit = TexUnits_reserve(self->id, loc);
        if (unit < 0) {
            log_msg("[!] Error (from _set_sampler_): no free texture units for %s\n", name);
            return;
        }

        // Привязываем sampler к юниту ОДИН раз:
        glUniform1i(loc, unit);

        // Создаём кэш:
        ShaderCacheSampler cache = {
            .location = loc,
            .tex_id   = tex_id,
            .unit_id  = unit
        };
        Array_push(self->sampler_units, &cache);

        // Привязываем текстуру в уже зарезервированный юнит:
        TexUnits_rebind_owned(self->id, loc, tex_id, type);
        return;
    }

    // 2. Кэш есть:
    if (smp->tex_id == tex_id) { return; }  // Всё совпадает, ничего делать не нужно.

    // 3. Та же юниформа, но другая текстура:
    smp->tex_id = tex_id;
    TexUnits_rebind_owned(self->id, loc, tex_id, type);
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
    _clear_caches_(*shader, true);

    // Удаляем сам шейдер:
    Shader_end(*shader);
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

    // Очищаем старые ошибки:
    if (self->error) {
        mm_free(self->error);
        self->error = NULL;
    }

    // Проверяем наличие шейдеров:
    if (!self->vertex || !self->fragment) {
        _shader_set_error_(self, "ShaderCreateError: Vertex and Fragment shaders are required.\n");
        return;
    }

    // Удаляем шейдерную программу, если она существует:
    if (self->id) {
        glDeleteProgram(self->id);
        self->id = 0;
    }

    // Создаём шейдерную программу:
    uint32_t program = glCreateProgram();
    uint32_t shaders[3] = {0};
    bool attached[3] = {false, false, false};

    if (!program) {
        _shader_set_error_(self, "ShaderCreateError: The OpenGL context has not been created or is inactive.\n");
        return;
    }

    // Компилируем шейдеры:
    shaders[0] = _compile_shader_(self, self->vertex, GL_VERTEX_SHADER);
    if (!shaders[0]) { _cleanup_shaders_(program, shaders, attached); return; }
    shaders[1] = _compile_shader_(self, self->fragment, GL_FRAGMENT_SHADER);
    if (!shaders[1]) { _cleanup_shaders_(program, shaders, attached); return; }
    if (self->geometry) {
        shaders[2] = _compile_shader_(self, self->geometry, GL_GEOMETRY_SHADER);
        if (!shaders[2]) { _cleanup_shaders_(program, shaders, attached); return; }
    }

    // Линкуем программу:
    for (int i = 0; i < 3; ++i) {
        if (shaders[i]) {
            glAttachShader(program, shaders[i]);
            attached[i] = true;
        }
    }
    glLinkProgram(program);

    // Проверяем статус линковки:
    int linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char *log_msg_str = _get_shader_info_log_(program, true);
        _shader_set_error_(self, "ShaderLinkingError:\n%s\n", log_msg_str);
        mm_free(log_msg_str);
        _cleanup_shaders_(program, shaders, attached);
        return;
    }

    // Удаляем отдельные шейдеры:
    for (int i = 0; i < 3; ++i) {
        if (shaders[i]) {
            if (attached[i]) glDetachShader(program, shaders[i]);
            glDeleteShader(shaders[i]);
        }
    }
    self->id = program;

    // Очищаем кэш:
    _clear_caches_(self, false);
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
    if (self->_id_before_begin_ != self->id) {
        glUseProgram(self->id);
    }
    self->_is_begin_ = true;
}

// Деактивация программы:
void Shader_end(Shader *self) {
    if (!self) return;
    if (self->_id_before_begin_ != self->id) {
        glUseProgram((uint32_t)self->_id_before_begin_);
    }
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
    int32_t location = glGetUniformLocation(self->id, name);
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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_BOOL);

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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_INT);

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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_FLOAT);

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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_VEC2);

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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_VEC3);

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
    ShaderCacheUniformValue *u = _find_cached_uniform_(self, loc, SHADERCACHE_UNIFORM_VEC4);

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
    if (!self || !self->_is_begin_ || !name) return;
    _set_sampler_(self, name, tex_id, TEX_TYPE_2D);
}

// Установить 3D текстуру:
void Shader_set_tex3d(Shader *self, const char* name, uint32_t tex_id) {
    if (!self || !self->_is_begin_ || !name) return;
    _set_sampler_(self, name, tex_id, TEX_TYPE_3D);
}

// Очистить кэши шейдера:
void Shader_clear_caches(Shader *self) {
    if (!self) return;
    _clear_caches_(self, false);
}
