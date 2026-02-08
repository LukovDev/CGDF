//
// texunit.h
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>
#include "../texture.h"


// Объявление структур:
typedef struct TextureUnits TextureUnits;
typedef struct TexUnit TexUnit;


// Текстурные юниты:
struct TextureUnits {
    Array *stack;  // Стек юнитов и привязок.
    size_t total;  // Всего юнитов.
    size_t used;   // Использовано юнитов.
};


// Один текстурный юнит:
struct TexUnit {
    uint32_t shd_id;  // Айди шейдера.
    uint32_t loc_id;  // Локация юниформа.
    uint32_t tex_id;  // Айди текстуры.
    uint32_t type;    // Тип текстуры.
    bool     used;    // Флаг использования.
};


// Создаём единую глобальную структуру:
extern TextureUnits texunits_gl;


// Инициализировать текстурные юниты (вызывается автоматически):
void TextureUnits_init();

// Уничтожить текстурные юниты (вызывается автоматически):
void TextureUnits_destroy();

// Получить всего возможных юнитов:
size_t TexUnits_get_total_units();

// Получить количество занятых юнитов:
size_t TexUnits_get_used_units();

// Получить количество свободных юнитов:
size_t TexUnits_get_free_units();

// Отвязать все текстуры:
void TexUnits_unbind_all();

// Деактивировать определённую текстуру во всех юнитах:
void TexUnits_invalidate_texture(uint32_t tex_id);

// Получить номер юнита по (шейдер, локация):
int TexUnits_find(uint32_t shd_id, int32_t loc_id);

// Зарезервировать юнит для шейдера:
int TexUnits_reserve(uint32_t shd_id, int32_t loc_id);

// Перепривязать текстуру к юниту, которая уже зарезервирована для шейдера:
int TexUnits_rebind_owned(uint32_t shd_id, int32_t loc_id, uint32_t tex_id, TextureType type);

// Освободить все юниты, которые зарезервированы для шейдера (используйте только при удалении шейдера!):
void TexUnits_release_shader(uint32_t shd_id);
