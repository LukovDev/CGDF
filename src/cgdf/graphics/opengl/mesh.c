//
// mesh.c - Реализует работу с сетками модели.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../vertex.h"
#include "buffers/buffers.h"
#include "gl.h"
#include "mesh.h"


// Создать сетку:
Mesh* Mesh_create(
    const Vertex* vertices,
    uint32_t vertex_count,
    const uint32_t* indices,
    uint32_t index_count,
    bool is_dynamic
) {
    if (!vertices || !indices) return NULL;
    Mesh *mesh = (Mesh*)mm_alloc(sizeof(Mesh));

    int mode = is_dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    // Заполняем поля:
    mesh->vao = BufferVAO_create();
    mesh->vbo = BufferVBO_create(vertices, vertex_count * sizeof(Vertex), mode);
    mesh->ebo = BufferEBO_create(indices, index_count * sizeof(uint32_t), mode);
    mesh->index_count = index_count;
    mesh->is_dynamic = is_dynamic;

    // Настраиваем атрибуты:
    BufferVAO_begin(mesh->vao);
    // Привязываем буферы к VAO:
    BufferVBO_begin(mesh->vbo);
    BufferEBO_begin(mesh->ebo);
    // Позиция:
    BufferVAO_attrib_pointer(mesh->vao, 0, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, px));
    // Нормаль:
    BufferVAO_attrib_pointer(mesh->vao, 1, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, nx));
    // Цвет:
    BufferVAO_attrib_pointer(mesh->vao, 2, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, r));
    // Текстурные координаты:
    BufferVAO_attrib_pointer(mesh->vao, 3, 2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, u));
    // Готово, можем отвязать атрибуты:
    BufferVAO_end(mesh->vao);

    // Возвращаем сетку:
    return mesh;
}


// Уничтожить сетку:
void Mesh_destroy(Mesh **mesh) {
    if (!mesh || !*mesh) return;

    // Уничтожаем буферы:
    if ((*mesh)->vao) BufferVAO_destroy(&(*mesh)->vao);
    if ((*mesh)->vbo) BufferVBO_destroy(&(*mesh)->vbo);
    if ((*mesh)->ebo) BufferEBO_destroy(&(*mesh)->ebo);

    // Удаляем сетку:
    mm_free(*mesh);
    *mesh = NULL;
}


// -------- API сетки: --------


// Простой способ отрисовать сетку через forward rendering:
void Mesh_render(Mesh *self, bool wireframe) {
    if (!self) return;

    // Рисуем как есть, используя текущий активный шейдер:
    BufferVAO_begin(self->vao);
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, self->index_count, GL_UNSIGNED_INT, 0);
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    BufferVAO_end(self->vao);
}
