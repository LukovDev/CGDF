//
// font.c - Реализация функционала для рендеринга текста.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/libs.h>
#include <cgdf/core/mm.h>
#include "font.h"


// -------- Вспомогательные функции: --------


// -------- API шрифта: --------


// Создать растровый шрифт:
FontPixmap* FontPixmap_create() {
    FontPixmap *font = (FontPixmap*)mm_alloc(sizeof(FontPixmap));

    // Заполняем поля:
    return font;
}

// Уничтожить растровый шрифт:
void FontPixmap_destroy(FontPixmap **font) {
    if (!font || !*font) return;

    mm_free(*font);
    *font = NULL;
}
