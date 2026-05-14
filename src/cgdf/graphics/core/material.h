//
// material.h - Определяет структуру материала.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include "texture.h"


// Объявление структур:
typedef struct Material Material;  // Структура материала.


// Структура материала:
struct Material {
    char *name;  // Название материала.

    // Параметры:
    Vec4f albedo;               // Цвет материала RGBA.
    Vec3f ambient;              // Фоновый цвет.
    float metallic;             // Металлизация (0.0 = диэлектрик, 1.0 = метал).
    Vec3f specular;             // Цвет спекулярности (зеркальности).
    float specular_strength;    // Сила спекулярности (зеркальности).
    float roughness;            // Шероховатость (0.0 = зеркало, 1.0 = матовая поверхность).
    float ao;                   // Коэффициент внешней окклюзии. Обычно 1.
    float normal_strength;      // Интенсивность нормалей.
    Vec3f emissive_color;       // Цвет свечения.
    float emissive_strength;    // Интенсивность свечения.
    float alpha_cutoff;         // Альфа отсечение.
    bool double_sided;          // Двухсторонний материал (отключение culling).
    bool transparent;           // Прозрачный материал (для blend mode).
    float distortion_strength;             // Сила искажения.
    float distortion_aberration_strength;  // Сила искажения цвета.

    // Текстуры:
    Texture *albedo_map;        // Текстура цвета (RGB = цвет, A = альфа opacity).
    Texture *normal_map;        // Текстура нормалей материала.
    Texture *occlusion_map;     // Текстура внешней окклюзии (Ambient Occlusion).
    Texture *roughness_map;     // Текстура шероховатости (Roughness).
    Texture *metallic_map;      // Текстура металличности (Metallic).
    Texture *emissive_map;      // Текстура свечения.
    Texture *height_map;        // Текстура высоты (параллакс).

    // Владеет ли материал текстурами (true = автоматически удалить текстуру при удалении материала):
    bool owns_albedo_map;
    bool owns_normal_map;
    bool owns_occlusion_map;
    bool owns_roughness_map;
    bool owns_metallic_map;
    bool owns_emissive_map;
    bool owns_height_map;
};


// -------- API материала: --------


// Создать материал:
Material* Material_create(
    const char *name,
    Vec4f albedo,
    Vec3f ambient,
    float metallic,
    Vec3f specular,
    float specular_strength,
    float roughness,
    float ao,
    float normal_strength,
    Vec3f emissive_color,
    float emissive_strength,
    float alpha_cutoff,
    bool double_sided,
    bool transparent,
    float distortion_strength,
    float distortion_aberration_strength,
    Texture *albedo_map,
    Texture *normal_map,
    Texture *occlusion_map,
    Texture *roughness_map,
    Texture *metallic_map,
    Texture *emissive_map,
    Texture *height_map
);

// Создать материал по умолчанию (пустой):
Material* Material_create_default(const char *name);

// Уничтожить материал:
void Material_destroy(Material **material);
