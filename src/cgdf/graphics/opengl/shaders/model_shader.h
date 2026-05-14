//
// model_shader.h - Шейдеры модели.
//

#pragma once


static const char* MODEL_SHADER_VERT = "\
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

static const char* MODEL_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform vec4 u_color;\n\
uniform bool u_use_texture;\n\
uniform bool u_use_normals;\n\
uniform sampler2D u_texture;\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec3 v_normal_world;\n\
in vec4 v_color;\n\
in vec3 v_frag_pos;\n\
out vec4 FragColor;\n\
\n\
void main(void) {\n\
    vec4 color = u_color;\n\
    if (u_use_texture) {\n\
        color *= texture(u_texture, v_texcoord);\n\
    }\n\
    vec3 light_pos = vec3(100, 500, 100);\n\
    vec3 light_color = vec3(1, 1, 1);\n\
    float light_radius = 1.0f;\n\
    vec3 normal = normalize(v_normal_world);\n\
    vec3 light_dir = normalize(light_pos - v_frag_pos);\n\
    float distance = length(light_pos - v_frag_pos);\n\
    float diff = max(dot(normal, light_dir), 0.0);\n\
    vec3 diffuse = diff * light_color;\n\
    vec3 ambient = 0.1 * color.rgb;\n\
    vec3 result = (ambient + diffuse) * color.rgb;\n\
    if (distance <= light_radius) result = color.rgb;\n\
    FragColor = vec4(result, 1.0);\n\
    if (u_use_normals) FragColor = vec4(normalize(v_normal_world), 1.0);\n\
}";
