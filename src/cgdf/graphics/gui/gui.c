//
// gui.c - Реализация пользовательского интерфейса.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "../core/font.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../core/texture.h"
#include "../core/window.h"
#include "gui.h"


// Проверить пересечение с прямоугольником:
static bool _check_intersect_(int x, int y, int w, int h, int px, int py) {
    return (px >= x && px <= x + w) &&
           (py >= y && py <= y + h);
}


// Получить одно нажатие на кнопку:
bool GUI_button_get_down(GUIButton *button) {
    if (!button) return false;
    if (button->action && !button->_down_pressed_) {
        button->_down_pressed_ = true;
        return true;
    }
    return false;
}


// Обновить кнопку:
void GUI_button_update(Window *window, GUIButton *button) {
    if (!window || !button) return;

    // Получаем координаты мыши в окне:
    Vec2i mpos = Input_get_mouse_pos(window);
    mpos.y = Window_get_height(window) - mpos.y;

    // Получить нажатие левой кнопки мыши:
    bool mpressed = Input_get_mouse_pressed(window)[0];

    // Проверяем что мышь навелась на кнопку:
    if (_check_intersect_(button->x, button->y, button->width, button->height, mpos.x, mpos.y)) {
        button->hover = true;
    } else button->hover = false;

    // Проверяем нажатие мыши на кнопку:
    if (mpressed && button->hover) {
        button->action = true;
    } else {
        button->action = false;
        button->_down_pressed_ = false;
    }
}


// Отрисовать кнопку:
void GUI_button_render(Window *window, FontPixmap *font, GUIButton *button) {
    if (!window || !button) return;

    // По умолчанию цвет кнопки обычный:
    Vec4f color = button->color;

    // Если навелись на кнопку:
    if (button->hover) color = button->hover_color;

    // Если нажали на кнопку:
    if (button->action) color = button->action_color;

    // Обводка:
    int border = button->border_width;
    if (border > button->width / 2)  border = button->width / 2;
    if (border > button->height / 2) border = button->height / 2;

    // Рисуем обводку:
    if (border > 0) {
        Sprite2D_render(window->renderer, nullptr,
            button->x, button->y,
            button->width,
            button->height, 0.0f,
            button->border_color, false
        );
    }

    // Рисуем кнопку:
    Sprite2D_render(window->renderer, button->texture,
        button->x + border, button->y + border,
        button->width - border * 2,
        button->height - border * 2, 0.0f,
        color, false
    );

    // Центр кнопки:
    Vec2i center = {
        .x = button->x + button->width / 2,
        .y = button->y + button->height / 2
    };

    // Рисуем текст:
    if (font) {
        FontPixmap_set_color(font, button->text_color);
        FontPixmap_set_align(font, FONT_ALIGN_CENTER_CENTER);
        FontPixmap_set_scale_factor(font, (Vec2f){button->text_scale, button->text_scale});
        FontPixmap_render(font, center.x, center.y, 0.0f, "%s", button->text);
    }
}
