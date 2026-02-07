//
// animator.h - Определяет апи для работы с анимацией.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>


// Объявление структур:
typedef struct FrameAnimator2D FrameAnimator2D;  // 2D кадровый аниматор.


// Структура 2D кадрового аниматора:
struct FrameAnimator2D {
    uint32_t frames;    // Количество кадров анимации.
    float    duration;  // Продолжительность кадра в секундах.
    float    count;     // Счётчик кадров.
    bool     _paused_;  // Анимация приостановлена.
};


// -------- API 2D кадрового аниматора: --------


// Создать кадровый аниматор:
FrameAnimator2D* FrameAnimator2D_create(uint32_t frames, float duration);

// Уничтожить кадровый аниматор:
void FrameAnimator2D_destroy(FrameAnimator2D **animator);

// Обновить анимацию:
void FrameAnimator2D_update(FrameAnimator2D *self, float dtime);

// Запустить анимацию:
void FrameAnimator2D_start(FrameAnimator2D *self);

// Остановить анимацию и вернуть к первому кадру:
void FrameAnimator2D_stop(FrameAnimator2D *self);

// Остановить анимацию:
void FrameAnimator2D_pause(FrameAnimator2D *self);

// Возобновить анимацию:
void FrameAnimator2D_resume(FrameAnimator2D *self);

// Вернуть к первому кадру:
void FrameAnimator2D_reset(FrameAnimator2D *self);

// Получить активность анимации:
bool FrameAnimator2D_get_active(FrameAnimator2D *self);

// Получить кадр анимации:
int FrameAnimator2D_get_frame(FrameAnimator2D *self);
