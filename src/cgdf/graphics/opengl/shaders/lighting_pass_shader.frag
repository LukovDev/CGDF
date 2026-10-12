#version 330 core

#define CASCADES_MAX 16  // Должно совпадать с SHADOW_CASCADES_MAX в renderer.c

// Камера:
uniform vec3 u_camera_pos;      // Координаты камеры.
uniform vec3 u_camera_forward;  // Куда смотрит камера.
uniform bool u_camera_ortho;    // Ортографическая ли камера.
uniform float u_camera_near;    // Ближняя плоскость отсечения.
uniform mat4 u_inv_view_proj;   // Обратная матрица (проекция * вид): экран -> мир.

// Текстуры G-Buffer:
uniform sampler2D u_albedo_roughness;
uniform sampler2D u_normal_ao;
uniform sampler2D u_pbr;
uniform sampler2D u_emissive;
uniform sampler2D u_depth;

// Солнце (направленный свет):
uniform vec3  u_sun_dir;
uniform vec3  u_sun_color;
uniform float u_sun_intensity;

// Фоновый свет:
uniform vec3 u_ambient_color;
uniform float u_ambient_intensity;

// Тени:
uniform sampler2DShadow u_shadow_map;
uniform bool  u_shadows_enabled;
uniform bool  u_shadows_smooth;
uniform bool  u_shadows_debug;
uniform int   u_shadow_pcf_radius;
uniform float u_shadow_depth_bias;
// Каскады:
uniform int   u_cascade_count;
uniform float u_cascade_blend;
uniform ivec2 u_cascade_grid;
uniform mat4  u_cascade_matrices[CASCADES_MAX];
uniform float u_cascade_normal_offset[CASCADES_MAX];
uniform float u_cascade_splits[CASCADES_MAX + 1];

in vec2 v_texcoord;
out vec4 FragColor;

const float PI = 3.14159265359;

// Тень из одного каскада (1 - освещено, 0 - в тени):
float sample_cascade(int c, vec3 P, vec3 N, float NdotL, float d) {
    vec3 offset_pos = P + N * u_cascade_normal_offset[c] * mix(0.5, 1.0, 1.0 - NdotL);
    vec4 light_pos = u_cascade_matrices[c] * vec4(offset_pos, 1.0);
    vec3 coords = light_pos.xyz / light_pos.w * 0.5 + 0.5;
    vec2 tile = vec2(float(c % u_cascade_grid.x), float(c / u_cascade_grid.x));
    coords.xy = (coords.xy + tile) / vec2(u_cascade_grid);  // Координаты каскада -> координаты атласа.
    float ref = coords.z - u_shadow_depth_bias * (1.0 + d * d * u_camera_near);

    // textureLod вместо texture: без тонких линий на швах каскадов:
    if (u_shadows_smooth) {
        int r = u_shadow_pcf_radius;
        vec2 texel = 1.0 / vec2(textureSize(u_shadow_map, 0));
        float lit = 0.0;
        for (int x = -r; x <= r; x++) {
            for (int y = -r; y <= r; y++) {
                lit += textureLod(u_shadow_map, vec3(coords.xy + vec2(x, y) * texel, ref), 0.0);
            }
        }
        return lit / float((2 * r + 1) * (2 * r + 1));
    }
    return textureLod(u_shadow_map, vec3(coords.xy, ref), 0.0);
}

// Насколько точка освещена солнцем (cascade и blend - для режима отладки):
float calc_shadow(vec3 P, vec3 N, vec3 L, out int cascade, out float blend) {
    float NdotL = max(dot(N, L), 0.0);
    float d = dot(P - u_camera_pos, u_camera_forward);  // Расстояние вдоль взгляда камеры.
    int last = u_cascade_count - 1;
    cascade = -1;
    blend = 0.0;
    if (d >= u_cascade_splits[last + 1]) return 1.0;  // Дальше всех каскадов - тени нет.

    // Выбираем каскад по дальности:
    cascade = 0;
    for (int i = 0; i < last; i++) {
        if (d > u_cascade_splits[i + 1]) cascade = i + 1;
    }
    float shadow = sample_cascade(cascade, P, N, NdotL, d);

    // В конце каскада плавно переходим в следующий (последний плавно гаснет в "нет тени"):
    float near = u_cascade_splits[cascade];
    float far = u_cascade_splits[cascade + 1];
    blend = smoothstep(far - (far - near) * u_cascade_blend, far, d);
    if (blend > 0.0) {
        float next = (cascade < last) ? sample_cascade(cascade + 1, P, N, NdotL, d) : 1.0;
        shadow = mix(shadow, next, blend);
    }
    return shadow;
}

// Восстановить мировую позицию пикселя из его глубины:
vec3 reconstruct_world_pos(vec2 uv, float depth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);  // Координаты экрана -> NDC (-1..1).
    vec4 world = u_inv_view_proj * ndc;
    return world.xyz / world.w;
}

// D: распределение микро-граней (GGX/Trowbridge-Reitz):
float distribution_ggx(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// G: затенение микро-граней (Schlick-GGX), для одного направления:
float geometry_schlick_ggx(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// G для направления на камеру и на свет одновременно (Smith):
float geometry_smith(float NdotV, float NdotL, float roughness) {
    return geometry_schlick_ggx(NdotV, roughness) * geometry_schlick_ggx(NdotL, roughness);
}

// F: доля отражённого света в зависимости от угла (Френель, приближение Schlick):
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

void main(void) {
    float depth = texture(u_depth, v_texcoord).r;
    if (depth >= 1.0) { FragColor = vec4(0.0); return; }  // Пустой пиксель.

    // Читаем G-Buffer:
    vec4 ar = texture(u_albedo_roughness, v_texcoord);
    vec4 na = texture(u_normal_ao, v_texcoord);
    vec3 albedo = ar.rgb;
    float roughness = clamp(ar.a, 0.04, 1.0);  // При нуле блик становится бесконечно маленьким и мерцает.
    vec3 N = normalize(na.xyz);
    float ao = na.a;
    float metallic = texture(u_pbr, v_texcoord).r;
    vec3 emissive = texture(u_emissive, v_texcoord).rgb;

    vec3 P = reconstruct_world_pos(v_texcoord, depth);
    vec3 V = u_camera_ortho ? -u_camera_forward : normalize(u_camera_pos - P);

    // Базовая отражательная способность: у неметаллов ~4%, у металлов их цвет:
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Солнце:
    vec3 L = normalize(-u_sun_dir);  // Направление от поверхности на источник.
    vec3 H = normalize(V + L);       // Вектор посередине между камерой и светом.
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = distribution_ggx(NdotH, roughness);
    float G = geometry_smith(NdotV, NdotL, roughness);
    vec3  F = fresnel_schlick(HdotV, F0);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);  // Что не отразилось, рассеялось (у металлов рассеяния нет).

    vec3 radiance = u_sun_color * u_sun_intensity;
    int cascade = -1;
    float blend = 0.0;
    float shadow = u_shadows_enabled ? calc_shadow(P, N, L, cascade, blend) : 1.0;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * shadow;

    // Фоновый свет:
    vec3 ambient = u_ambient_color * u_ambient_intensity * albedo * ao;

    FragColor = vec4(ambient + Lo + emissive, 1.0);

    // Отладка каскадов:
    if (u_shadows_debug && cascade >= 0) {
        const vec3 tint[4] = vec3[4](
            vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0)
        );
        vec3 next = (cascade < u_cascade_count - 1) ? tint[(cascade + 1) % 4] : vec3(1.0);
        FragColor.rgb *= mix(tint[cascade % 4], next, blend);
    }
}
