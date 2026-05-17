//
// model.h - Определяет модель. Модель содержит сетки, материалы, шейдеры, нод, и прочее.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include "renderer.h"
#include "mesh.h"


// Определения:
#define MODEL_DEFAULT_MESHES_COUNT 16  // Количество сеток по умолчанию.


// Объявление структур:
typedef struct Model Model;  // Модель.


// Структура модели:
struct Model {
    Renderer *renderer;  // Рендерер модели.
    mat4 transform;      // Матрица трансформации модели.
    Array *meshes;       // Сетки модели. Каждая сетка имеет свой материал.
    bool wireframe;      // Рисовать сетку как линии.
};


// -------- API модели: --------


// Создать модель:
Model* Model_create(Renderer *renderer);

// Уничтожить модель:
void Model_destroy(Model **model);

// Добавить сетку в модель:
void Model_add_mesh(Model *self, Mesh *mesh);

// Удалить сетку из модели:
void Model_remove_mesh(Model *self, Mesh *mesh);

// Удалить и освободить память сетки из модели:
void Model_delete_mesh(Model *self, Mesh *mesh);

// Отрисовать модель:
void Model_render(Model *self, bool wireframe);
