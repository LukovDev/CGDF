//
// font.h - Функционал для рендеринга текста.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/libs.h>


// Объявление структур:
typedef struct FontPixmap FontPixmap;  // Растровый шрифт.


// Растровый шрифт:
struct FontPixmap {
    //
};


// -------- API шрифта: --------


// Создать растровый шрифт:
FontPixmap* FontPixmap_create();

// Уничтожить растровый шрифт:
void FontPixmap_destroy(FontPixmap **font);
