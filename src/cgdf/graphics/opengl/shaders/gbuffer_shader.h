//
// gbuffer_shader.h - Шейдеры GBuffer.
//

#pragma once


static const char* GBUFFER_SHADER_VERT = "\
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
\n\
void main(void) {\n\
    v_texcoord = a_texcoord;\n\
    v_normal = a_normal;\n\
    v_normal_world = transpose(inverse(mat3(u_model))) * a_normal;\n\
    v_color = a_color;\n\
    gl_Position = u_proj * u_view * u_model * vec4(a_position, 1.0f);\n\
}";

static const char* GBUFFER_SHADER_FRAG = "\
#version 330 core\n\
\n\
uniform vec4 u_albedo;\n\
uniform bool u_use_tex_albedo;\n\
uniform sampler2D u_tex_albedo;\n\
\n\
in vec2 v_texcoord;\n\
in vec3 v_normal;\n\
in vec3 v_normal_world;\n\
in vec4 v_color;\n\
layout (location = 0) out vec4 g_albedo_roughness;\n\
layout (location = 1) out vec4 g_normal_ao;\n\
layout (location = 2) out vec4 g_pbr_properties;\n\
layout (location = 3) out vec4 g_emissive_dist;\n\
\n\
void main(void) {\n\
    vec4 color = u_albedo;\n\
    if (u_use_tex_albedo) {\n\
        color *= texture(u_tex_albedo, v_texcoord);\n\
    }\n\
    \n\
    vec3 albedo = color.rgb;\n\
    float roughness = 1.0;\n\
    vec3 normal = normalize(v_normal_world);\n\
    float ao = 0.0;\n\
    float metallic = 0.0;\n\
    float height = 0.0;\n\
    float distortion = 0.0;\n\
    float distortion_aberration = 1.0;\n\
    vec3 emissive = vec3(0);\n\
    float reserved = 0.0;\n\
    \n\
    // Выводим данные в GBuffer:\n\
    g_albedo_roughness = vec4(albedo, roughness);\n\
    g_normal_ao = vec4(normal, ao);\n\
    g_pbr_properties = vec4(metallic, height, distortion, distortion_aberration);\n\
    g_emissive_dist = vec4(emissive, reserved);\n\
}";
