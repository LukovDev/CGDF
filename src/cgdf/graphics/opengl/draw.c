//
// draw.c - Реализация функций отрисовки примитивов для OpenGL.
//
// В этой реализации, не рекомендуется рисовать большие, и сложные примитивы (не оптимизированно).
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/mm.h>
#include "../shader.h"
#include "../renderer.h"
#include "../vertex.h"
#include "../draw.h"
#include "buffers/buffers.h"
#include "gl.h"


// Простая отрисовка примитивов:
struct SimpleDraw {
    Renderer  *renderer;     // Рендерер.
    BufferVAO *vao;          // Буфер атрибутов.
    BufferVBO *vbo;          // Буфер вершин.
    uint32_t  vertex_count;  // Сколько вершин записано в буфер последний раз.
};


// -------- Вспомогательные функции: --------


static Vertex* _get_verts_(Vec3f *points, uint32_t count) {
    Vertex *verts = (Vertex*)mm_alloc(count * sizeof(Vertex));
    for (uint32_t i = 0; i < count; i++) {
        verts[i] = (Vertex){points[i].x, points[i].y, points[i].z, 0, 0, 0, 1, 1, 1, 1, 0, 0};
    }
    return verts;  // Не забудьте освободить память!
}

static void _simpledraw_render_(SimpleDraw *draw, Vec4f color, Vertex *verts, uint32_t count, uint32_t mode) {
    // Используем дефолтный шейдер и буферы:
    mat4 model;
    glm_mat4_identity(model);
    Renderer *renderer = draw->renderer;
    Shader *shader = renderer->shader;
    Shader_begin(shader);
    BufferVAO_begin(draw->vao);
    BufferVBO_begin(draw->vbo);
    if (mode == GL_POINTS) Shader_set_bool(renderer->shader, "u_use_points", true);
    Shader_set_bool(renderer->shader, "u_use_texture", false);
    Shader_set_bool(renderer->shader, "u_use_normals", false);
    Shader_set_bool(renderer->shader, "u_use_vcolor", false);
    Shader_set_vec4(renderer->shader, "u_color", color);
    Shader_set_mat4(renderer->shader, "u_model", model);

    // Выделяем новую память в буфере, если число вершин больше прошлого:
    if (count > draw->vertex_count) {
        BufferVBO_set_data(draw->vbo, verts, count * sizeof(Vertex), GL_DYNAMIC_DRAW);
        draw->vertex_count = count;
    } else {
        // Иначе просто обновляем данные в буфере:
        BufferVBO_set_subdata(draw->vbo, verts, 0, count * sizeof(Vertex));
    }

    // Рисуем:
    glDrawArrays(mode, 0, count);

    // Возвращаем буферы:
    if (mode == GL_POINTS) Shader_set_bool(renderer->shader, "u_use_points", false);
    BufferVBO_end(draw->vbo);
    BufferVAO_end(draw->vao);
    Shader_end(shader);
}


// -------- API простой отрисовки примитивов: --------


// Создать простую отрисовку примитивов:
SimpleDraw* SimpleDraw_create(Renderer *renderer) {
    SimpleDraw *draw = (SimpleDraw*)mm_alloc(sizeof(SimpleDraw));

    // Заполняем поля:
    draw->renderer = renderer;
    draw->vao = BufferVAO_create();
    draw->vbo = BufferVBO_create(NULL, 0, GL_DYNAMIC_DRAW);
    draw->vertex_count = 0;

    // Настраиваем буфер:
    BufferVAO_begin(draw->vao);
    BufferVBO_begin(draw->vbo);
    BufferVAO_attrib_pointer(draw->vao, 0, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, px));
    BufferVAO_attrib_pointer(draw->vao, 1, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, nx));
    BufferVAO_attrib_pointer(draw->vao, 2, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, r));
    BufferVAO_attrib_pointer(draw->vao, 3, 2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, u));
    BufferVBO_end(draw->vbo);
    BufferVAO_end(draw->vao);
    return draw;
}

