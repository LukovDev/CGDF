//
// graphics.h - Модуль графики. Объединяет весь функционал модуля графики.
//
// Реализация графики выбирается исходя из модуля, который вы компилируете. Обычно это "opengl".
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// Подключаем:
#include "controllers/controllers.h"
#include "camera.h"
#include "input.h"
#include "renderer.h"
#include "shader.h"
#include "texture.h"
#include "window.h"


#ifdef __cplusplus
}
#endif
