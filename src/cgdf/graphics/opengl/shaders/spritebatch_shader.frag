#version 330 core

uniform bool u_use_texture;
uniform sampler2D u_texture;

in vec2 v_texcoord;
in vec4 v_color;

out vec4 FragColor;

void main(void) {
    vec4 color = v_color;
    if (u_use_texture) {
        color *= texture(u_texture, v_texcoord);
    }
    FragColor = color;
}
