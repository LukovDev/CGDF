//
// gui.c - Реализация интерфейса.
//


// Подключаем:
#include <cgdf/core/std.h>
#include "../core/shader.h"
#include "../core/texture.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "../core/font.h"
#include "../opengl/gl.h"
#include "block_shader.h"
#include "gui.h"


// Глобальные переменные:
bool g_GUI_initialized;
Shader *g_GUI_block_shader;
Renderer *g_GUI_renderer;
Window *g_GUI_window;
FontPixmap *g_GUI_font;
Vec4f g_GUI_corner_radius;
Vec4f g_GUI_border_color;
float g_GUI_border_width;
bool g_GUI_is_begin;
Vec4f g_GUI_last_panel_rect;


// Инициализировать интерфейс:
bool GUI_init(Window *window, const char *font_path, int font_size) {
    if (g_GUI_initialized) return false;
    else g_GUI_initialized = true;

    g_GUI_window = window;
    g_GUI_renderer = window->renderer;

    g_GUI_block_shader = Shader_create(g_GUI_renderer, GUI_BLOCK_SHADER_VERT, GUI_BLOCK_SHADER_FRAG, NULL);
    Shader_compile(g_GUI_block_shader);

    g_GUI_font = FontPixmap_create(g_GUI_renderer, font_path, font_size);
    g_GUI_is_begin = false;
    return true;
}


// Уничтожить интерфейс:
void GUI_destroy(void) {
    if (!g_GUI_initialized) return;
    g_GUI_initialized = false;
    Shader_destroy(&g_GUI_block_shader);
    FontPixmap_destroy(&g_GUI_font);
}


// Установить скругление углов:
void GUI_set_corner_radius(Vec4f corner_radius) {
    g_GUI_corner_radius = corner_radius;
    if (g_GUI_is_begin) {
        Shader_set_vec4(g_GUI_block_shader, "u_corner_radius", g_GUI_corner_radius);
    }
}


// Установить цвет обводки:
void GUI_set_border_color(Vec4f border_color) {
    g_GUI_border_color = border_color;
    if (g_GUI_is_begin) {
        Shader_set_vec4(g_GUI_block_shader, "u_border_color", g_GUI_border_color);
    }
}


// Установить ширину обводки:
void GUI_set_border_width(float border_width) {
    g_GUI_border_width = border_width;
    if (g_GUI_is_begin) {
        Shader_set_float(g_GUI_block_shader, "u_border_width", g_GUI_border_width);
    }
}


// Начать рисовать интерфейс:
void GUI_begin(void) {
    if (g_GUI_is_begin) return;
    else g_GUI_is_begin = true;
    Vec2f resolution = { Renderer_get_width(g_GUI_renderer), Renderer_get_height(g_GUI_renderer) };
    mat4 view, proj;
    Renderer_get_view_proj(g_GUI_renderer, view, proj);
    Shader_begin(g_GUI_block_shader);
    Shader_set_mat4(g_GUI_block_shader, "u_view", view);
    Shader_set_mat4(g_GUI_block_shader, "u_proj", proj);
    Shader_set_vec2(g_GUI_block_shader, "u_resolution", resolution);
    Shader_set_vec4(g_GUI_block_shader, "u_corner_radius", g_GUI_corner_radius);
    Shader_set_vec4(g_GUI_block_shader, "u_border_color", g_GUI_border_color);
    Shader_set_float(g_GUI_block_shader, "u_border_width", g_GUI_border_width);
}


