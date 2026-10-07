#version 330 core

uniform vec3 u_camera_pos;                 // Мировые координаты камеры.
uniform bool u_camera_ortho;               // Ортографическая ли камера.
uniform vec3 u_camera_forward;             // Куда смотрит камера.
uniform float u_pom_min_layers = 16.0f;    // Минимальное количество слоёв параллакса [8, 16, 32].
uniform float u_pom_max_layers = 128.0f;   // Максимальное количество слоёв параллакса [64, 128, 256].
uniform bool u_pom_cutoff_enabled = true;  // Отсекать ли фрагменты параллакса за координатами текстуры.
uniform float u_pom_low;                   // Притянуть уровень высоты к нулю (растяжение).
uniform float u_pom_high;                  // Притянуть уровень высоты к единице (растяжение).

// Параметры материала:
uniform vec4 u_albedo;
uniform vec3 u_ambient;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;
uniform float u_normal_strength;
uniform vec3 u_emissive_color;
uniform float u_emissive_strength;
uniform float u_height_strength;
uniform float u_alpha_cutoff;
uniform float u_distortion;
uniform float u_distortion_aberration;

// Какие текстуры материала используются:
uniform bool u_use_tex_albedo;
uniform bool u_use_tex_normal;
uniform bool u_use_tex_occlusion;
uniform bool u_use_tex_roughness;
uniform bool u_use_tex_metallic;
uniform bool u_use_tex_emissive;
uniform bool u_use_tex_height;

// Текстуры материала:
uniform sampler2D u_tex_albedo;
uniform sampler2D u_tex_normal;
uniform sampler2D u_tex_occlusion;
uniform sampler2D u_tex_roughness;
uniform sampler2D u_tex_metallic;
uniform sampler2D u_tex_emissive;
uniform sampler2D u_tex_height;

in vec2 v_texcoord;
in vec3 v_normal_world;
in vec4 v_color;
in vec3 v_pos_world;

layout (location = 0) out vec4 g_albedo_roughness;
layout (location = 1) out vec4 g_normal_ao;
layout (location = 2) out vec4 g_pbr_properties;
layout (location = 3) out vec4 g_emissive;

// Прочитать высоту с растяжением диапазона:
float sample_height(vec2 uv, vec2 dx, vec2 dy) {
    float h = textureGrad(u_tex_height, uv, dx, dy).r;
    return clamp((h - u_pom_low) / max(u_pom_high - u_pom_low, 0.0001), 0.0, 1.0);
}

vec3 get_normal_from_map(vec3 N, vec2 coords, mat3 out_TBN, vec2 duv1, vec2 duv2) {
    vec3 tangent_normal = textureGrad(u_tex_normal, coords, duv1, duv2).xyz * 2.0 - 1.0;
    tangent_normal.xy *= u_normal_strength;
    tangent_normal.z = sqrt(max(0.0, 1.0 - dot(tangent_normal.xy, tangent_normal.xy)));
    return normalize(out_TBN * tangent_normal);
}

