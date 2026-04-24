//
// model.c - Реализует модель. Модель содержит сетки, материалы, шейдеры, нод, и прочее.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include <cgdf/core/logger.h>
#include "renderer.h"
#include "mesh.h"
#include "model.h"


// Создать модель:
Model* Model_create(Renderer *renderer) {
    if (!renderer) {
        log_msg("[E] Model_create: Renderer is NULL.\n");
        return NULL;
    }

    Model *model = (Model*)mm_alloc(sizeof(Model));

    // Заполняем поля:
    model->renderer = renderer;
    glm_mat4_identity(model->transform);
    model->meshes = Array_create(sizeof(Mesh*), MODEL_DEFAULT_MESHES_COUNT);
    return model;
}

// Уничтожить модель:
void Model_destroy(Model **model) {
    if (!model || !*model) return;

    // Удаляем сетки модели:
    for (int i = 0; i < Array_len((*model)->meshes); i++) {
        Mesh *mesh = (Mesh*)Array_get_ptr((*model)->meshes, i);
        Mesh_destroy(&mesh);
    }
    Array_destroy(&(*model)->meshes);

    mm_free(*model);
    *model = NULL;
}

// Добавить сетку в модель:
void Model_add_mesh(Model *self, Mesh *mesh) {
    if (!self || !mesh) return;

    // Проходимся по сеткам и проверяем наличие сетки в модели (добавляем только уникальные):
    if (Array_find(self->meshes, &mesh)) return;
    Array_push(self->meshes, &mesh);
}

// Удалить сетку из модели:
void Model_remove_mesh(Model *self, Mesh *mesh) {
    if (!self || !mesh) return;

    // Ищем сетку в массиве сеток:
    size_t index = Array_find(self->meshes, &mesh);
    if (!index) return;  // index = 0. Сетки нет в массиве.

    // Удаляем по индексу - 1:
    Array_remove(self->meshes, index-1, NULL);
}

// Удалить и освободить память сетки из модели:
void Model_delete_mesh(Model *self, Mesh *mesh) {
    if (!self || !mesh) return;

    Model_remove_mesh(self, mesh);
    Mesh_destroy(&mesh);
}

// Отрисовать модель:
void Model_render(Model *self, bool wireframe) {
    if (!self) return;

    // Проходимся по сеткам и рисуем их:
    for (int i = 0; i < Array_len(self->meshes); i++) {
        Mesh *mesh = (Mesh*)Array_get_ptr(self->meshes, i);
        Mesh_render(mesh, wireframe);
    }
}
