//
// texunit.c - Реализует работу с привязкой текстур к текстурным юнитам.
//
// Работает по принципу "выделения" определенных юнитов для шейдеров.
// Шейдер запрашивает выделение юнитов, меняет в них текстуру, а другие
// шейдеры не могут получить доступ к этим "заприваченным" юнитам.
// Также шейдер может разом отвязать все выделенные для себя юниты,
// чтобы другие шейдеры могли их переиспользовать для себя.
//

/* (копия пометки из shader.c):
ВНИМАНИЕ:
    Мы резервируем нулевой юнит.
    Нельзя шейдерам использовать нулевой юнит glActiveTexture(GL_TEXTURE0).
    Потому что нулевой юнит общий, и зарезервирован глобально для всех вызовов привязки текстур.
    Если вы всё равно укажете нулевой юнит для шейдера, то скорее всего, шейдер может
    получить другую текстуру, из за возможных вызовов привязки других текстур к этому юниту.
*/


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
#include "../texture.h"
#include "gl.h"
#include "texunit.h"


// Создаём единую глобальную структуру:
TextureUnits texunits_gl = {0};


// -------- Вспомогательные функции: --------


// Конвертируем тип текстуры в GL-тип:
static uint32_t TextureType_to_gl(TextureType type) {
    switch(type) {
        case TEX_TYPE_3D: return GL_TEXTURE_3D;
        case TEX_TYPE_2D:  // 2D по умолчанию.
        default: return GL_TEXTURE_2D;
    }
}


// -------- Основной код: --------


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

    // Резервируем нулевой юнит. Запрещаем его выдавать шейдерам.
    // Дело в том, что если шейдер заберет нулевой юнит, то если
    // в коде будет привязка другой текстуры к этому юниту
    // (юнит это ActiveTexture. Нулевой у нас всегда по умолчанию глобально),
    // то в шейдере текстура сменится. По этому мы резервируем нулевой
    // юнит как глобальный для всех мест в коде, где мы привязываем текстуру.
    TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, 0);
    unit->shd_id = 0;
    unit->loc_id = 0;
    unit->tex_id = 0;
    unit->used = true;
    texunits_gl.used++;
}

