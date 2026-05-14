//
// light2d_shader.h - Шейдеры 2D освещения.
//

#pragma once


static const char* LIGHT2D_SHADER_VERT = "\
#version 330 core\n\
layout (location = 0) in vec3 a_position;\n\
void main(void) {\n\
    gl_Position = vec4(a_position, 1.0f);\n\
}";

static const char* LIGHT2D_SHADER_FRAG = "\
#version 330 core\n\
uniform sampler2D u_albedo_texture;\n\
uniform sampler2D u_light_texture;\n\
uniform vec3      u_ambient;\n\
uniform float     u_intensity;\n\
uniform vec2      u_resolution;\n\
out vec4 FragColor;\n\
void main(void) {\n\
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;\n\
    vec4 albedo = texture(u_albedo_texture, uv);\n\
    vec3 light  = texture(u_light_texture, uv).rgb;\n\
    \n\
    // 2D lighting: final = albedo * (ambient + light * intensity):\n\
    vec3 lit = albedo.rgb * (u_ambient + light * max(u_intensity, 0.0));\n\
    FragColor = vec4(lit, albedo.a);\n\
}";
