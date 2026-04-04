//
// vector.h - Реализует базовую простую работу с векторами.
//

#pragma once


// Вектор двумерный целочисленный:
typedef struct Vec2i { int x, y; } Vec2i;
static inline int Vec2i_len(Vec2i v) { return sqrtf(v.x*v.x + v.y*v.y); }
static inline void Vec2i_sub(Vec2i *a, Vec2i b) { a->x -= b.x; a->y -= b.y; }
static inline void Vec2i_add(Vec2i *a, Vec2i b) { a->x += b.x; a->y += b.y; }
static inline void Vec2i_mul(Vec2i *a, Vec2i b) { a->x *= b.x; a->y *= b.y; }
static inline void Vec2i_div(Vec2i *a, Vec2i b) {
    if (b.x == 0 || b.y == 0) return;
    a->x /= b.x; a->y /= b.y;
}

// Вектор трехмерный целочисленный:
typedef struct Vec3i { int x, y, z; } Vec3i;
static inline int Vec3i_len(Vec3i v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
static inline void Vec3i_sub(Vec3i *a, Vec3i b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; }
static inline void Vec3i_add(Vec3i *a, Vec3i b) { a->x += b.x; a->y += b.y; a->z += b.z; }
static inline void Vec3i_mul(Vec3i *a, Vec3i b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; }
static inline void Vec3i_div(Vec3i *a, Vec3i b) {
    if(b.x == 0 || b.y == 0 || b.z == 0) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z;
}

// Вектор четырехмерный целочисленный:
typedef struct Vec4i { int x, y, z, w; } Vec4i;
static inline int Vec4i_len(Vec4i v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }
static inline void Vec4i_sub(Vec4i *a, Vec4i b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; a->w -= b.w; }
static inline void Vec4i_add(Vec4i *a, Vec4i b) { a->x += b.x; a->y += b.y; a->z += b.z; a->w += b.w; }
static inline void Vec4i_mul(Vec4i *a, Vec4i b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; a->w *= b.w; }
static inline void Vec4i_div(Vec4i *a, Vec4i b) {
    if(b.x == 0 || b.y == 0 || b.z == 0 || b.w == 0) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z; a->w /= b.w;
}

// Вектор двумерный вещественный:
typedef struct Vec2f { float x, y; } Vec2f;
static inline float Vec2f_len(Vec2f v) { return sqrtf(v.x*v.x + v.y*v.y); }
static inline void Vec2f_sub(Vec2f *a, Vec2f b) { a->x -= b.x; a->y -= b.y; }
static inline void Vec2f_add(Vec2f *a, Vec2f b) { a->x += b.x; a->y += b.y; }
static inline void Vec2f_mul(Vec2f *a, Vec2f b) { a->x *= b.x; a->y *= b.y; }
static inline void Vec2f_div(Vec2f *a, Vec2f b) {
    if(b.x == 0.0f || b.y == 0.0f) return;
    a->x /= b.x; a->y /= b.y;
}
static inline Vec2f Vec2f_norm(Vec2f v) {
    float l = Vec2f_len(v);
    if (l == 0.0f) return (Vec2f){0.0f, 0.0f};
    return (Vec2f){v.x / l, v.y / l};
}

// Вектор трехмерный вещественный:
typedef struct Vec3f { float x, y, z; } Vec3f;
static inline float Vec3f_len(Vec3f v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
static inline void Vec3f_sub(Vec3f *a, Vec3f b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; }
static inline void Vec3f_add(Vec3f *a, Vec3f b) { a->x += b.x; a->y += b.y; a->z += b.z; }
static inline void Vec3f_mul(Vec3f *a, Vec3f b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; }
static inline void Vec3f_div(Vec3f *a, Vec3f b) {
    if(b.x == 0.0f || b.y == 0.0f || b.z == 0.0f) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z;
}
static inline Vec3f Vec3f_norm(Vec3f v) {
    float l = Vec3f_len(v);
    if (l == 0.0f) return (Vec3f){0.0f, 0.0f, 0.0f};
    return (Vec3f){v.x / l, v.y / l, v.z / l};
}

// Вектор четырехмерный вещественный:
typedef struct Vec4f { float x, y, z, w; } Vec4f;
static inline float Vec4f_len(Vec4f v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }
static inline void Vec4f_sub(Vec4f *a, Vec4f b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; a->w -= b.w; }
static inline void Vec4f_add(Vec4f *a, Vec4f b) { a->x += b.x; a->y += b.y; a->z += b.z; a->w += b.w; }
static inline void Vec4f_mul(Vec4f *a, Vec4f b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; a->w *= b.w; }
static inline void Vec4f_div(Vec4f *a, Vec4f b) {
    if(b.x == 0.0f || b.y == 0.0f || b.z == 0.0f || b.w == 0.0f) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z; a->w /= b.w;
}
static inline Vec4f Vec4f_norm(Vec4f v) {
    float l = Vec4f_len(v);
    if (l == 0.0f) return (Vec4f){0.0f, 0.0f, 0.0f, 0.0f};
    return (Vec4f){v.x / l, v.y / l, v.z / l, v.w / l};
}

// Вектор двумерный вещественный с двойной точностью:
typedef struct Vec2d { double x, y; } Vec2d;
static inline double Vec2d_len(Vec2d v) { return sqrt(v.x*v.x + v.y*v.y); }
static inline void Vec2d_sub(Vec2d *a, Vec2d b) { a->x -= b.x; a->y -= b.y; }
static inline void Vec2d_add(Vec2d *a, Vec2d b) { a->x += b.x; a->y += b.y; }
static inline void Vec2d_mul(Vec2d *a, Vec2d b) { a->x *= b.x; a->y *= b.y; }
static inline void Vec2d_div(Vec2d *a, Vec2d b) {
    if(b.x == 0.0 || b.y == 0.0) return;
    a->x /= b.x; a->y /= b.y;
}
static inline Vec2d Vec2d_norm(Vec2d v) {
    double l = Vec2d_len(v);
    if (l == 0.0) return (Vec2d){0.0, 0.0};
    return (Vec2d){v.x / l, v.y / l};
}

// Вектор трехмерный вещественный с двойной точностью:
typedef struct Vec3d { double x, y, z; } Vec3d;
static inline double Vec3d_len(Vec3d v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
static inline void Vec3d_sub(Vec3d *a, Vec3d b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; }
static inline void Vec3d_add(Vec3d *a, Vec3d b) { a->x += b.x; a->y += b.y; a->z += b.z; }
static inline void Vec3d_mul(Vec3d *a, Vec3d b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; }
static inline void Vec3d_div(Vec3d *a, Vec3d b) {
    if(b.x == 0.0 || b.y == 0.0 || b.z == 0.0) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z;
}
static inline Vec3d Vec3d_norm(Vec3d v) {
    double l = Vec3d_len(v);
    if (l == 0.0) return (Vec3d){0.0, 0.0, 0.0};
    return (Vec3d){v.x / l, v.y / l, v.z / l};
}

// Вектор четырехмерный вещественный с двойной точностью:
typedef struct Vec4d { double x, y, z, w; } Vec4d;
static inline double Vec4d_len(Vec4d v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }
static inline void Vec4d_sub(Vec4d *a, Vec4d b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; a->w -= b.w; }
static inline void Vec4d_add(Vec4d *a, Vec4d b) { a->x += b.x; a->y += b.y; a->z += b.z; a->w += b.w; }
static inline void Vec4d_mul(Vec4d *a, Vec4d b) { a->x *= b.x; a->y *= b.y; a->z *= b.z; a->w *= b.w; }
static inline void Vec4d_div(Vec4d *a, Vec4d b) {
    if(b.x == 0.0 || b.y == 0.0 || b.z == 0.0 || b.w == 0.0) return;
    a->x /= b.x; a->y /= b.y; a->z /= b.z; a->w /= b.w;
}
static inline Vec4d Vec4d_norm(Vec4d v) {
    double l = Vec4d_len(v);
    if (l == 0.0) return (Vec4d){0.0, 0.0, 0.0, 0.0};
    return (Vec4d){v.x / l, v.y / l, v.z / l, v.w / l};
}
