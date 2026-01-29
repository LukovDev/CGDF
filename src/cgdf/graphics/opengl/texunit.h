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

// Привязать текстуру к юниту:
int TexUnits_bind(uint32_t tex_id, TextureType type);

// Отвязать текстуру от юнита:
int TexUnits_unbind(uint32_t tex_id);

// Перепривязать текстуру к конкретному юниту:
int TexUnits_rebind(uint32_t tex_id, uint32_t unit_id, TextureType type);

// Получить айди юнита к которому привязана текстура:
int TexUnits_get_unit_id(uint32_t tex_id);

// Получить айди текстуры по айди юнита:
uint32_t TexUnits_get_tex_id(uint32_t unit_id);

// Отвязать все текстуры:
void TexUnits_unbind_all();

// Сбросить активный юнит:
void TexUnits_reset_active();

// Получить всего возможных юнитов:
size_t TexUnits_get_total_units();

// Получить количество занятых юнитов:
size_t TexUnits_get_used_units();

// Получить количество свободных юнитов:
size_t TexUnits_get_free_units();
