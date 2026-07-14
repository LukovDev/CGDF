//
// block_shader.h - Шейдер блока интерфейса.
//

#pragma once


static const char* GUI_BLOCK_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_model;\n\
uniform mat4 u_view;\n\
uniform mat4 u_proj;\n\
layout (location = 0) in vec3 a_position;\n\
layout (location = 1) in vec3 a_normal;\n\
layout (location = 2) in vec4 a_color;\n\
layout (location = 3) in vec2 a_texcoord;\n\
out vec2 v_texcoord;\n\
out vec3 v_normal;\n\
out vec3 v_normal_world;\n\
out vec4 v_color;\n\
\n\
void main(void) {\n\
    v_texcoord = a_texcoord;\n\
    v_normal = a_normal;\n\
    v_normal_world = transpose(inverse(mat3(u_model))) * a_normal;\n\
    v_color = a_color;\n\
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0f);\n\
}";

static const char* GUI_BLOCK_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform sampler2D u_texture;\n\
uniform vec2 u_resolution;      // Размер экрана в пикселях.\n\
uniform vec2 u_pos;             // Позиция левого угла блока (в пикселях экрана).\n\
uniform vec2 u_size;            // Размер блока в пикселях.\n\
uniform vec4 u_color;           // Цвет заливки.\n\
uniform vec4 u_corner_radius;   // Радиусы скругления: x=TL, y=TR, z=BR, w=BL\n\
uniform vec4 u_border_color;    // Цвет обводки.\n\
uniform float u_border_width;   // Ширина обводки.\n\
\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec3 v_normal_world;\n\
in vec4 v_color;\n\
out vec4 FragColor;\n\
\n\
float sdSquircleIndependent(vec2 p, vec2 b, vec4 r) {\n\
    // Выбираем радиус для текущей четверти экрана:\n\
    float radius = (p.x > 0.0) ? ((p.y > 0.0) ? r.y : r.z) : ((p.y > 0.0) ? r.x : r.w);\n\
    radius = clamp(radius, 0.0, min(b.x, b.y));\n\
    \n\
    // Вектор до жестких краев коробки:\n\
    vec2 edge_q = abs(p) - b;\n\
    \n\
    // Проверяем, находится ли пиксель строго в угловом секторе:\n\
    if (edge_q.x > -radius && edge_q.y > -radius && radius > 0.0) {\n\
        vec2 q = edge_q + radius;\n\
        return pow(pow(q.x, 4.0) + pow(q.y, 4.0), 0.25) - radius;\n\
    }\n\
    return max(edge_q.x, edge_q.y);\n\
}\n\
\n\
void main(void) {\n\
    vec2 fragpos = gl_FragCoord.xy;\n\
    vec2 center = u_pos + u_size * 0.5;\n\
    vec2 p = fragpos - center;\n\
    \n\
    // Внешняя форма блока:\n\
    float distance = sdSquircleIndependent(p, u_size * 0.5, u_corner_radius);\n\
    \n\
    if (distance > 1.0) discard;\n\
    \n\
    float edge_softness = 1.0; \n\
    float alpha_outer = smoothstep(edge_softness, -edge_softness, distance);\n\
    \n\
    // Внутренняя форма блока:\n\
    vec4 internal_radius = max(u_corner_radius - vec4(u_border_width), vec4(0.0));\n\
    float distance_inner = sdSquircleIndependent(p, (u_size * 0.5) - u_border_width, internal_radius);\n\
    \n\
    float alpha_inner = smoothstep(edge_softness, -edge_softness, distance_inner);\n\
    \n\
    // Расчет маски обводки:\n\
    float border_mask = alpha_outer - alpha_inner;\n\
    border_mask = clamp(border_mask, 0.0, 1.0);\n\
    \n\
    // Текстурирование и оригинальное смешивание:\n\
    vec2 screen_uv = gl_FragCoord.xy / u_resolution;\n\
    vec4 blur_color = mix(texture(u_texture, screen_uv), u_color, u_color.a);\n\
    vec4 finalcolor = mix(blur_color, u_border_color, border_mask * u_border_color.a);\n\
    \n\
    // Сглаживание:\n\
    finalcolor.a *= alpha_outer;\n\
    FragColor = finalcolor;\n\
}";
