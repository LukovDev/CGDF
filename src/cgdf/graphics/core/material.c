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
    if (!name) return NULL;
    Material *material = (Material*)mm_alloc(sizeof(Material));

    // Заполняем поля:
    material->name = mm_strdup(name ? name : "Default");
    material->albedo = albedo;
    material->ambient = ambient;
    material->metallic = metallic;
    material->roughness = roughness;
    material->ao = ao;
    material->normal_strength = normal_strength;
    material->emissive_color = emissive_color;
    material->emissive_strength = emissive_strength;
    material->height_strength = height_strength;
    material->height_min_layers = height_min_layers;
    material->height_max_layers = height_max_layers;
    material->height_cutoff_enabled = height_cutoff_enabled;
    material->alpha_cutoff = alpha_cutoff;
    material->double_sided = double_sided;
    material->transparent = transparent;
    material->distortion = distortion;
    material->distortion_aberration = distortion_aberration;

    material->albedo_map = albedo_map;
    material->normal_map = normal_map;
    material->occlusion_map = occlusion_map;
    material->roughness_map = roughness_map;
    material->metallic_map = metallic_map;
    material->emissive_map = emissive_map;
    material->height_map = height_map;

    // Владеем ли текстурами:
    material->owns_albedo_map = false;
    material->owns_normal_map = false;
    material->owns_occlusion_map = false;
    material->owns_roughness_map = false;
    material->owns_metallic_map = false;
    material->owns_emissive_map = false;
    material->owns_height_map = false;
    return material;
}

// Создать материал по умолчанию (пустой):
Material* Material_create_default(const char *name) {
    return Material_create(
        name, (Vec4f){1, 1, 1, 1}, (Vec3f){0, 0, 0}, 0.0f, 0.0f, 0.0f,
        0.0f, (Vec3f){0, 0, 0}, 0.0f, 0.0f, 16.0f, 128.0f, false, 0.0f,
        false, false, 0.0f, 0.0f, NULL, NULL, NULL, NULL, NULL, NULL, NULL
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
        (*material)->name = NULL;
    }

    mm_free(*material);
    *material = NULL;
}