// Уничтожить простую отрисовку примитивов:
void SimpleDraw_destroy(SimpleDraw **draw) {
    if (!draw || !*draw) return;

    // Удаляем буферы:
    BufferVAO_destroy(&(*draw)->vao);
    BufferVBO_destroy(&(*draw)->vbo);

    mm_free(*draw);
    *draw = NULL;
}


// -------- Примитивы: --------


// Нарисовать точку:
void SimpleDraw_point(SimpleDraw *draw, Vec4f color, Vec3f point, float size) {
    if (!draw) return;
    glPointSize(size);
    _simpledraw_render_(draw, color, &(Vertex){.px=point.x, .py=point.y, .pz=point.z}, 1, GL_POINTS);
}

// Нарисовать точки:
void SimpleDraw_points(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float size) {
    if (!draw || !points) return;
    glPointSize(size);

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_POINTS);
    mm_free(verts);
}

// Нарисовать линию:
void SimpleDraw_line(SimpleDraw *draw, Vec4f color, Vec3f start, Vec3f end, float width) {
    if (!draw) return;
    glLineWidth(width);
    _simpledraw_render_(draw, color, (Vertex[]){
        {start.x, start.y, start.z, 0, 0, 0, 1, 1, 1, 1, 0, 0},
        {end.x,   end.y,   end.z,   0, 0, 0, 1, 1, 1, 1, 0, 0}
    }, 2, GL_LINES);
}

// Нарисовать ломаную линию:
void SimpleDraw_line_strip(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float width) {
    if (!draw || !points) return;
    glLineWidth(width);

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_LINE_STRIP);
    mm_free(verts);
}

// Нарисовать замкнутую ломаную линию:
void SimpleDraw_line_loop(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count, float width) {
    if (!draw || !points) return;
    glLineWidth(width);

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_LINE_LOOP);
    mm_free(verts);
}

// Нарисовать треугольники:
void SimpleDraw_triangles(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count) {
    if (!draw || !points) return;

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_TRIANGLES);
    mm_free(verts);
}

// Нарисовать треугольники с общей стороной:
void SimpleDraw_triangle_strip(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count) {
    if (!draw || !points) return;

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_TRIANGLE_STRIP);
    mm_free(verts);
}

// Нарисовать треугольники последняя вершина которой будет соединена с первой:
void SimpleDraw_triangle_fan(SimpleDraw *draw, Vec4f color, Vec3f *points, uint32_t count) {
    if (!draw || !points) return;

    // Создаём массив вершин:
    Vertex *verts = _get_verts_(points, count);
    _simpledraw_render_(draw, color, verts, count, GL_TRIANGLE_FAN);
    mm_free(verts);
}

// Нарисовать квадрат:
void SimpleDraw_quad(SimpleDraw *draw, Vec4f color, Vec3f point, Vec2f size, float width) {
    if (!draw) return;
    SimpleDraw_line_loop(draw, color, (Vec3f[]){
        {.x=point.x,        .y=point.y,        .z=point.z},
        {.x=point.x+size.x, .y=point.y,        .z=point.z},
        {.x=point.x+size.x, .y=point.y+size.y, .z=point.z},
        {.x=point.x,        .y=point.y+size.y, .z=point.z}
    }, 4, width);
}

// Нарисовать квадрат с заливкой:
void SimpleDraw_quad_fill(SimpleDraw *draw, Vec4f color, Vec3f point, Vec2f size) {
    if (!draw) return;
    SimpleDraw_triangles(draw, color, (Vec3f[]){
        {.x=point.x,        .y=point.y,        .z=point.z},  // 1.
        {.x=point.x+size.x, .y=point.y,        .z=point.z},  // 2.
        {.x=point.x+size.x, .y=point.y+size.y, .z=point.z},  // 3.
        {.x=point.x+size.x, .y=point.y+size.y, .z=point.z},  // 3.
        {.x=point.x,        .y=point.y+size.y, .z=point.z},  // 4.
        {.x=point.x,        .y=point.y,        .z=point.z}   // 1.
    }, 6);
}

