//
// scene.h - Определение функционала для работы со сценами.
//
// Сцена окна - Набор функций которые обрабатывает код окна.
// Это позволяет создавать разные игровые сцены и переключаться между ними.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>


// Объявление структур:
typedef struct Window Window;
typedef struct WindowScene WindowScene;  // Сцена окна.


// Сцена окна:
struct WindowScene {
    // Основные функции сцены (callbacks):
    void (*start)   (Window *self);  // Вызывается после создания окна.
    void (*update)  (Window *self, float dtime);  // Вызывается каждый кадр (цикл окна).
    void (*render)  (Window *self, float dtime);  // Вызывается каждый кадр (отрисовка окна).
    void (*resize)  (Window *self, int width, int height);  // Вызывается при изменении размера окна.
    void (*show)    (Window *self);  // Вызывается при разворачивании окна.
    void (*hide)    (Window *self);  // Вызывается при сворачивании окна.
    void (*destroy) (Window *self);  // Вызывается при закрытии окна.
};
