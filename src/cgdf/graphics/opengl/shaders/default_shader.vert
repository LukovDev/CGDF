#version 330 core

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_color;
layout (location = 3) in vec2 a_texcoord;

out vec2 v_texcoord;
out vec3 v_normal;
out vec3 v_normal_world;
out vec4 v_color;

void main(void) {
    v_texcoord = a_texcoord;
    v_normal = a_normal;
    v_normal_world = transpose(inverse(mat3(u_model))) * a_normal;
    v_color = a_color;
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0f);
}
