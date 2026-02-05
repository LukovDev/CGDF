//
// draw.h - Создаёт общий апи для отрисовки примитивов.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "renderer.h"


// Объявление структур:
typedef struct SimpleDraw SimpleDraw;  // Простая отрисовка примитивов.


// Простая отрисовка примитивов:
// (Определение структуры находится в реализации).


// -------- API простой отрисовки примитивов: --------


// Создать простую отрисовку примитивов:
SimpleDraw* SimpleDraw_create(Renderer *renderer);

// Уничтожить простую отрисовку примитивов:
void SimpleDraw_destroy(SimpleDraw **draw);

// Нарисовать точку:
void SimpleDraw_point(SimpleDraw *draw, Vec4f color, Vec3f point, float size);

// Нарисовать точки:
void SimpleDraw_points(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float size);

// Нарисовать линию:
void SimpleDraw_line(SimpleDraw *draw, Vec4f color, Vec3f start, Vec3f end, float width);

// Нарисовать ломаную линию:
void SimpleDraw_line_strip(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float width);

// Нарисовать замкнутую ломаную линию:
void SimpleDraw_line_loop(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float width);

// Нарисовать треугольники:
void SimpleDraw_triangles(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count);

// Нарисовать треугольники с общей стороной:
void SimpleDraw_triangle_strip(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count);

// Нарисовать треугольники последняя вершина которой будет соединена с первой:
void SimpleDraw_triangle_fan(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count);

// Нарисовать квадрат:
void SimpleDraw_quad(SimpleDraw *draw, Vec4f color, Vec3f point, Vec2f size, float width);

// Нарисовать квадрат с заливкой:
void SimpleDraw_quad_fill(SimpleDraw *draw, Vec4f color, Vec3f point, Vec2f size);

// Нарисовать круг:
void SimpleDraw_circle(SimpleDraw *draw, Vec4f color, Vec3f center, float radius, uint32_t num_verts, float width);

// Нарисовать круг с заливкой:
void SimpleDraw_circle_fill(SimpleDraw *draw, Vec4f color, Vec3f center, float radius, uint32_t num_verts);

// Нарисовать звезду:
void SimpleDraw_star(
    SimpleDraw *draw, Vec4f color, Vec3f center, float outradius,
    float inradius, uint32_t num_verts, float width
);

// Нарисовать звезду с заливкой:
void SimpleDraw_star_fill(
    SimpleDraw *draw, Vec4f color, Vec3f center,
    float outradius, float inradius, uint32_t num_verts
);
