//
// window.h - Общий апи для работы с окном приложения.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/pixmap.h>
#include "scene.h"
#include "renderer.h"
#include "input.h"


// Объявление структур:
typedef struct Window Window;        // Структура окна.
typedef struct WinConfig WinConfig;  // Конфигурация окна.
typedef struct WinVars WinVars;      // Внутренние переменные окна (определяется в реализации).


// Структура окна:
struct Window {
    WindowScene scene;      // Текущая сцена окна.
    WinConfig   *config;    // Конфигурация окна.
    WinVars     *vars;      // Указатель на переменные окна.
    Input       *input;     // Ввод.
    Renderer    *renderer;  // Рендерер.
};


// Конфигурация окна:
struct WinConfig {
    const char *title;  // Заголовок окна.
    Pixmap *icon;       // Иконка окна.
    bool vsync;         // Вертикальная синхронизация.
    int  fps;           // Количество кадров в секунду.
    bool visible;       // Видимость окна (скрыт/виден).
    bool titlebar;      // Видимость заголовка окна.
    bool resizable;     // Масштабируемость окна.
    bool fullscreen;    // Полноэкранный режим.
    bool always_top;    // Всегда на переднем плане.
    WindowScene scene;  // Сцена окна.

    // Версия рендерера:
    int gl_major;
    int gl_minor;

    union {
        int size[2];  // Размер окна.
        struct {
            int width;   // Ширина окна.
            int height;  // Высота окна.
        };
    };

    union {
        int position[2];  // Позиция окна.
        struct {
            int x;  // Позиция окна по ширине.
            int y;  // Позиция окна по высоте.
        };
    };

    union {
        int min_size[2];  // Минимальный размер окна.
        struct {
            int min_width;   // Минимальный размер окна по ширине.
            int min_height;  // Минимальный размер окна по высоте.
        };
    };

    union {
        int max_size[2];  // Максимальный размер окна.
        struct {
            int max_width;   // Максимальный размер окна по ширине.
            int max_height;  // Максимальный размер окна по высоте.
        };
    };
};


// Создать конфигурацию окна:
WinConfig* Window_create_config(WindowScene scene);

// Уничтожить конфигурацию окна:
void Window_destroy_config(WinConfig **config);

// Создать окно:
Window* Window_create(WinConfig *config);

// Уничтожить окно:
void Window_destroy(Window **window);


// -------- API окна: --------


// Вызовите для открытия окна:
bool Window_open(Window *self, bool renderer_debug);

// Вызовите для закрытия окна:
bool Window_close(Window *self);

// Вызовите для полного завершения работы всех окон:
bool Window_quit(Window *self);

// Установить заголовок окна:
void Window_set_title(Window *self, const char *title, ...);

// Получить заголовок окна:
const char* Window_get_title(Window *self);

// Установить иконку окна:
void Window_set_icon(Window *self, Pixmap *icon);

// Получить иконку окна:
Pixmap* Window_get_icon(Window *self);

// Установить размер окна:
void Window_set_size(Window *self, int width, int height);

// Получить размер окна:
void Window_get_size(Window *self, int *width, int *height);

// Установить ширину окна:
void Window_set_width(Window *self, int width);

// Получить ширину окна:
int Window_get_width(Window *self);

// Установить высоту окна:
void Window_set_height(Window *self, int height);

// Получить высоту окна:
int Window_get_height(Window *self);

// Получить центр окна:
void Window_get_center(Window *self, int *x, int *y);

// Установить позицию окна:
void Window_set_position(Window *self, int x, int y);

// Получить позицию окна:
void Window_get_position(Window *self, int *x, int *y);

// Установить вертикальную синхронизацию:
void Window_set_vsync(Window *self, bool vsync);

// Получить вертикальную синхронизацию:
bool Window_get_vsync(Window *self);

// Установить фпс окна:
void Window_set_fps(Window *self, int fps);

// Получить установленный фпс окна:
int Window_get_target_fps(Window *self);

// Установить видимость окна:
void Window_set_visible(Window *self, bool visible);

// Получить видимость окна:
bool Window_get_visible(Window *self);

// Установить видимость заголовка окна:
void Window_set_titlebar(Window *self, bool titlebar);

// Получить видимость заголовка окна:
bool Window_get_titlebar(Window *self);

// Установить масштабируемость окна:
void Window_set_resizable(Window *self, bool resizable);

// Получить масштабируемость окна:
bool Window_get_resizable(Window *self);

// Установить полноэкранный режим:
void Window_set_fullscreen(Window *self, bool fullscreen);

// Получить полноэкранный режим:
bool Window_get_fullscreen(Window *self);

// Установить минимальный размер окна:
void Window_set_min_size(Window *self, int width, int height);

// Получить минимальный размер окна:
void Window_get_min_size(Window *self, int *width, int *height);

// Установить максимальный размер окна:
void Window_set_max_size(Window *self, int width, int height);

// Получить максимальный размер окна:
void Window_get_max_size(Window *self, int *width, int *height);

// Установить всегда на переднем плане или нет:
void Window_set_always_top(Window *self, bool on_top);

// Получить всегда на переднем плане или нет:
bool Window_get_always_top(Window *self);

// Получить фокус окна:
bool Window_get_is_focused(Window *self);

// Получить расфокус окна:
bool Window_get_is_defocused(Window *self);

// Получить айди дисплея в котором это окно:
uint32_t Window_get_window_display_id(Window *self);

// Получить размер дисплея:
bool Window_get_display_size(Window *self, uint32_t id, int *width, int *height);

// Развернуть окно на весь экран:
void Window_maximize(Window *self);

// Свернуть окно в панель задач:
void Window_minimize(Window *self);

// Восстановить обычное состояние окна:
void Window_restore(Window *self);

// Перенести окно на передний план:
void Window_raise(Window *self);

// Получить текущий фпс:
float Window_get_current_fps(Window *self);

// Получить дельту времени:
double Window_get_dtime(Window *self);

// Получить время со старта окна:
double Window_get_time(Window *self);

// Установить сцену окна:
void Window_set_scene(Window *self, WindowScene scene);

// Получить сцену окна:
WindowScene Window_get_scene(Window *self);

// Очистить окно:
void Window_clear(Window *self, float r, float g, float b);

// Отрисовка содержимого окна:
void Window_display(Window *self);