void main(void) {
    // Расчёт TBN:
    vec3 N_base = normalize(v_normal_world);
    if (!gl_FrontFacing) N_base = -N_base;
    vec3 dp1  = dFdx(v_pos_world);
    vec3 dp2  = dFdy(v_pos_world);
    vec2 duv1 = dFdx(v_texcoord);
    vec2 duv2 = dFdy(v_texcoord);
    bool has_uv_basis = max(length(duv1), length(duv2)) > 1e-6;  // Чтобы не было артефактов на одинаковых UV.
    float r = duv1.x * duv2.y - duv1.y * duv2.x;
    float sign_det = (r >= 0.0) ? 1.0 : -1.0;
    vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * sign_det;
    vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * sign_det;

    // Касательный базис по производным (с учётом зеркальных UV):
    T = normalize(T - N_base * dot(T, N_base));
    float handedness = (dot(cross(N_base, T), B) < 0.0) ? 1.0 : -1.0;
    B = cross(N_base, T) * handedness;
    mat3 TBN = mat3(T, B, N_base);
    
    vec2 texCoords = v_texcoord;
    
    // Накладываем эффект POM (параллакс):
    if (u_use_tex_height && u_height_strength > 0.0f && has_uv_basis) {
        // Направление от поверхности на камеру. В ортографии лучи параллельны:
        vec3 view_dir = u_camera_ortho ? -u_camera_forward : normalize(u_camera_pos - v_pos_world);
        vec3 tangent_view_dir = normalize(transpose(TBN) * view_dir);
        if (!any(isnan(tangent_view_dir)) && !any(isinf(tangent_view_dir))) {
            tangent_view_dir.y = -tangent_view_dir.y;
            
            float num_layers = mix(u_pom_max_layers, u_pom_min_layers, abs(dot(vec3(0, 0, 1), tangent_view_dir)));
            float layer_depth = 1.0f / num_layers;
            float current_layer_depth = 0.0f;
            
            vec2 P = tangent_view_dir.xy / max(tangent_view_dir.z, 0.01) * u_height_strength;
            vec2 deltaUVs = P / num_layers;
            vec2 UVs = texCoords;
            float current_depth_map_value = 1.0f - sample_height(UVs, duv1, duv2);
            
            // Проходимся по слоям пока не попадем по высоте:
            int max_steps = int(u_pom_max_layers) + 1;  // Больше этого шагов при правильной работе не бывает.
            for (int steps = 0; current_layer_depth < current_depth_map_value && steps < max_steps; steps++) {
                current_layer_depth += layer_depth;
                UVs -= deltaUVs;
                current_depth_map_value = 1.0f - sample_height(UVs, duv1, duv2);
            }
            
            // Применяем иллюзию (бинарный поиск):
            vec2 uv_above = UVs + deltaUVs;
            vec2 uv_below = UVs;
            float depth_above = current_layer_depth - layer_depth;
            float depth_below = current_layer_depth;
            for (int k = 0; k < 6; k++) {  // 6 итераций достаточно чтобы была точность в 64 раза меньше расстояния между слоёв.
                vec2 uv_mid = (uv_above + uv_below) * 0.5;
                float depth_mid = (depth_above + depth_below) * 0.5;
                if (depth_mid < 1.0 - sample_height(uv_mid, duv1, duv2)) {
                    uv_above = uv_mid; depth_above = depth_mid;
                } else {
                    uv_below = uv_mid; depth_below = depth_mid;
                }
            }
            texCoords = (uv_above + uv_below) * 0.5;
            
            // Удаляем фрагменты за пределами координат:
            if (u_pom_cutoff_enabled) {
                vec2 tile = floor(v_texcoord);  // В каком тайле находится исходный пиксель.
                if (any(lessThan(texCoords, tile)) || any(greaterThan(texCoords, tile + 1.0))) { discard; }
            }
        }
    }
    
    // 1. Albedo & Alpha cutoff:
    vec4 albedo_tex = u_use_tex_albedo ? textureGrad(u_tex_albedo, texCoords, duv1, duv2) : vec4(1.0);
    vec4 final_albedo = u_albedo * albedo_tex * v_color;
    
    // Alpha cutoff (отсечение прозрачных пикселей):
    if (final_albedo.a < u_alpha_cutoff) { discard; }
    vec3 albedo = final_albedo.rgb;
    
    // 2. Roughness & Metallic & AO:
    float roughness = u_roughness;
    if (u_use_tex_roughness) { roughness *= textureGrad(u_tex_roughness, texCoords, duv1, duv2).r; }
    
    float metallic = u_metallic;
    if (u_use_tex_metallic) { metallic *= textureGrad(u_tex_metallic, texCoords, duv1, duv2).r; }
    
    float ao = u_ao;
    if (u_use_tex_occlusion) { ao *= textureGrad(u_tex_occlusion, texCoords, duv1, duv2).r; }
    
    // 3. Normal:
    vec3 normal = N_base;
    if (u_use_tex_normal && has_uv_basis) { normal = get_normal_from_map(normal, texCoords, TBN, duv1, duv2); }
    
    // 4. Height:
    float height = 0.0;
    if (u_use_tex_height) { height = sample_height(texCoords, duv1, duv2); }
    
    // 5. Emissive:
    vec3 emissive = u_emissive_color * u_emissive_strength;
    if (u_use_tex_emissive) { emissive *= textureGrad(u_tex_emissive, texCoords, duv1, duv2).rgb; }
    
    // 6. Distortion:
    float distortion = u_distortion;
    float aberration = u_distortion_aberration;
    
    // Упаковываем выходные данные в GBuffer:
    // Layout 0: Альбедо (RGB) + Шероховатость (A):
    g_albedo_roughness = vec4(albedo, roughness);
    
    // Layout 1: Нормаль (RGB) + Окклюзия (A):
    g_normal_ao = vec4(normal, ao);
    
    // Layout 2: Металл (R) + Высота (G) + Аберрация (B) + Искажение (A):
    g_pbr_properties = vec4(metallic, height, aberration, distortion);
    
    // Layout 3: Свечение (RGB) + Зарезервировано (A):
    g_emissive = vec4(emissive, 0.0);
}
