#version 330 core

uniform mat4 u_view;
uniform mat4 u_proj;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec4 a_color;

out vec2 v_texcoord;
out vec4 v_color;

void main(void) {
    gl_Position = u_proj * u_view * vec4(a_position, 1.0f);
    v_texcoord = a_texcoord;
    v_color = a_color;
}