// Отрисовать блок интерфейса:
bool GUI_render_block(float x, float y, float width, float height, Vec4f color, Texture *background) {
    mat4 model;
    glm_mat4_identity(model);
    glm_translate(model, (vec3){x+width*0.5f, y+height*0.5f, 0.0f});
    glm_scale(model, (vec3){width*0.5f, height*0.5f, 0.0f});
    Shader_set_mat4(g_GUI_block_shader, "u_model", model);
    Shader_set_vec2(g_GUI_block_shader, "u_pos", (Vec2f){x, y});
    Shader_set_vec2(g_GUI_block_shader, "u_size", (Vec2f){width, height});
    Shader_set_vec4(g_GUI_block_shader, "u_color", color);
    Shader_set_tex2d(g_GUI_block_shader, "u_texture", background != NULL ? background->id : 0);
    Mesh_render(g_GUI_renderer->sprite_mesh, false);
    return true;
}


// Закончить рисовать интерфейс:
void GUI_end(void) {
    if (!g_GUI_is_begin) return;
    else g_GUI_is_begin = false;
    Shader_end(g_GUI_block_shader);
}


// Отрисовать панель:
bool GUI_panel(float x, float y) {
    float win_width = Renderer_get_width(g_GUI_renderer);
    float win_height = Renderer_get_height(g_GUI_renderer);

    // Координаты мыши на экране:
    Vec2i mouse_pos = Input_get_mouse_pos(g_GUI_window);
    float mouse_x = mouse_pos.x;
    float mouse_y = mouse_pos.y;

    // Подготавливаем параметры:
    float width = 128.0f;
    float height = 256.0f;
    Vec2i margin = {8, 8};
    Vec4f corner_radius = {32.0f, 32.0f, 32.0f, 32.0f};
    Vec4f bg_color = {0.05, 0.075, 0.1, 1.0f};
    Vec4f border_color = {0.33, 0.33, 0.33, 0.8f};
    float border_width = 1.0f;

    static float sx = 16, sy = 16;

    bool inside = mouse_x >= sx && mouse_x <= sx+width && mouse_y >= sy && mouse_y <= sy+height;
    static bool pressed_inside = false;
    if (Input_get_mouse_down(g_GUI_window)[0] && inside) pressed_inside = true;
    if (Input_get_mouse_up(g_GUI_window)[0]) pressed_inside = false;
    if (pressed_inside) {
        Vec2i rel = Input_get_mouse_rel(g_GUI_window);
        sx += rel.x;
        sy += rel.y;
    }

    // Не даем блоку выйти за границы окна:
    sx = glm_clamp(sx, 0, win_width-width);
    sy = glm_clamp(sy, 0, win_height-height);

    x = sx;
    y = sy;
    g_GUI_last_panel_rect.x = sx;
    g_GUI_last_panel_rect.y = sy;
    g_GUI_last_panel_rect.z = width;
    g_GUI_last_panel_rect.w = height;

    float inside_x = x + margin.x;
    float inside_y = win_height - y - margin.y;

    float font_size = 16.0f;
    float target_size = font_size / g_GUI_font->font_size;

    // Рисуем фон панели:
    GUI_begin();
    GUI_set_border_color(border_color);
    GUI_set_border_width(border_width);
    GUI_set_corner_radius((Vec4f){24, 24, 24, 24});
    GUI_render_block(x, win_height - height - y, width, height, bg_color, NULL);
    GUI_set_corner_radius((Vec4f){24, 24, 0, 0});
    GUI_render_block(x, win_height - 32 - y, width, 32, bg_color, NULL);
    GUI_end();

    // Включаем тест отсечения и задаем область (размеры нашей панели):
    glEnable(GL_SCISSOR_TEST);
    glScissor((int)x, (int)(win_height - y - height), (int)width, (int)height);

    FontPixmap_set_color(g_GUI_font, (Vec4f){1, 1, 1, 1});
    FontPixmap_set_bg_color(g_GUI_font, (Vec4f){0, 0, 0, 0});
    FontPixmap_set_bg_padding(g_GUI_font, (Vec4f){0, 0, 0, 0});
    FontPixmap_set_scale_factor(g_GUI_font, (Vec2f){target_size, target_size});
    FontPixmap_set_pixelized(g_GUI_font, false);
    FontPixmap_set_align(g_GUI_font, FONT_ALIGN_TOP_LEFT);
    FontPixmap_set_tab_size(g_GUI_font, 4);
    FontPixmap_render(g_GUI_font, inside_x, inside_y, 0, "Panel coords:\n%.2f %.2f", x, y);

    // Выключаем тест отсечения, чтобы остальной интерфейс не обрезался:
    glDisable(GL_SCISSOR_TEST);
    return true;
}


