#version 330 core

uniform mat4 u_view_proj;
uniform mat4 u_model;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_color;
layout (location = 3) in vec2 a_texcoord;

// Атрибуты инстансинга. Матрица 4х4 занимает 4 слота локаций подряд:
layout (location = 4) in mat4 a_instance_model;

out vec2 v_texcoord;
out vec3 v_normal_world;
out vec4 v_color;
out vec3 v_pos_world;

void main(void) {
    v_texcoord = a_texcoord;
    v_color = a_color;
    mat4 model_matrix = u_model;
    v_pos_world = (model_matrix * vec4(a_position, 1.0)).xyz;
    v_normal_world = transpose(inverse(mat3(model_matrix))) * a_normal;
    gl_Position = u_view_proj * vec4(v_pos_world, 1.0f);
}
