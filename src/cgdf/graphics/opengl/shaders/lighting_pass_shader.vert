#version 330 core

layout (location = 0) in vec3 a_position;
out vec2 v_texcoord;

void main(void) {
    v_texcoord = a_position.xy * 0.5 + 0.5;
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
}