// Уничтожить текстурные юниты:
void TextureUnits_destroy() {
    // Отвязываем текстурные юниты:
    TexUnits_unbind_all();

    // Удаляем стек:
    Array_destroy(&texunits_gl.stack);
    texunits_gl.total = 0;
    texunits_gl.used = 0;
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

// Отвязать все текстуры:
void TexUnits_unbind_all() {
    // Проходимся по стеку:
    for (size_t i=1; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *unit = (TexUnit*)Array_get(texunits_gl.stack, i);
        // Если юнит используется, то отвязываем его:
        if (unit->used) {
            if (unit->tex_id) {
                // Обнуляем бинд, сохраняя тип:
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(unit->type, 0);
            }
            unit->used   = false;
            unit->tex_id = 0;
            unit->shd_id = 0;
            unit->loc_id = 0;
        }
    }
    texunits_gl.used = 1;
    glActiveTexture(GL_TEXTURE0);
}

// Деактивировать определённую текстуру во всех юнитах:
void TexUnits_invalidate_texture(uint32_t tex_id) {
    if (!texunits_gl.stack || tex_id == 0) return;

    /*
        Эта функция должна использоваться внутри кода удаления текстуры.
        Потому что при удалении текстуры, OpenGL автоматически отвязывает
        её от текстурных юнитов. По этому нам надо учитывать это тоже.
        И чтобы не было сложностей, просто сразу автоматически отвязываем
        от всех наших юнитов.

        Таким образом мы корректно обрабатываем удаление текстуры и их
        автоматическую отвязку от текстурных юнитов.
    */

    // i = 1, потому что мы пропускаем нулевой юнит. Он разерервирован.
    for (size_t i = 1; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *u = (TexUnit*)Array_get(texunits_gl.stack, i);
        // Если юнит используется, и текстура такая же что наша:
        if (u->used && u->tex_id == tex_id) {
            // Просто отвязываем текстуру:
            glActiveTexture(GL_TEXTURE0 + (int)i);
            glBindTexture(u->type, 0);
            u->tex_id = 0;
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

// Получить номер юнита по (шейдер, локация):
int TexUnits_find(uint32_t shd_id, int32_t loc_id) {
    if (!texunits_gl.stack) return -1;

    // i = 1, потому что мы пропускаем нулевой юнит. Он разерервирован.
    for (size_t i = 1; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *u = Array_get(texunits_gl.stack, i);
        // Если используется (занят) + совпадает с (шейдер, локация):
        if (u->used && u->shd_id == shd_id && u->loc_id == loc_id) {
            return (int)i;  // Нашли зарезервированный юнит.
        }
    }
    return -1;  // Этого юнита не существует.
}

// Зарезервировать юнит для шейдера:
int TexUnits_reserve(uint32_t shd_id, int32_t loc_id) {
    if (!shd_id || loc_id < 0 || !texunits_gl.stack) return -1;

    /*
    - Если юнит уже закреплён за (shd_id, loc_id) -> вернуть его.
    - Иначе найти свободный.
    - Пометить владельца и вернуть номер.
    - НЕ биндим текстуру.
    */

    // 1. Уже зарезервирован?:
    int find_unit = TexUnits_find(shd_id, loc_id);
    if (find_unit >= 0) return find_unit;

    // 2. Ищем свободный и резервируем:
    // i = 1, потому что мы пропускаем нулевой юнит. Он разерервирован.
    for (size_t i = 1; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *u = Array_get(texunits_gl.stack, i);
        if (!u->used) {  // Если юнит свободный:
            u->used   = true;
            u->shd_id = shd_id;
            u->loc_id = loc_id;
            u->tex_id = 0;
            u->type   = TextureType_to_gl(TEX_TYPE_2D);
            texunits_gl.used++;
            return (int)i;  // Нашли свободный юнит.
        }
    }

    return -1; // Свободных юнитов нет.
}

// Перепривязать текстуру к юниту, которая уже зарезервирована для шейдера:
int TexUnits_rebind_owned(uint32_t shd_id, int32_t loc_id, uint32_t tex_id, TextureType type) {
    if (!shd_id || loc_id < 0 || !texunits_gl.stack) return -1;

    /*
    - Найти юнит по (shd_id, loc_id).
    - Если уже лежит нужная текстура (такая же) -> ничего не делать.
    - Иначе -> glBindTexture (перепривязываем новую).
    */

    // Ищем юнит по (shd_id, loc_id):
    int find_unit = TexUnits_find(shd_id, loc_id);
    uint32_t gl_type = TextureType_to_gl(type);

    // Нашли юнит:
    if (find_unit >= 0) {
        // Получаем указатель на конкретный юнит из стека:
        TexUnit *u = Array_get(texunits_gl.stack, find_unit);

        // Проверка - та же ли это текстура?:
        if (u->tex_id == tex_id && u->type == gl_type) {
            return (int)find_unit;  // Ничего не делаем.
        }

        // Иначе, значит текстура другая (или отсутствует) -> перепривязываем на нужную:
        glActiveTexture(GL_TEXTURE0 + find_unit);
        glBindTexture(gl_type, tex_id);
        glActiveTexture(GL_TEXTURE0);

        // Сохраняем новые параметры:
        u->tex_id = tex_id;
        u->type   = gl_type;
        return (int)find_unit;
    }
    return -1; // Юнит не был зарезервирован.
}

// Освободить все юниты, которые зарезервированы для шейдера (используйте только при удалении шейдера!):
void TexUnits_release_shader(uint32_t shd_id) {
    if (!shd_id || !texunits_gl.stack) return;

    /*
    - Освободить ВСЕ юниты, принадлежащие шейдеру.
    - Корректно отвязать текстуры.
    - Вернуть юниты в пул (стек).
    */

    // Проход по всему стеку юнитов:
    for (size_t i = 0; i < Array_len(texunits_gl.stack); i++) {
        TexUnit *u = Array_get(texunits_gl.stack, i);

        // Если используется (занят) + совпадает с айди шейдера, значит нашли запись:
        if (u->used && u->shd_id == shd_id) {
            // Обнулять бинд не надо (используя glBindTexture), потому что
            // мы уже пометили, что юнит свободен. TexUnits_rebind_owned
            // перебиндит текстуру. Так мы экономим переключения контекста.

            // Стираем запись:
            u->used   = false;
            u->tex_id = 0;
            u->shd_id = 0;
            u->loc_id = 0;
            texunits_gl.used--;
        }
    }
}
