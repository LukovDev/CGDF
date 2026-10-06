//
// gbuffer_shader.h - Шейдеры GBuffer.
//

#pragma once


static const char* GBUFFER_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_view_proj;\n\
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
    gl_Position = u_view_proj * vec4(v_pos_world, 1.0f);\n\
}";

static const char* GBUFFER_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform vec3 u_camera_pos;                 // Мировые координаты камеры.\n\
uniform bool u_camera_ortho;               // Ортографическая ли камера.\n\
uniform vec3 u_camera_forward;             // Куда смотрит камера.\n\
uniform float u_pom_min_layers = 16.0f;    // Минимальное количество слоёв параллакса [8, 16, 32].\n\
uniform float u_pom_max_layers = 128.0f;   // Максимальное количество слоёв параллакса [64, 128, 256].\n\
uniform bool u_pom_cutoff_enabled = true;  // Отсекать ли фрагменты параллакса за координатами текстуры.\n\
uniform float u_pom_low;                   // Притянуть уровень высоты к нулю (растяжение).\n\
uniform float u_pom_high;                  // Притянуть уровень высоты к единице (растяжение).\n\
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
// Прочитать высоту с растяжением диапазона:\n\
float sample_height(vec2 uv, vec2 dx, vec2 dy) {\n\
    float h = textureGrad(u_tex_height, uv, dx, dy).r;\n\
    return clamp((h - u_pom_low) / max(u_pom_high - u_pom_low, 0.0001), 0.0, 1.0);\n\
}\n\
\n\
vec3 get_normal_from_map(vec3 N, vec2 coords, mat3 out_TBN, vec2 duv1, vec2 duv2) {\n\
    vec3 tangent_normal = textureGrad(u_tex_normal, coords, duv1, duv2).xyz * 2.0 - 1.0;\n\
    tangent_normal.xy *= u_normal_strength;\n\
    tangent_normal.z = sqrt(max(0.0, 1.0 - dot(tangent_normal.xy, tangent_normal.xy)));\n\
    return normalize(out_TBN * tangent_normal);\n\
}\n\
\n\
void main(void) {\n\
    // Рассчет TBN:\n\
    vec3 N_base = normalize(v_normal_world);\n\
    if (!gl_FrontFacing) N_base = -N_base;\n\
    vec3 dp1  = dFdx(v_pos_world);\n\
    vec3 dp2  = dFdy(v_pos_world);\n\
    vec2 duv1 = dFdx(v_texcoord);\n\
    vec2 duv2 = dFdy(v_texcoord);\n\
    bool has_uv_basis = max(length(duv1), length(duv2)) > 1e-6;  // Чтобы не было артефактов на одинаковых UV.\n\
    float r = duv1.x * duv2.y - duv1.y * duv2.x;\n\
    float sign_det = (r >= 0.0) ? 1.0 : -1.0;\n\
    vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * sign_det;\n\
    vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * sign_det;\n\
    // Касательный базис по производным (с учётом зеркальных UV):\n\
    T = normalize(T - N_base * dot(T, N_base));\n\
    float handedness = (dot(cross(N_base, T), B) < 0.0) ? 1.0 : -1.0;\n\
    B = cross(N_base, T) * handedness;\n\
    mat3 TBN = mat3(T, B, N_base);\n\
    \n\
    vec2 texCoords = v_texcoord;\n\
    \n\
    // Накладываем эффект POM (параллакс):\n\
    if (u_use_tex_height && u_height_strength > 0.0f && has_uv_basis) {\n\
        // Направление от поверхности на камеру. В ортографии лучи параллельны:\n\
        vec3 view_dir = u_camera_ortho ? -u_camera_forward : normalize(u_camera_pos - v_pos_world);\n\
        vec3 tangent_view_dir = normalize(transpose(TBN) * view_dir);\n\
        if (!any(isnan(tangent_view_dir)) && !any(isinf(tangent_view_dir))) {\n\
            tangent_view_dir.y = -tangent_view_dir.y;\n\
            \n\
            float num_layers = mix(u_pom_max_layers, u_pom_min_layers, abs(dot(vec3(0, 0, 1), tangent_view_dir)));\n\
            float layer_depth = 1.0f / num_layers;\n\
            float current_layer_depth = 0.0f;\n\
            \n\
            vec2 P = tangent_view_dir.xy / max(tangent_view_dir.z, 0.01) * u_height_strength;\n\
            vec2 deltaUVs = P / num_layers;\n\
            vec2 UVs = texCoords;\n\
            float current_depth_map_value = 1.0f - sample_height(UVs, duv1, duv2);\n\
            \n\
            // Проходися по слоям пока не попадем по высоте:\n\
            int max_steps = int(u_pom_max_layers) + 1;  // Больше этого шагов при правильной работе не бывает.\n\
            for (int steps = 0; current_layer_depth < current_depth_map_value && steps < max_steps; steps++) {\n\
                current_layer_depth += layer_depth;\n\
                UVs -= deltaUVs;\n\
                current_depth_map_value = 1.0f - sample_height(UVs, duv1, duv2);\n\
            }\n\
            \n\
            // Применяем иллюзию (бинарный поиск):\n\
            vec2 uv_above = UVs + deltaUVs;\n\
            vec2 uv_below = UVs;\n\
            float depth_above = current_layer_depth - layer_depth;\n\
            float depth_below = current_layer_depth;\n\
            for (int k = 0; k < 6; k++) {  // 6 итераций достаточно чтобы была точность в 64 раза меньше расстояния между слоёв.\n\
                vec2 uv_mid = (uv_above + uv_below) * 0.5;\n\
                float depth_mid = (depth_above + depth_below) * 0.5;\n\
                if (depth_mid < 1.0 - sample_height(uv_mid, duv1, duv2)) {\n\
                    uv_above = uv_mid; depth_above = depth_mid;\n\
                } else {\n\
                    uv_below = uv_mid; depth_below = depth_mid;\n\
                }\n\
            }\n\
            texCoords = (uv_above + uv_below) * 0.5;\n\
            \n\
            // Удаляем фрагменты за пределами координат:\n\
            if (u_pom_cutoff_enabled) {\n\
                vec2 tile = floor(v_texcoord);  // В каком тайле находится исходный пиксель.\n\
                if (any(lessThan(texCoords, tile)) || any(greaterThan(texCoords, tile + 1.0))) { discard; }\n\
            }\n\
        }\n\
    }\n\
    \n\
    // 1. Albedo & Alpha cutoff:\n\
    vec4 albedo_tex = u_use_tex_albedo ? textureGrad(u_tex_albedo, texCoords, duv1, duv2) : vec4(1.0);\n\
    vec4 final_albedo = u_albedo * albedo_tex * v_color;\n\
    \n\
    // Alpha cutoff (отсечение прозрачных пикселей):\n\
    if (final_albedo.a < u_alpha_cutoff) { discard; }\n\
    vec3 albedo = final_albedo.rgb;\n\
    \n\
    // 2. Rroughness & Mmetallic & AO:\n\
    float roughness = u_roughness;\n\
    if (u_use_tex_roughness) { roughness *= textureGrad(u_tex_roughness, texCoords, duv1, duv2).r; }\n\
    \n\
    float metallic = u_metallic;\n\
    if (u_use_tex_metallic) { metallic *= textureGrad(u_tex_metallic, texCoords, duv1, duv2).r; }\n\
    \n\
    float ao = u_ao;\n\
    if (u_use_tex_occlusion) { ao *= textureGrad(u_tex_occlusion, texCoords, duv1, duv2).r; }\n\
    \n\
    // 3. Normal:\n\
    vec3 normal = N_base;\n\
    if (u_use_tex_normal && has_uv_basis) { normal = get_normal_from_map(normal, texCoords, TBN, duv1, duv2); }\n\
    \n\
    // 4. Height:\n\
    float height = 0.0;\n\
    if (u_use_tex_height) { height = sample_height(texCoords, duv1, duv2); }\n\
    \n\
    // 5. Emissive:\n\
    vec3 emissive = u_emissive_color * u_emissive_strength;\n\
    if (u_use_tex_emissive) { emissive *= textureGrad(u_tex_emissive, texCoords, duv1, duv2).rgb; }\n\
    \n\
    // 6. Distortion:\n\
    float distortion = u_distortion;\n\
    float aberration = u_distortion_aberration;\n\
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
