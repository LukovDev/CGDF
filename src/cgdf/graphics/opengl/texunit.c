//
// texunit.c - Реализует работу с привязкой текстур к текстурным юнитам.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
#include "../texture.h"
#include "gl.h"
#include "texunit.h"


// Определения:
#define TEXUNITS_RESET_ACTIVE0 true  // Сбрасывать ли активную текстуру на нулевую или нет (экспериментальный параметр).


// Создаём единую глобальную структуру:
TextureUnits texunits_gl = {0};


// Конвертируем тип текстуры в GL-тип:
static uint32_t TextureType_to_gl(TextureType type) {
    switch(type) {
        case TEX_TYPE_3D: return GL_TEXTURE_3D;
        default: return GL_TEXTURE_2D;
    }
}


// Ищем текстуру в стеке юнитов, и удаляем запись если айди юнита не совпадает с новым айди юнита:
static void check_duplications(uint32_t tex_id, uint32_t unit_id) {
    // Этот кусок кода предотвращает множественное ошибочное присваивание текстуры к нескольким юнитам.
    for (size_t i=0; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);
        // Если нашли по текстуре, но юниты не совпадают - сбрасываем привязку:
        if (unit->tex_id == tex_id && i != unit_id) {
            glActiveTexture(GL_TEXTURE0+i);
            glBindTexture(unit->type, 0);
            if (TEXUNITS_RESET_ACTIVE0) glActiveTexture(GL_TEXTURE0);
            unit->tex_id = 0;
            unit->type = GL_TEXTURE_2D;
            unit->used = false;
            if (texunits_gl.used > 0) texunits_gl.used--;
        }
    }
}


// Инициализировать текстурные юниты (стек):
void TextureUnits_init() {
    // Узнаём сколько возможно привязывать юнитов:
    int max_units = 0;  // Обычно 16-32 юнита.
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_units);

    // Если значение нулевое, значит что-то пошло не так:
    if (max_units <= 0) {
        log_msg("[!] Warning (from TextureUnits_init): GL_MAX_TEXTURE_IMAGE_UNITS returned 0. Using fallback = 16.\n");
        max_units = 16;  // Аварийно используем минимальное количество.
    }

    // Инициализируем стек:
    texunits_gl.stack = Array_create(sizeof(TexUnit), max_units);
    for (size_t i=0; i < Array_capacity(texunits_gl.stack); i++) {
        Array_push(texunits_gl.stack, &(TexUnit){
            .tex_id = 0,
            .type = GL_TEXTURE_2D,
            .used = false,
        });
    }
    texunits_gl.total = Array_capacity(texunits_gl.stack);  // Capacity использовать безопаснее.
    texunits_gl.used = 0;

    // Резервируем нулевой юнит, чтобы избежать проблемы:
    // Когда привязываешь текстуру к нулевому юниту, возможен случай
    // что другая текстура может перепривязаться к нулевому юниту
    // и в шейдере будет использована другая текстура.
    // Проблема была выявлена и исправлена ниже, в ходе экспериментов.
    if (TEXUNITS_RESET_ACTIVE0) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, 0);
        unit->used = true;
        unit->tex_id = 0;
        texunits_gl.used++;
    }
}


// Уничтожить текстурные юниты:
void TextureUnits_destroy() {
    // Отвязываем текстурные юниты:
    TexUnits_unbind_all();
    TexUnits_reset_active();

    // Удаляем стек:
    Array_destroy(&texunits_gl.stack);
    texunits_gl.total = 0;
    texunits_gl.used = 0;
}


