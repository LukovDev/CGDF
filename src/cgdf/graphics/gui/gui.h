//
// gui.h - Модуль интерфейса. Объединяет весь функционал модуля интерфейса.
//
// Является собственной реализацией графического интерфейса, наподобие ImGUI, но для CGDF.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// Подключаем:
#include <cgdf/core/std.h>
#include "../core/shader.h"
#include "../core/texture.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "../core/font.h"


// Глобальные переменные:
extern bool g_GUI_initialized;
extern Shader *g_GUI_block_shader;
extern Renderer *g_GUI_renderer;
extern Window *g_GUI_window;
extern FontPixmap *g_GUI_font;
extern Vec4f g_GUI_corner_radius;
extern Vec4f g_GUI_border_color;
extern float g_GUI_border_width;
extern bool g_GUI_is_begin;
extern Vec4f g_GUI_last_panel_rect;


// Инициализировать интерфейс:
bool GUI_init(Window *window, const char *font_path, int font_size);

// Уничтожить интерфейс:
void GUI_destroy(void);

// Установить скругление углов:
void GUI_set_corner_radius(Vec4f corner_radius);

// Установить цвет обводки:
void GUI_set_border_color(Vec4f border_color);

// Установить ширину обводки:
void GUI_set_border_width(float border_width);

// Начать рисовать интерфейс:
void GUI_begin(void);

// Отрисовать блок интерфейса:
bool GUI_render_block(float x, float y, float width, float height, Vec4f color, Texture *background);

// Закончить рисовать интерфейс:
void GUI_end(void);

// Отрисовать панель:
bool GUI_panel(float x, float y);

// Кнопка:
bool GUI_button(const char *text);


#ifdef __cplusplus
}
#endif
