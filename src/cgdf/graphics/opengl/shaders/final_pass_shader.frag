#version 330 core

uniform sampler2D u_hdr;    // Картинка сцены в линейном цвете (HDR).
uniform sampler2D u_depth;  // Глубина из G-Buffer.
uniform float u_exposure;   // Экспозиция (общая яркость).
uniform bool u_aces_tm;     // Используем ACES.

in vec2 v_texcoord;
out vec4 FragColor;

// Тональная компрессия ACES (приближение Narkowicz): сжимает яркость > 1.0 в диапазон 0..1:
vec3 tonemap_aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Перевод из линейного цвета в sRGB (для монитора):
vec3 linear_to_srgb(vec3 c) {
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

void main(void) {
    float depth = texture(u_depth, v_texcoord).r;
    if (depth >= 1.0) discard;  // Пустой пиксель (нет геометрии): оставляем то, что уже на экране.
    vec3 hdr = texture(u_hdr, v_texcoord).rgb * u_exposure;
    if (u_aces_tm) hdr = tonemap_aces(hdr);
    FragColor = vec4(linear_to_srgb(clamp(hdr, 0.0, 1.0)), 1.0);
    gl_FragDepth = depth;
}
