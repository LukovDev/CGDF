//
// objloader.h - Определяет функции загрузчика моделей в формате OBJ.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/array.h>
#include "renderer.h"


// Объявление структур:
typedef struct OBJFile OBJFile;  // Структура объектного файла.


// Структура объектного файла:
struct OBJFile {
    Array *models;     // Список моделей (Model*).
    Array *materials;  // Список материалов (Material*).
};


// Загрузить модели из OBJ-файла:
OBJFile ObjLoader_load(Renderer *renderer, const char *filepath);
