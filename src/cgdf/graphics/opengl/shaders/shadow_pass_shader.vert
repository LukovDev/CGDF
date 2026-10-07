#version 330 core

uniform mat4 u_light_view_proj;
uniform mat4 u_model;

layout (location = 0) in vec3 a_position;

void main(void) {
    gl_Position = u_light_view_proj * u_model * vec4(a_position, 1.0);
}
