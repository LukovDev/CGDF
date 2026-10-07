//
// vertex.h - Просто объявление структуры вершины.
//

#pragma once


// Объявление структур:
typedef struct Vertex Vertex;  // Структура вершины.


// Структура вершины:
struct Vertex {
    float px, py, pz;  // Позиция вершины       (loc=0, vec3 a_position).
    float nx, ny, nz;  // Нормаль вершины       (loc=1, vec3 a_normal).
    float r, g, b, a;  // Цвет вершины          (loc=2, vec3 a_color).
    float u, v;        // Текстурные координаты (loc=3, vec2 a_texcoord).
};

// Вершина уходит в видеокарту как есть, поэтому её размер должен совпадать с атрибутами (12 float):
static_assert(sizeof(Vertex) == 12 * sizeof(float), "Vertex layout must match vertex attributes.");
