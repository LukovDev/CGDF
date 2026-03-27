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
#include "core/controllers/controllers.h"
#include "core/animator.h"
#include "core/camera.h"
#include "core/draw.h"
#include "core/font.h"
#include "core/input.h"
#include "core/light.h"
#include "core/mesh.h"
#include "core/renderer.h"
#include "core/scene.h"
#include "core/shader.h"
#include "core/sprite.h"
#include "core/spritebatch.h"
#include "core/texture.h"
#include "core/utils.h"
#include "core/vertex.h"
#include "core/window.h"

#ifdef __cplusplus
}
#endif
