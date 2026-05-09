//
// default_shader.h - Шейдер по умолчанию (затычка).
//

#pragma once


static const char* DEFAULT_SHADER_VERT = "\
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
out vec3 v_frag_pos;\n\
\n\
void main(void) {\n\
    v_texcoord = a_texcoord;\n\
    v_normal = a_normal;\n\
    v_normal_world = transpose(inverse(mat3(u_model))) * a_normal;\n\
    v_color = a_color;\n\
    v_frag_pos = vec3(u_model * vec4(a_position, 1.0f));\n\
    gl_Position = u_proj * u_view * vec4(v_frag_pos, 1.0f);\n\
}";

static const char* DEFAULT_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform bool u_use_points = false;\n\
uniform bool u_use_texture;\n\
uniform bool u_use_normals;\n\
uniform bool u_use_vcolor;\n\
uniform vec4 u_color = vec4(1.0);\n\
uniform sampler2D u_texture;\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec3 v_normal_world;\n\
in vec4 v_color;\n\
in vec3 v_frag_pos;\n\
out vec4 FragColor;\n\
\n\
void main(void) {\n\
    // Если мы используем точки для рисования:\n\
    if (u_use_points) {\n\
        vec2 coord = gl_PointCoord*2.0f-1.0f;\n\
        if (dot(coord, coord) > 1.0f) discard;  // Отбрасываем всё за пределами круга.\n\
    }\n\
    // Если мы используем текстуру, рисуем с ней, иначе только цвет:\n\
    if (u_use_texture) {\n\
        FragColor = u_color * texture(u_texture, v_texcoord);\n\
    } else if (u_use_normals) {\n\
        FragColor = vec4(normalize(v_normal.rgb), 1.0f);\n\
    } else if (u_use_vcolor) {\n\
        FragColor = v_color;\n\
    } else {\n\
        FragColor = u_color;\n\
    }\n\
}";
