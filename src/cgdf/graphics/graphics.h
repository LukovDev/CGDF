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
#include "animator.h"
#include "batch.h"
#include "camera.h"
#include "draw.h"
#include "input.h"
#include "mesh.h"
#include "renderer.h"
#include "scene.h"
#include "shader.h"
#include "sprite.h"
#include "texture.h"
#include "utils.h"
#include "vertex.h"
#include "window.h"


#ifdef __cplusplus
}
#endif