// Нарисовать круг:
void SimpleDraw_circle(SimpleDraw *draw, Vec4f color, Vec3f center, float radius, uint32_t num_verts, float width) {
    if (!draw) return;
    if (num_verts < 3) num_verts = 3;

    // Создаём массив вершин:
    Vec3f *verts = (Vec3f*)mm_alloc(num_verts * sizeof(Vec3f));
    for (uint32_t i = 0; i < num_verts; i++) {
        float rad_angle = radians((360.0f/num_verts) * i);
        verts[i] = (Vec3f){center.x + cos(rad_angle) * radius, center.y + sin(rad_angle) * radius, center.z};
    }
    SimpleDraw_line_loop(draw, color, verts, num_verts, width);
    mm_free(verts);
}

// Нарисовать круг с заливкой:
void SimpleDraw_circle_fill(SimpleDraw *draw, Vec4f color, Vec3f center, float radius, uint32_t num_verts) {
    if (!draw) return;
    if (num_verts < 3) num_verts = 3;  // Вершины круга.
    uint32_t count = num_verts * 3;    // Всего вершин (в треугольниках).
    float angle_step = 2.0f * GLM_PIf / num_verts;

    // Создаём массив вершин:
    uint32_t base = 0;  // Шаг между вершинами.
    Vec3f *verts = (Vec3f*)mm_alloc(count * sizeof(Vec3f));
    for (uint32_t i = 0; i < num_verts; i++) {
        float theta1 = i * angle_step;
        float theta2 = (i + 1) * angle_step;
        verts[base++] = (Vec3f){center.x, center.y, center.z};
        verts[base++] = (Vec3f){center.x + cos(theta1) * radius, center.y + sin(theta1) * radius, center.z};
        verts[base++] = (Vec3f){center.x + cos(theta2) * radius, center.y + sin(theta2) * radius, center.z};
    }
    SimpleDraw_triangles(draw, color, verts, count);
    mm_free(verts);
}

// Нарисовать звезду:
void SimpleDraw_star(
    SimpleDraw *draw, Vec4f color, Vec3f center, float outradius,
    float inradius, uint32_t num_verts, float width
) {
    if (!draw) return;
    if (num_verts < 2) num_verts = 2;  // Концы звезды.

    // Создаём массив вершин:
    Vec3f *verts = (Vec3f*)mm_alloc(num_verts*2 * sizeof(Vec3f));
    for (uint32_t i = 0; i < num_verts*2; i++) {
        float radius = i % 2 ? inradius : outradius;
        float rad_angle = radians(i*180.0f/num_verts);
        verts[i] = (Vec3f){center.x + sin(rad_angle) * radius, center.y + cos(rad_angle) * radius, center.z};
    }
    SimpleDraw_line_loop(draw, color, verts, num_verts*2, width);
    mm_free(verts);
}

// Нарисовать звезду с заливкой:
void SimpleDraw_star_fill(
    SimpleDraw *draw, Vec4f color, Vec3f center,
    float outradius, float inradius, uint32_t num_verts
) {
    if (!draw) return;
    if (num_verts < 2) num_verts = 2;    // Концы звезды.
    uint32_t count = num_verts * 2 * 3;  // Всего вершин (в треугольниках).

    // Создаём массив вершин:
    uint32_t base = 0;  // Шаг между вершинами.
    Vec3f *verts = (Vec3f*)mm_alloc(count * sizeof(Vec3f));
    for (uint32_t i = 0; i < num_verts*2; i++) {
        float r1 = i % 2 ? inradius : outradius;
        float r2 = (i+1) % 2 ? inradius : outradius;
        float a1 = radians(i * 180.0 / num_verts);
        float a2 = radians((i+1) * 180.0 / num_verts);
        verts[base++] = (Vec3f){center.x, center.y, center.z};
        verts[base++] = (Vec3f){center.x + sinf(a1)*r1, center.y + cosf(a1)*r1, center.z};
        verts[base++] = (Vec3f){center.x + sinf(a2)*r2, center.y + cosf(a2)*r2, center.z};
    }
    SimpleDraw_triangles(draw, color, verts, count);
    mm_free(verts);
}
