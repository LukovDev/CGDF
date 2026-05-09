//
// spritebatch_shader.h - Шейдеры пакетной отрисовки спрайтов.
//

#pragma once


static const char* SPRITEBATCH_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_view;\n\
uniform mat4 u_proj;\n\
layout (location = 0) in vec3 a_position;\n\
layout (location = 1) in vec2 a_texcoord;\n\
layout (location = 2) in vec4 a_color;\n\
out vec2 v_texcoord;\n\
out vec4 v_color;\n\
\n\
void main() {\n\
    gl_Position = u_proj * u_view * vec4(a_position, 1.0f);\n\
    v_texcoord = a_texcoord;\n\
    v_color = a_color;\n\
}";

static const char* SPRITEBATCH_SHADER_FRAG = "\
#version 330 core\n\
uniform bool u_use_texture;\n\
uniform sampler2D u_texture;\n\
in vec2 v_texcoord;\n\
in vec4 v_color;\n\
out vec4 FragColor;\n\
\n\
void main() {\n\
    vec4 color = v_color;\n\
    if (u_use_texture) {\n\
        color *= texture(u_texture, v_texcoord);\n\
    }\n\
    FragColor = color;\n\
}";
