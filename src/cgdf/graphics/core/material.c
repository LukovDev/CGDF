//
// material.c - Реализация материала.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include "material.h"


// Создать материал:
Material* Material_create(
    const char *name,
    Vec4f albedo,
    Vec3f ambient,
    float metallic,
    float roughness,
    float ao,
    float normal_strength,
    Vec3f emissive_color,
    float emissive_strength,
    float height_strength,
    float height_min_layers,
    float height_max_layers,
    float height_low,
    float height_high,
    float height_base,
    float height_distance,
    bool height_cutoff_enabled,
    float alpha_cutoff,
    bool double_sided,
    bool transparent,
    float distortion,
    float distortion_aberration,
    Texture *albedo_map,
    Texture *normal_map,
    Texture *occlusion_map,
    Texture *roughness_map,
    Texture *metallic_map,
    Texture *emissive_map,
    Texture *height_map
) {
    Material *mat = (Material*)mm_alloc(sizeof(Material));

    // Заполняем поля:
    mat->name = mm_strdup(name ? name : "Default");
    mat->albedo = albedo;
    mat->ambient = ambient;
    mat->metallic = metallic;
    mat->roughness = roughness;
    mat->ao = ao;
    mat->normal_strength = normal_strength;
    mat->emissive_color = emissive_color;
    mat->emissive_strength = emissive_strength;
    mat->height_strength = height_strength;
    mat->height_min_layers = height_min_layers;
    mat->height_max_layers = height_max_layers;
    mat->height_low = height_low;
    mat->height_high = height_high;
    mat->height_base = height_base;
    mat->height_distance = height_distance;
    mat->height_cutoff_enabled = height_cutoff_enabled;
    mat->alpha_cutoff = alpha_cutoff;
    mat->double_sided = double_sided;
    mat->transparent = transparent;
    mat->distortion = distortion;
    mat->distortion_aberration = distortion_aberration;

    mat->albedo_map = albedo_map;
    mat->normal_map = normal_map;
    mat->occlusion_map = occlusion_map;
    mat->roughness_map = roughness_map;
    mat->metallic_map = metallic_map;
    mat->emissive_map = emissive_map;
    mat->height_map = height_map;

    // Владеем ли текстурами:
    mat->owns_albedo_map = false;
    mat->owns_normal_map = false;
    mat->owns_occlusion_map = false;
    mat->owns_roughness_map = false;
    mat->owns_metallic_map = false;
    mat->owns_emissive_map = false;
    mat->owns_height_map = false;
    return mat;
}

// Создать материал по умолчанию (пустой):
Material* Material_create_default(const char *name) {
    return Material_create(
        name, (Vec4f){1, 1, 1, 1}, (Vec3f){0, 0, 0}, 0.0f, 0.5f, 1.0f,
        1.0f, (Vec3f){0, 0, 0}, 0.0f, 0.0f, 4.0f, 64.0f, 0.0f, 1.0f, 0.5f, 10.0f, false, 0.0f,
        false, false, 0.0f, 0.0f, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    );
}

// Уничтожить материал:
void Material_destroy(Material **material) {
    if (!material || !*material) return;

    // Уничтожаем текстуры, если материал ими владеет:
    if ((*material)->owns_albedo_map)    Texture_destroy(&(*material)->albedo_map);
    if ((*material)->owns_normal_map)    Texture_destroy(&(*material)->normal_map);
    if ((*material)->owns_occlusion_map) Texture_destroy(&(*material)->occlusion_map);
    if ((*material)->owns_roughness_map) Texture_destroy(&(*material)->roughness_map);
    if ((*material)->owns_metallic_map)  Texture_destroy(&(*material)->metallic_map);
    if ((*material)->owns_emissive_map)  Texture_destroy(&(*material)->emissive_map);
    if ((*material)->owns_height_map)    Texture_destroy(&(*material)->height_map);

    // Освобождаем имя:
    if ((*material)->name) {
        mm_free((*material)->name);
        (*material)->name = nullptr;
    }

    mm_free(*material);
    *material = nullptr;
}
