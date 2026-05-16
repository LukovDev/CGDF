//
// mesh.h - Определение общего функционала сетки.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "vertex.h"
#include "material.h"


// Объявление структур:
typedef struct Mesh Mesh;  // Структура сетки.

// (Определение структуры находится в реализации).


// -------- API сетки: --------


// Создать сетку:
Mesh* Mesh_create(
    const Vertex* vertices,
    uint32_t vertex_count,
    const uint32_t* indices,
    uint32_t index_count,
    bool is_dynamic,
    Material *material
);

// Уничтожить сетку:
void Mesh_destroy(Mesh **mesh);

// Получить айди буфера вершин и индексов (x=vbo, y=ebo):
Vec2i Mesh_get_buffers_ids(Mesh *self);

// Динамическая ли сетка:
bool Mesh_is_dynamic(Mesh *self);

// Получить материал из сетки:
Material* Mesh_get_material(Mesh *self);

// Получить размер сетки в байтах (VRAM. VBO+EBO):
size_t Mesh_get_size(Mesh *self);

// Простой способ отрисовать сетку через forward rendering:
void Mesh_render(Mesh *self, bool wireframe);
