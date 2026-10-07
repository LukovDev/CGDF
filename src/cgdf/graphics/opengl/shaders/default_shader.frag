#version 330 core

uniform bool u_use_points = false;
uniform bool u_use_texture;
uniform bool u_use_normals;
uniform bool u_use_vcolor;
uniform vec4 u_color = vec4(1.0);
uniform sampler2D u_texture;
uniform bool u_use_gbuffer;
uniform int u_gbuffer_view;

in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_normal_world;
in vec4 v_color;
out vec4 FragColor;

void main(void) {
    if (u_use_gbuffer) {
        vec2 uv = vec2(v_texcoord.x, 1.0 - v_texcoord.y);
        vec4 s = texture(u_texture, uv);
        if (u_gbuffer_view == 0) {
            FragColor = vec4(s.rgba);
        } else if (u_gbuffer_view == 1) {
            FragColor = vec4(s.rgb / 2.0 + 0.5, 1.0);
        } else if (u_gbuffer_view == 2) {
            FragColor = vec4(s.rgb, 1.0);
        } else if (u_gbuffer_view == 3) {
            FragColor = vec4(s.rgb, 1.0);
        } else {
            float near = 0.01;
            float far = 50.0;
            FragColor = vec4(vec3((2.0 * near * far) / (far + near - s.r * (far - near)) / far), 1.0);
        }
        return;
    }

    // Если мы используем точки для рисования:
    if (u_use_points) {
        vec2 coord = gl_PointCoord*2.0f-1.0f;
        if (dot(coord, coord) > 1.0f) discard;  // Отбрасываем всё за пределами круга.
    }

    // Если мы используем текстуру, рисуем с ней, иначе только цвет:
    if (u_use_texture) {
        FragColor = u_color * texture(u_texture, v_texcoord);
    } else if (u_use_normals) {
        FragColor = vec4(normalize(v_normal.rgb), 1.0f);
    } else if (u_use_vcolor) {
        FragColor = v_color;
    } else {
        FragColor = u_color;
    }
}
