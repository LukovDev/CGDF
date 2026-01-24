//
// input.c - Код для работы с вводом окна.
//
// Определения функций, зависящие от реализации окна,
// указываются в реализации кода окна.
// Информация ввода обновляется в коде окна.
// Этот код не зависит от реализации окна.
//


// Подключаем:
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include "window.h"
#include "input.h"


// Создать структуру мыши:
Input_MouseState* Input_MouseState_create(int max_keys) {
    Input_MouseState *ms = (Input_MouseState*)mm_calloc(1, sizeof(Input_MouseState));
    ms->max_keys = max_keys;
    ms->visible = true;
    ms->pressed = (bool*)mm_calloc(max_keys, sizeof(bool));
    ms->down    = (bool*)mm_calloc(max_keys, sizeof(bool));
    ms->up      = (bool*)mm_calloc(max_keys, sizeof(bool));
    if (!ms->pressed || !ms->down || !ms->up) {
        if (ms->pressed) mm_free(ms->pressed);
        if (ms->down) mm_free(ms->down);
        if (ms->up) mm_free(ms->up);
        mm_free(ms);
        mm_alloc_error();
    }
    return ms;
}

// Уничтожить структуру мыши:
void Input_MouseState_destroy(Input_MouseState **mouse_state) {
    if (!mouse_state || !*mouse_state) return;
    if ((*mouse_state)->pressed) mm_free((*mouse_state)->pressed);
    if ((*mouse_state)->down) mm_free((*mouse_state)->down);
    if ((*mouse_state)->up) mm_free((*mouse_state)->up);
    mm_free(*mouse_state);
    *mouse_state = NULL;
}


// Создать структуру клавиатуры:
Input_KeyboardState* Input_KeyboardState_create(int max_keys) {
    Input_KeyboardState *kb = (Input_KeyboardState*)mm_calloc(1, sizeof(Input_KeyboardState));
    kb->max_keys = max_keys;
    kb->pressed = (bool*)mm_calloc(max_keys, sizeof(bool));
    kb->down    = (bool*)mm_calloc(max_keys, sizeof(bool));
    kb->up      = (bool*)mm_calloc(max_keys, sizeof(bool));
    if (!kb->pressed || !kb->down || !kb->up) {
        if (kb->pressed) mm_free(kb->pressed);
        if (kb->down) mm_free(kb->down);
        if (kb->up) mm_free(kb->up);
        mm_free(kb);
        mm_alloc_error();
    }
    return kb;
}

// Уничтожить структуру клавиатуры:
void Input_KeyboardState_destroy(Input_KeyboardState **keyboard_state) {
    if (!keyboard_state || !*keyboard_state) return;
    if ((*keyboard_state)->pressed) mm_free((*keyboard_state)->pressed);
    if ((*keyboard_state)->down) mm_free((*keyboard_state)->down);
    if ((*keyboard_state)->up) mm_free((*keyboard_state)->up);
    mm_free(*keyboard_state);
    *keyboard_state = NULL;
}


// Создать структуру ввода:
Input* Input_create(
    void (*set_mouse_pos) (Window *self, int x, int y),
    void (*set_mouse_visible) (Window *self, bool visible)
) {
    Input *input = (Input*)mm_alloc(sizeof(Input));

    // Заполняем поля:
    input->mouse = Input_MouseState_create(8);  // Максимум 8 кнопок мыши.
    input->keyboard = Input_KeyboardState_create(INPUT_SCANCODE_COUNT);

    // Регистрируем функции:
    input->set_mouse_pos = set_mouse_pos;          // Функция зависит от окна.
    input->set_mouse_visible = set_mouse_visible;  // Функция зависит от окна.

    // Возвращаем:
    return input;
}

// Уничтожить структуру ввода:
void Input_destroy(Input **input) {
    if (!input || !*input) return;
    if ((*input)->mouse)    Input_MouseState_destroy(&(*input)->mouse);
    if ((*input)->keyboard) Input_KeyboardState_destroy(&(*input)->keyboard);
    mm_free(*input);
    *input = NULL;
}


// -------- Мышь: --------


// Получить нажатые кнопки мыши:
bool* Input_get_mouse_pressed(Window *self) {
    if (!self || !self->input || !self->input->mouse) return NULL;
    return self->input->mouse->pressed;
}

// Получить нажатие кнопки мыши:
bool* Input_get_mouse_down(Window *self) {
    if (!self || !self->input || !self->input->mouse) return NULL;
    return self->input->mouse->down;
}

// Получить отжатие кнопки мыши:
bool* Input_get_mouse_up(Window *self) {
    if (!self || !self->input || !self->input->mouse) return NULL;
    return self->input->mouse->up;
}

// Получить смещение мыши за кадр:
Vec2i Input_get_mouse_rel(Window *self) {
    if (!self || !self->input || !self->input->mouse) return (Vec2i){0, 0};
    return self->input->mouse->rel;
}

// Получить нахождение мыши над окном:
bool Input_get_mouse_focused(Window *self) {
    if (!self || !self->input || !self->input->mouse) return false;
    return self->input->mouse->focused;
}

// Получить вращение колёсика мыши:
Vec2i Input_get_mouse_wheel(Window *self) {
    if (!self || !self->input || !self->input->mouse) return (Vec2i){0, 0};
    return self->input->mouse->wheel;
}

// Установить позицию мыши:
void Input_set_mouse_pos(Window *self, int x, int y) {
    if (!self || !self->input || self->input->set_mouse_pos) return;

    // Вызываем функцию, из реализации окна:
    self->input->set_mouse_pos(self, x, y);
}

// Получить позицию мыши:
Vec2i Input_get_mouse_pos(Window *self) {
    if (!self || !self->input || !self->input->mouse) return (Vec2i){0, 0};
    return self->input->mouse->pos;
}

// Установить видимость мыши:
void Input_set_mouse_visible(Window *self, bool visible) {
    if (!self || !self->input || self->input->set_mouse_visible) return;

    // Вызываем функцию, из реализации окна:
    self->input->set_mouse_visible(self, visible);
}

// Получить видимость мыши:
bool Input_get_mouse_visible(Window *self) {
    if (!self || !self->input || !self->input->mouse) return false;
    return self->input->mouse->visible;
}


// -------- Клавиатура: --------


// Получить нажатые клавиши клавиатуры:
bool* Input_get_key_pressed(Window *self) {
    if (!self || !self->input || !self->input->keyboard) return NULL;
    return self->input->keyboard->pressed;
}

// Получить нажатие клавиши клавиатуры:
bool* Input_get_key_down(Window *self) {
    if (!self || !self->input || !self->input->keyboard) return NULL;
    return self->input->keyboard->down;
}

// Получить отжатие клавиши клавиатуры:
bool* Input_get_key_up(Window *self) {
    if (!self || !self->input || !self->input->keyboard) return NULL;
    return self->input->keyboard->up;
}
