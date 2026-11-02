//
// gui.h - Графический интерфейс.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "../core/font.h"
#include "../core/texture.h"
#include "../core/window.h"


// Объявление структур:
typedef struct GUIButton GUIButton;  // Структура кнопки.
typedef struct GUICheckBox GUICheckBox;


// Структура кнопки:
struct GUIButton {
    int x, y;            // Позиция кнопки.
    int width;           // Ширина кнопки.
    int height;          // Высота кнопки.
    int border_width;    // Ширина обводки.
    char *text;          // Текст кнопки.
    float text_scale;    // Масштабирование текста (по умолчанию 1.0).
    Texture *texture;    // Текстура кнопки.
    Vec4f text_color;    // Цвет текста.
    Vec4f color;         // Основной цвет кнопки.
    Vec4f border_color;  // Цвет обводки.
    Vec4f hover_color;   // Цвет при наведении мыши.
    Vec4f action_color;  // Цвет при нажатии на кнопку.
    bool hover;          // Наведена ли мышь на кнопку.
    bool action;         // Нажатие на кнопку.
    bool _down_pressed_;
};


// -------- API интерфейса: --------


// Получить одно нажатие на кнопку:
bool GUI_button_get_down(GUIButton *button);

// Обновить кнопку:
void GUI_button_update(Window *window, GUIButton *button);

// Отрисовать кнопку:
void GUI_button_render(Window *window, FontPixmap *font, GUIButton *button);
