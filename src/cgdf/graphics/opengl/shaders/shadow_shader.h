//
// shadow_shader.h - Шейдеры теней.
//

#pragma once


static const char* SHADOW_SHADER_VERT = "\
#version 330 core\n\
\n\
uniform mat4 u_light_view_proj;\n\
uniform mat4 u_model;\n\
\n\
layout (location = 0) in vec3 a_position;\n\
\n\
void main(void) {\n\
    gl_Position = u_light_view_proj * u_model * vec4(a_position, 1.0);\n\
}";

static const char* SHADOW_SHADER_FRAG = "\
#version 330 core\n\
\n\
void main(void) {\n\
    // Буквально ничего не делаем. Глубина сцены записывается сама.\n\
}";
