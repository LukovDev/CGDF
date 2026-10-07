#version 330 core

uniform sampler2D u_albedo_texture;
uniform sampler2D u_light_texture;
uniform vec3      u_ambient;
uniform float     u_intensity;
uniform vec2      u_resolution;

out vec4 FragColor;

void main(void) {
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    vec4 albedo = texture(u_albedo_texture, uv);
    vec3 light  = texture(u_light_texture, uv).rgb;
    
    // 2D lighting: final = albedo * (ambient + light * intensity):
    vec3 lit = albedo.rgb * (u_ambient + light * max(u_intensity, 0.0));
    FragColor = vec4(lit, albedo.a);
}
