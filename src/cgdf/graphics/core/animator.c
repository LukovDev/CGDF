//
// animator.c - Реализует код для создания анимации.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/mm.h>
#include "animator.h"


// -------- API 2D кадрового аниматора: --------


// Создать кадровый аниматор:
FrameAnimator2D* FrameAnimator2D_create(uint32_t frames, float duration) {
    FrameAnimator2D *animator = (FrameAnimator2D*)mm_alloc(sizeof(FrameAnimator2D));

    // Заполняем поля:
    animator->frames = frames;
    animator->duration = duration;
    animator->count = 0.0f;
    animator->_paused_ = false;
    return animator;
}

// Уничтожить кадровый аниматор:
void FrameAnimator2D_destroy(FrameAnimator2D **animator) {
    if (!animator || !*animator) return;
    mm_free(*animator);
    *animator = NULL;
}

// Обновить анимацию:
void FrameAnimator2D_update(FrameAnimator2D *self, float dtime) {
    if (!self) return;

    // Если анимация не на паузе:
    if (!self->_paused_) {
        self->count += 1.0f / self->duration * dtime;
    }

    // Если счётчик превысил количество кадров, обнуляем его:
    if ((int)self->count >= self->frames) {
        self->count = 0.0f;
    }
}

// Запустить анимацию:
void FrameAnimator2D_start(FrameAnimator2D *self) {
    if (!self) return;
    self->_paused_ = false;
}

// Остановить анимацию и вернуть к первому кадру:
void FrameAnimator2D_stop(FrameAnimator2D *self) {
    if (!self) return;
    self->_paused_ = true;
    self->count = 0.0f;
}

// Остановить анимацию:
void FrameAnimator2D_pause(FrameAnimator2D *self) {
    if (!self) return;
    self->_paused_ = true;
}

// Возобновить анимацию:
void FrameAnimator2D_resume(FrameAnimator2D *self) {
    if (!self) return;
    self->_paused_ = false;
}

// Вернуть к первому кадру:
void FrameAnimator2D_reset(FrameAnimator2D *self) {
    if (!self) return;
    self->count = 0.0f;
}

// Получить активность анимации:
bool FrameAnimator2D_get_active(FrameAnimator2D *self) {
    if (!self) return false;
    return !self->_paused_;
}

// Получить кадр анимации:
int FrameAnimator2D_get_frame(FrameAnimator2D *self) {
    if (!self) return 0;
    return (int)self->count;
}
