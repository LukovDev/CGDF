//
// gbuffer_shader.h - Шейдеры GBuffer.
//

#pragma once


static const char* GBUFFER_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_view;\n\
uniform mat4 u_proj;\n\
uniform mat4 u_model;\n\
\n\
layout (location = 0) in vec3 a_position;\n\
layout (location = 1) in vec3 a_normal;\n\
layout (location = 2) in vec4 a_color;\n\
layout (location = 3) in vec2 a_texcoord;\n\
\n\
// Атрибуты инстансинга. Матрица 4х4 занимает 4 слота локаций подряд:\n\
layout (location = 4) in mat4 a_instance_model;\n\
\n\
out vec2 v_texcoord;\n\
out vec3 v_normal_world;\n\
out vec4 v_color;\n\
out vec3 v_pos_world;\n\
\n\
void main(void) {\n\
    v_texcoord = a_texcoord;\n\
    v_color = a_color;\n\
    mat4 model_matrix = u_model;\n\
    v_pos_world = (model_matrix * vec4(a_position, 1.0)).xyz;\n\
    v_normal_world = transpose(inverse(mat3(model_matrix))) * a_normal;\n\
    gl_Position = u_proj * u_view * vec4(v_pos_world, 1.0f);\n\
}";

static const char* GBUFFER_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform vec3 u_camera_pos;                 // Мировые координаты камеры.\n\
uniform float u_pom_min_layers = 16.0f;    // Минимальное количество слоёв параллакса [8, 16, 32].\n\
uniform float u_pom_max_layers = 128.0f;   // Максимальное количество слоёв параллакса [64, 128, 256].\n\
uniform bool u_pom_cutoff_enabled = true;  // Отсекать ли фрагменты параллакса за координатами текстуры.\n\
\n\
// Параметры материала:\n\
uniform vec4 u_albedo;\n\
uniform vec3 u_ambient;\n\
uniform float u_metallic;\n\
uniform float u_roughness;\n\
uniform float u_ao;\n\
uniform float u_normal_strength;\n\
uniform vec3 u_emissive_color;\n\
uniform float u_emissive_strength;\n\
uniform float u_height_strength;\n\
uniform float u_alpha_cutoff;\n\
uniform float u_distortion;\n\
uniform float u_distortion_aberration;\n\
\n\
// Какие текстуры материала используются:\n\
uniform bool u_use_tex_albedo;\n\
uniform bool u_use_tex_normal;\n\
uniform bool u_use_tex_occlusion;\n\
uniform bool u_use_tex_roughness;\n\
uniform bool u_use_tex_metallic;\n\
uniform bool u_use_tex_emissive;\n\
uniform bool u_use_tex_height;\n\
\n\
// Текстуры материала:\n\
uniform sampler2D u_tex_albedo;\n\
uniform sampler2D u_tex_normal;\n\
uniform sampler2D u_tex_occlusion;\n\
uniform sampler2D u_tex_roughness;\n\
uniform sampler2D u_tex_metallic;\n\
uniform sampler2D u_tex_emissive;\n\
uniform sampler2D u_tex_height;\n\
\n\
in vec2 v_texcoord;\n\
in vec3 v_normal_world;\n\
in vec4 v_color;\n\
in vec3 v_pos_world;\n\
\n\
layout (location = 0) out vec4 g_albedo_roughness;\n\
layout (location = 1) out vec4 g_normal_ao;\n\
layout (location = 2) out vec4 g_pbr_properties;\n\
layout (location = 3) out vec4 g_emissive;\n\
\n\
vec3 get_normal_from_map(vec3 N, vec2 coords, mat3 out_TBN, vec2 duv1, vec2 duv2) {\n\
    vec3 tangent_normal = textureGrad(u_tex_normal, coords, duv1, duv2).xyz * 2.0 - 1.0;\n\
    tangent_normal.xy *= u_normal_strength;\n\
    tangent_normal.z = sqrt(max(0.0, 1.0 - dot(tangent_normal.xy, tangent_normal.xy)));\n\
    return normalize(out_TBN * tangent_normal);\n\
}\n\
\n\
vec3 computeLighting(vec3 normal, vec3 baseColor, vec3 lightDir, vec3 lightColor, vec3 ambientColor) {\n\
    float diffuseFactor = max(dot(normalize(normal), normalize(-lightDir)), 0.0);\n\
    return baseColor * (ambientColor + diffuseFactor * lightColor);\n\
}\n\
void main(void) {\n\
    // Рассчет TBN:\n\
    vec3 N_base = normalize(v_normal_world);\n\
    vec3 dp1  = dFdx(v_pos_world);\n\
    vec3 dp2  = dFdy(v_pos_world);\n\
    vec2 duv1 = dFdx(v_texcoord);\n\
    vec2 duv2 = dFdy(v_texcoord);\n\
    float r = duv1.x * duv2.y - duv1.y * duv2.x;\n\
    float sign_det = (r >= 0.0) ? 1.0 : -1.0;\n\
    vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * sign_det;\n\
    vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * sign_det;\n\
    T = normalize(T - N_base * dot(T, N_base));\n\
    B = normalize(cross(N_base, T)); \n\
    mat3 TBN = mat3(T, B, N_base);\n\
    \n\
    vec2 texCoords = fract(v_texcoord);\n\
    \n\
    // Накладываем эффект POM (параллакс):\n\
    if (u_use_tex_height) {\n\
        vec3 view_dir = normalize(u_camera_pos - v_pos_world);\n\
        vec3 tangent_view_dir = normalize(transpose(TBN) * view_dir);\n\
        tangent_view_dir.y = -tangent_view_dir.y;\n\
        \n\
        float num_layers = mix(u_pom_max_layers, u_pom_min_layers, abs(dot(vec3(0, 0, 1), tangent_view_dir)));\n\
        float layer_depth = 1.0f / num_layers;\n\
        float current_layer_depth = 0.0f;\n\
        \n\
        vec2 P = tangent_view_dir.xy / max(tangent_view_dir.z, 0.00001) * u_height_strength;\n\
        vec2 deltaUVs = P / num_layers;\n\
        vec2 UVs = texCoords;\n\
        float current_depth_map_value = 1.0f - texture(u_tex_height, UVs).r;\n\
        \n\
        // Проходися по слоям пока не попадем по высоте:\n\
        while (current_layer_depth < current_depth_map_value) {\n\
            UVs -= deltaUVs;\n\
            current_depth_map_value = 1.0f - texture(u_tex_height, UVs).r;\n\
            current_layer_depth += layer_depth;\n\
        }\n\
        \n\
        // Применяем иллюзию:\n\
        vec2 prev_UVs = UVs + deltaUVs;\n\
        float afterDepth = current_depth_map_value - current_layer_depth;\n\
        float beforeDepth = 1.0f - texture(u_tex_height, prev_UVs).r - current_layer_depth + layer_depth;\n\
        float weight = 0.0;\n\
        if (abs(afterDepth - beforeDepth) > 0.0001) { weight = afterDepth / (afterDepth - beforeDepth); }\n\
        texCoords = prev_UVs * weight + UVs * (1.0f - weight);  // Интерполяция (сглаживание шагов).\n\
        \n\
        // Удаляем фрагменты за пределами координат:\n\
        if (texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0) {\n\
            if (u_pom_cutoff_enabled) discard;\n\
        }\n\
    }\n\
    \n\
    // 1. Albedo & Alpha cutoff:\n\
    vec4 albedo_tex = u_use_tex_albedo ? texture(u_tex_albedo, texCoords) : vec4(1.0);\n\
    vec4 final_albedo = u_albedo * albedo_tex * v_color;\n\
    \n\
    // Alpha cutoff (отсечение прозрачных пикселей):\n\
    if (final_albedo.a < u_alpha_cutoff) { discard; }\n\
    vec3 albedo = final_albedo.rgb;\n\
    \n\
    // 2. Rroughness & Mmetallic & AO:\n\
    float roughness = u_roughness;\n\
    if (u_use_tex_roughness) { roughness *= texture(u_tex_roughness, texCoords).r; }\n\
    \n\
    float metallic = u_metallic;\n\
    if (u_use_tex_metallic) { metallic *= texture(u_tex_metallic, texCoords).r; }\n\
    \n\
    float ao = u_ao;\n\
    if (u_use_tex_occlusion) { ao *= texture(u_tex_occlusion, texCoords).r; }\n\
    \n\
    // 3. Normal:\n\
    vec3 normal = normalize(v_normal_world);\n\
    if (u_use_tex_normal) { normal = get_normal_from_map(normal, texCoords, TBN, duv1, duv2); }\n\
    \n\
    // 4. Height:\n\
    float height = 0.0;\n\
    if (u_use_tex_height) { height = texture(u_tex_height, texCoords).r; }\n\
    \n\
    // 5. Emissive:\n\
    vec3 emissive = u_emissive_color * u_emissive_strength;\n\
    if (u_use_tex_emissive) { emissive *= texture(u_tex_emissive, texCoords).rgb; }\n\
    \n\
    // 6. Distortion:\n\
    float distortion = u_distortion;\n\
    float aberration = u_distortion_aberration;\n\
    albedo.rgb = computeLighting(normal, albedo.rgb, normalize(vec3(-1.0, -1.0, -1.0)), vec3(2.0), vec3(0.1));\n\
    \n\
    // Упаковываем выходные данные в GBuffer:\n\
    // Layout 0: Альбедо (RGB) + Шероховатость (A):\n\
    g_albedo_roughness = vec4(albedo, roughness);\n\
    \n\
    // Layout 1: Нормаль (RGB) + Окклюзия (A):\n\
    g_normal_ao = vec4(normal, ao);\n\
    \n\
    // Layout 2: Металл (R) + Высота (G) + Аберрация (B) + Искажение (A):\n\
    g_pbr_properties = vec4(metallic, height, aberration, distortion);\n\
    \n\
    // Layout 3: Свечение (RGB) + Зарезервировано (A):\n\
    g_emissive = vec4(emissive, 0.0);\n\
}";