// Кнопка:
bool GUI_button(const char *text) {
    float win_width = Renderer_get_width(g_GUI_renderer);
    float win_height = Renderer_get_height(g_GUI_renderer);

    // Координаты мыши на экране:
    Vec2i mouse_pos = Input_get_mouse_pos(g_GUI_window);
    float mouse_x = mouse_pos.x;
    float mouse_y = win_height - mouse_pos.y;

    // Подготавливаем параметры:
    float width = 64.0f;
    float height = 24.0f;
    Vec2i margin = {2, 2};
    Vec2i offset = {0, 64};
    Vec4f corner_radius = {24.0f, 24.0f, 24.0f, 24.0f};
    Vec4f color = {0.1, 0.125, 0.15, 1};
    Vec4f hover_color = {0.05, 0.075, 0.1, 1};
    Vec4f click_color = {0.01, 0.01, 0.01, 1};
    Vec4f border_color = {0.25, 0.25, 0.25, 0.95f};
    float border_width = 1.0f;

    float x = g_GUI_last_panel_rect.x + offset.x;
    float y = g_GUI_last_panel_rect.y + offset.y;

    float inside_x = x + g_GUI_last_panel_rect.z / 2.0f;
    float inside_y = win_height - y;

    bool inside = mouse_x >= inside_x-width/2.0f && mouse_x <= inside_x+width/2.0f && mouse_y >= inside_y-height/2.0f && mouse_y <= inside_y+height/2.0f;
    bool clicked = false;
    if (Input_get_mouse_up(g_GUI_window)[0] && inside) clicked = true;
    if (inside) {
        color.x = hover_color.x;
        color.y = hover_color.y;
        color.z = hover_color.z;
        color.w = hover_color.w;
    }
    if (clicked) {
        color.x = click_color.x;
        color.y = click_color.y;
        color.z = click_color.z;
        color.w = click_color.w;
    }

    float font_size = 16.0f;
    float target_size = font_size / g_GUI_font->font_size;

    // Рисуем фон панели:
    GUI_begin();
    GUI_set_border_color(border_color);
    GUI_set_border_width(border_width);
    GUI_set_corner_radius(corner_radius);
    GUI_render_block(inside_x-width/2.0f, inside_y-height/2.0f, width, height, color, NULL);
    GUI_end();

    // Включаем тест отсечения и задаем область (размеры нашей панели):
    glEnable(GL_SCISSOR_TEST);
    glScissor((int)(inside_x-width/2.0f), (int)(inside_y-height/2.0f), (int)width, (int)height);

    FontPixmap_set_color(g_GUI_font, (Vec4f){1, 1, 1, 1});
    FontPixmap_set_bg_color(g_GUI_font, (Vec4f){0, 0, 0, 0});
    FontPixmap_set_bg_padding(g_GUI_font, (Vec4f){0, 0, 0, 0});
    FontPixmap_set_scale_factor(g_GUI_font, (Vec2f){target_size, target_size});
    FontPixmap_set_pixelized(g_GUI_font, false);
    FontPixmap_set_align(g_GUI_font, FONT_ALIGN_CENTER_CENTER);
    FontPixmap_set_tab_size(g_GUI_font, 4);
    FontPixmap_render(g_GUI_font, inside_x, inside_y, 0, text);

    // Выключаем тест отсечения, чтобы остальной интерфейс не обрезался:
    glDisable(GL_SCISSOR_TEST);
    return clicked;
}
