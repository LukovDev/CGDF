//
// mesh.h - Определение общего функционала сетки.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include "buffers/buffers.h"
#include "../vertex.h"


// Объявление структур:
typedef struct Mesh Mesh;      // Структура одной сетки модели.


// Структура сетки:
struct Mesh {
    BufferVAO *vao;  // Атрибуты вершин.
    BufferVBO *vbo;  // Буфер вершин.
    BufferEBO *ebo;  // Буфер индексов.
    uint32_t index_count;  // Количество индексов.
    bool is_dynamic;       // Динамическая ли сетка.
};


// -------- API сетки: --------


// Создать сетку:
Mesh* Mesh_create(
    const Vertex* vertices,
    uint32_t vertex_count,
    const uint32_t* indices,
    uint32_t index_count,
    bool is_dynamic
);

// Уничтожить сетку:
void Mesh_destroy(Mesh **mesh);

// Простой способ отрисовать сетку через forward rendering:
void Mesh_render(Mesh *self, bool wireframe);
