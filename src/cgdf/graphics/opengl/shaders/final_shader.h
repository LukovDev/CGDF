//
// final_shader.h - Шейдеры финального прохода.
//

#pragma once


static const char* FINAL_SHADER_VERT = "\
#version 330 core\n\
\n\
layout (location = 0) in vec3 a_position;\n\
out vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    v_texcoord = a_position.xy * 0.5 + 0.5;\n\
    gl_Position = vec4(a_position.xy, 0.0, 1.0);\n\
}";

static const char* FINAL_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform sampler2D u_hdr;    // Картинка сцены в линейном цвете (HDR).\n\
uniform sampler2D u_depth;  // Глубина из G-Buffer.\n\
uniform float u_exposure;   // Экспозиция (общая яркость).\n\
uniform bool u_aces_tm;     // Используем ACES.\n\
in vec2 v_texcoord;\n\
out vec4 FragColor;\n\
\n\
// Тональная компрессия ACES (приближение Narkowicz): сжимает яркость > 1.0 в диапазон 0..1:\n\
vec3 tonemap_aces(vec3 x) {\n\
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;\n\
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);\n\
}\n\
\n\
// Перевод из линейного цвета в sRGB (для монитора):\n\
vec3 linear_to_srgb(vec3 c) {\n\
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));\n\
}\n\
\n\
void main(void) {\n\
    float depth = texture(u_depth, v_texcoord).r;\n\
    if (depth >= 1.0) discard;  // Пустой пиксель (нет геометрии): оставляем то, что уже на экране.\n\
    vec3 hdr = texture(u_hdr, v_texcoord).rgb * u_exposure;\n\
    if (u_aces_tm) hdr = tonemap_aces(hdr);\n\
    FragColor = vec4(linear_to_srgb(clamp(hdr, 0.0, 1.0)), 1.0);\n\
    gl_FragDepth = depth;\n\
}";
