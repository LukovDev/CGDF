//
// font.h - Функционал для рендеринга текста.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/libs.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include <cgdf/core/hashtable.h>
#include "renderer.h"
#include "texture.h"
#include "spritebatch.h"


// Определения:
#define FONT_ATLAS_SIZE    1024  // Размер атласа по умолчанию (в глифах).
#define FONT_ATLAS_PADDING 1     // Отступ между глифами со всех сторон (в пикселях).
#define FONT_ATLAS_SCALING 1.5   // Масштабирование атласа при расширении.
#define FONT_FALLBACK_SUMB '?'   // Замена нераспознанных символов.


// Перечисление точек центрирования:
typedef enum FontAlign {
    FONT_ALIGN_BOTTOM_LEFT = 0,
    FONT_ALIGN_BOTTOM_CENTER,
    FONT_ALIGN_BOTTOM_RIGHT,

    FONT_ALIGN_CENTER_LEFT,
    FONT_ALIGN_CENTER_CENTER,
    FONT_ALIGN_CENTER_RIGHT,

    FONT_ALIGN_TOP_LEFT,
    FONT_ALIGN_TOP_CENTER,
    FONT_ALIGN_TOP_RIGHT,
} FontAlign;


// Объявление структур:
typedef struct FontPixmap FontPixmap;        // Растровый шрифт.
typedef struct FontGlyph FontGlyph;          // Глиф.
typedef struct FontTextBlock FontTextBlock;  // Блок текста (размеры).


// Растровый шрифт:
struct FontPixmap {
    stbtt_fontinfo info;           // Информация о шрифте.
    Renderer       *renderer;      // Рендерер.
    Texture        *atlas;         // Атлас. Текстура с нашими символами.
    SpriteBatch    *batch;         // Пакетная отрисовка спрайтов (для нас - символов).
    Array          *glyphs_array;  // Массив глифов (просто коды символов). Нужен для расширения атласа.
    HashTable      *glyphs;        // Хэш-таблица глифов (символов). Нужен для немедленного доступа к глифам.
    Vec4f          color;          // Цвет текста.
    Vec4f          bg_color;       // Фоновый цвет.
    Vec4f          bg_padding;     // Отступы фона (left, top, right, bottom).
    bool           pixelized;      // Пикселизированный шрифт.
    int added_glyphs_count;        // Сколько глифов было добавлено в атлас. Нужен для авто-расширения атласа.
    unsigned char  *ttf_buffer;    // Буфер данных файла шрифта.
    FontAlign      align;          // Выравнивание текста.

    // Метрики которые нельзя редактировать:
    int   font_size;     // [don't edit] Размер шрифта (по высоте в пикселях).
    float scale;         // [don't edit] Коэффициент масштабирования (только для перевода внутренних единиц в пиксели).
    int   ascent;        // [don't edit] Верхняя часть символа.
    int   descent;       // [don't edit] Нижняя часть символа.
    int   line_gap;      // [don't edit] Расстояние между строками.
    float line_advance;  // [don't edit] Базовый шаг строки из шрифта (px). Используется при '\n'.

    // Метрики которые можно редактировать:
    float line_height;     // [editable] Смещение межстрочного интервала. Используется при '\n'.
    float letter_spacing;  // [editable] Дополнительный межбуквенный интервал (px).
    int   tab_size;        // [editable] Сколько пробелов в '\t'.
    float space_advance;   // [editable] Ширина пробела (px).
};


// Глиф:
struct FontGlyph {
    float u0, v0;    // Лево-низ символа.
    float u1, v1;    // Право-верх символа.
    int   width;     // Ширина символа.
    int   height;    // Высота символа.
    float offset_x;  // Смещение символа по ширине.
    float offset_y;  // Смещение символа по высоте.
    float advance;   // Насколько сдвигать курсор при отрисовке.
};


// Блок текста (размеры):
struct FontTextBlock {
    Vec2f start;  // Левый нижний угол прямоугольника.
    Vec2f end;    // Правый верхний угол прямоугольника.
    Vec2f size;   // Размер прямоугольника (ширина и высота).
    int   lines;  // Количество строк в блоке.
};


// -------- API шрифта: --------


// Создать растровый шрифт:
FontPixmap* FontPixmap_create(Renderer *renderer, const char *file_path, int font_size);

// Уничтожить растровый шрифт:
void FontPixmap_destroy(FontPixmap **font);

// Получить глиф (из кэша либо создать новый):
FontGlyph* FontPixmap_get_glyph(FontPixmap *self, uint32_t codepoint);

// Установить цвет текста:
void FontPixmap_set_color(FontPixmap *self, Vec4f color);

// Получить цвет текста:
Vec4f FontPixmap_get_color(FontPixmap *self);

// Установить фоновый цвет текста:
void FontPixmap_set_bg_color(FontPixmap *self, Vec4f bg_color);

// Получить фоновый цвет текста:
Vec4f FontPixmap_get_bg_color(FontPixmap *self);

// Установить отступ фона:
void FontPixmap_set_bg_padding(FontPixmap *self, Vec4f bg_padding);

// Получить отступ фона:
Vec4f FontPixmap_get_bg_padding(FontPixmap *self);

// Установить пикселизацию текста:
void FontPixmap_set_pixelized(FontPixmap *self, bool pixelized);

// Получить пикселизацию текста:
bool FontPixmap_get_pixelized(FontPixmap *self);

// Установить смещение межстрочного интервала:
void FontPixmap_set_line_height(FontPixmap *self, float line_height);

// Получить смещение межстрочного интервала:
float FontPixmap_get_line_height(FontPixmap *self);

// Установить межбуквенный интервал:
void FontPixmap_set_letter_spacing(FontPixmap *self, float letter_spacing);

// Получить межбуквенный интервал:
float FontPixmap_get_letter_spacing(FontPixmap *self);

// Установить размер табуляции:
void FontPixmap_set_tab_size(FontPixmap *self, int tab_size);

// Получить размер табуляции:
int FontPixmap_get_tab_size(FontPixmap *self);

// Установить ширину пробела:
void FontPixmap_set_space_advance(FontPixmap *self, float space_advance);

// Получить ширину пробела:
float FontPixmap_get_space_advance(FontPixmap *self);

// Получить блок текста:
FontTextBlock FontPixmap_get_text_block(FontPixmap *self, const char *text, ...);

// Отрисовать текст:
void FontPixmap_render(FontPixmap *self, float x, float y, float angle, const char *text, ...);
