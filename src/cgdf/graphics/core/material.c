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
) {
    if (!name) return NULL;
    Material *material = (Material*)mm_alloc(sizeof(Material));

    // Заполняем поля:
    material->name = mm_strdup(name ? name : "Default");
    material->albedo = albedo;
    material->ambient = ambient;
    material->metallic = metallic;
    material->specular = specular;
    material->specular_strength = specular_strength;
    material->roughness = roughness;
    material->ao = ao;
    material->normal_strength = normal_strength;
    material->emissive_color = emissive_color;
    material->emissive_strength = emissive_strength;
    material->alpha_cutoff = alpha_cutoff;
    material->double_sided = double_sided;
    material->transparent = transparent;
    material->distortion_strength = distortion_strength;
    material->distortion_aberration_strength = distortion_aberration_strength;

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
        name, (Vec4f){1, 1, 1, 1}, (Vec3f){0, 0, 0}, 0.0f, (Vec3f){0, 0, 0},
        0.0f, 0.0f, 0.0f, 0.0f, (Vec3f){0, 0, 0}, 0.0f, 0.0f, false, false,
        0.0f, 0.0f, NULL, NULL, NULL, NULL, NULL, NULL, NULL
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
