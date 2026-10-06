//
// lightning_shader.h - Шейдеры прохода освещения.
//

#pragma once


static const char* LIGHTNING_SHADER_VERT = "\
#version 330 core\n\
\n\
layout (location = 0) in vec3 a_position;\n\
out vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    v_texcoord = a_position.xy * 0.5 + 0.5;\n\
    gl_Position = vec4(a_position.xy, 0.0, 1.0);\n\
}";

static const char* LIGHTNING_SHADER_FRAG = "\
#version 330 core\n\
\n\
// Камера:\n\
uniform vec3 u_camera_pos;\n\
uniform vec3 u_camera_forward;   // Куда смотрит камера.\n\
uniform bool u_camera_ortho;     // Ортографическая ли камера.\n\
uniform mat4 u_inv_view_proj;    // Обратная матрица (проекция * вид): экран -> мир.\n\
uniform mat4 u_light_view_proj;  // Матрица камеры солнца.\n\
\n\
// Текстуры G-Buffer:\n\
uniform sampler2D u_albedo_roughness;\n\
uniform sampler2D u_normal_ao;\n\
uniform sampler2D u_pbr;\n\
uniform sampler2D u_emissive;\n\
uniform sampler2D u_depth;\n\
\n\
// Солнце (направленный свет):\n\
uniform vec3 u_sun_dir;  // Направление, КУДА светит солнце (нормализованное).\n\
uniform vec3 u_sun_color;\n\
uniform float u_sun_intensity;\n\
\n\
// Фоновый свет:\n\
uniform vec3 u_ambient_color;\n\
uniform float u_ambient_intensity;\n\
\n\
// Тени:\n\
uniform bool u_shadows_enabled;\n\
uniform bool u_shadows_smooth;\n\
uniform sampler2DShadow u_shadow_map;  // Карта теней (с аппаратным сравнением).\n\
\n\
in vec2 v_texcoord;\n\
out vec4 FragColor;\n\
\n\
const float PI = 3.14159265359;\n\
\n\
// Восстановить мировую позицию пикселя из его глубины:\n\
vec3 reconstruct_world_pos(vec2 uv, float depth) {\n\
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);  // Координаты экрана -> NDC (-1..1).\n\
    vec4 world = u_inv_view_proj * ndc;\n\
    return world.xyz / world.w;\n\
}\n\
\n\
// D: распределение микро-граней (GGX/Trowbridge-Reitz):\n\
float distribution_ggx(float NdotH, float roughness) {\n\
    float a = roughness * roughness;\n\
    float a2 = a * a;\n\
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;\n\
    return a2 / (PI * d * d);\n\
}\n\
\n\
// G: затенение микро-граней (Schlick-GGX), для одного направления:\n\
float geometry_schlick_ggx(float NdotX, float roughness) {\n\
    float r = roughness + 1.0;\n\
    float k = (r * r) / 8.0;\n\
    return NdotX / (NdotX * (1.0 - k) + k);\n\
}\n\
\n\
// G для направления на камеру и на свет одновременно (Smith):\n\
float geometry_smith(float NdotV, float NdotL, float roughness) {\n\
    return geometry_schlick_ggx(NdotV, roughness) * geometry_schlick_ggx(NdotL, roughness);\n\
}\n\
\n\
// F: доля отражённого света в зависимости от угла (Френель, приближение Schlick):\n\
vec3 fresnel_schlick(float cos_theta, vec3 F0) {\n\
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);\n\
}\n\
\n\
// Насколько точка освещена солнцем:\n\
float calc_shadow(vec3 P, vec3 N, vec3 L) {\n\
    float NdotL = max(dot(N, L), 0.0);\n\
    vec3 offset_pos = P + N * 0.03 * (1.0 - NdotL);\n\
    \n\
    // Переводим точку в координаты камеры солнца и затем в 0..1 (координаты текстуры + глубина):\n\
    vec4 light_pos = u_light_view_proj * vec4(offset_pos, 1.0);\n\
    vec3 coords = light_pos.xyz / light_pos.w * 0.5 + 0.5;\n\
    if (coords.z > 1.0) return 1.0;  // Дальше дальней плоскости, тени нет.\n\
    \n\
    if (u_shadows_smooth) {\n\
        // PCF 3x3: 9 выборок, каждая из которых ещё и сама сглажена (4 пикселя). Мягкий край:\n\
        vec2 texel = 1.0 / vec2(textureSize(u_shadow_map, 0));\n\
        float lit = 0.0;\n\
        for (int x = -1; x <= 1; x++) {\n\
            for (int y = -1; y <= 1; y++) {\n\
                lit += texture(u_shadow_map, vec3(coords.xy + vec2(x, y) * texel, coords.z));\n\
            }\n\
        }\n\
        return lit / 9.0;\n\
    }\n\
    return texture(u_shadow_map, vec3(coords.xy, coords.z));\n\
}\n\
\n\
void main(void) {\n\
    float depth = texture(u_depth, v_texcoord).r;\n\
    if (depth >= 1.0) { FragColor = vec4(0.0); return; }  // Пустой пиксель.\n\
    \n\
    // Читаем G-Buffer:\n\
    vec4 ar = texture(u_albedo_roughness, v_texcoord);\n\
    vec4 na = texture(u_normal_ao, v_texcoord);\n\
    vec3 albedo = ar.rgb;\n\
    float roughness = clamp(ar.a, 0.04, 1.0);  // При нуле блик становится бесконечно маленьким и мерцает.\n\
    vec3 N = normalize(na.xyz);\n\
    float ao = na.a;\n\
    float metallic = texture(u_pbr, v_texcoord).r;\n\
    vec3 emissive = texture(u_emissive, v_texcoord).rgb;\n\
    \n\
    vec3 P = reconstruct_world_pos(v_texcoord, depth);\n\
    vec3 V = u_camera_ortho ? -u_camera_forward : normalize(u_camera_pos - P);\n\
    \n\
    // Базовая отражательная способность: у неметаллов ~4%, у металлов их цвет:\n\
    vec3 F0 = mix(vec3(0.04), albedo, metallic);\n\
    \n\
    // Солнце:\n\
    vec3 L = normalize(-u_sun_dir);  // Направление от поверхности на источник.\n\
    vec3 H = normalize(V + L);       // Вектор посередине между камерой и светом.\n\
    float NdotL = max(dot(N, L), 0.0);\n\
    float NdotV = max(dot(N, V), 0.0001);\n\
    float NdotH = max(dot(N, H), 0.0);\n\
    float HdotV = max(dot(H, V), 0.0);\n\
    \n\
    float D = distribution_ggx(NdotH, roughness);\n\
    float G = geometry_smith(NdotV, NdotL, roughness);\n\
    vec3  F = fresnel_schlick(HdotV, F0);\n\
    \n\
    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);\n\
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);  // Что не отразилось, рассеялось (у металлов рассеяния нет).\n\
    \n\
    vec3 radiance = u_sun_color * u_sun_intensity;\n\
    float shadow = u_shadows_enabled ? calc_shadow(P, N, L) : 1.0;\n\
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * shadow;\n\
    \n\
    // Фоновый свет:\n\
    vec3 ambient = u_ambient_color * u_ambient_intensity * albedo * ao;\n\
    \n\
    FragColor = vec4(ambient + Lo + emissive, 1.0);\n\
}";