// Привязать текстуру к юниту:
int TexUnits_bind(uint32_t tex_id, TextureType type) {
    if (!tex_id) return -1;

    // Проверяем, привязана ли текстура уже к юниту (ничего не делаем если уже привязана):
    int unit_id = TexUnits_get_unit_id(tex_id);
    if (unit_id != -1) return unit_id;

    // Определяем тип текстуры:
    uint32_t texture_type = TextureType_to_gl(type);

    // Ищем свободное место в стеке юнитов:
    for (size_t i=0; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);

        // Если юнит свободен, то привязываем текстуру:
        if (unit->used == false) {
            glActiveTexture(GL_TEXTURE0+i);
            glBindTexture(texture_type, tex_id);
            if (TEXUNITS_RESET_ACTIVE0) glActiveTexture(GL_TEXTURE0);
            unit->tex_id = tex_id;
            unit->type = texture_type;
            unit->used = true;
            texunits_gl.used++;
            return i;  // Возвращаем айди юнита.
        }
    }
    return -1;  // Мы не нашли свободного юнита.
}


// Отвязать текстуру от юнита:
int TexUnits_unbind(uint32_t tex_id) {
    if (!tex_id) return -1;

    // Ищем текстуру в стеке юнитов:
    for (size_t i=0; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);
        if (unit->tex_id == tex_id) {
            glActiveTexture(GL_TEXTURE0+i);
            glBindTexture(unit->type, 0);
            if (TEXUNITS_RESET_ACTIVE0) glActiveTexture(GL_TEXTURE0);
            unit->tex_id = 0;
            unit->type = GL_TEXTURE_2D;
            unit->used = false;
            if (texunits_gl.used > 0) texunits_gl.used--;
            return i;  // Возвращаем айди юнита.
        }
    }
    return -1;  // Текстура не была привязана.
}


// Перепривязать текстуру к конкретному юниту:
int TexUnits_rebind(uint32_t tex_id, uint32_t unit_id, TextureType type) {
    if (!tex_id || unit_id >= texunits_gl.total) return -1;

    // Определяем тип текстуры:
    uint32_t texture_type = TextureType_to_gl(type);

    // Проверяем на существующую привязку:
    check_duplications(tex_id, unit_id);

    // Перепривязываем текстуру к юниту:
    if (unit_id < Array_len(texunits_gl.stack)) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, unit_id);
        glActiveTexture(GL_TEXTURE0+unit_id);
        glBindTexture(texture_type, tex_id);
        if (TEXUNITS_RESET_ACTIVE0) glActiveTexture(GL_TEXTURE0);
        if (!unit->used || unit->tex_id != tex_id) texunits_gl.used++;
        unit->tex_id = tex_id;
        unit->type = texture_type;
        unit->used = true;
        return unit_id;  // Возвращаем айди юнита.
    }
    return -1;  // Текстура не была привязана.
}


// Получить айди юнита к которому привязана текстура:
int TexUnits_get_unit_id(uint32_t tex_id) {
    if (!tex_id) return -1;

    // Ищем юнит к которому привязана текстура:
    for (size_t i=0; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);
        if (unit->tex_id == tex_id) return i;  // Возвращаем айди юнита.
    }
    return -1;  // Не нашли.
}


// Получить айди текстуры по айди юнита:
uint32_t TexUnits_get_tex_id(uint32_t unit_id) {
    if (unit_id >= texunits_gl.total) return 0;
    TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, unit_id);
    return unit->tex_id;
}


// Отвязать все текстуры:
void TexUnits_unbind_all() {
    size_t reserve = TEXUNITS_RESET_ACTIVE0;  // Резервируем первый юнит.
    for (size_t i=reserve; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);
        if (unit->used) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(unit->type, 0);
            unit->tex_id = 0;
            unit->type = GL_TEXTURE_2D;
            unit->used = false;
        }
    }
    texunits_gl.used = reserve;
}


// Сбросить активный юнит:
void TexUnits_reset_active() {
    glActiveTexture(GL_TEXTURE0);
}


// Получить всего возможных юнитов:
size_t TexUnits_get_total_units() {
    return texunits_gl.total;
}


// Получить количество занятых юнитов:
size_t TexUnits_get_used_units() {
    return texunits_gl.used;
}


// Получить количество свободных юнитов:
size_t TexUnits_get_free_units() {
    return texunits_gl.total - texunits_gl.used;
}
