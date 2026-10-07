//
// embeds_shaders.h - Встроенные шейдеры в код. Подключает файлы шейдеров при компиляции.
//

#pragma once


// -------- Шейдер по умолчанию: --------

static const unsigned char DEFAULT_SHD_VERT[] = {
    #embed "default_shader.vert" suffix(, '\0')
};

static const unsigned char DEFAULT_SHD_FRAG[] = {
    #embed "default_shader.frag" suffix(, '\0')
};

// -------- Шейдер GBuffer прохода: --------

static const unsigned char GBUFFER_SHD_VERT[] = {
    #embed "gbuffer_pass_shader.vert" suffix(, '\0')
};

static const unsigned char GBUFFER_SHD_FRAG[] = {
    #embed "gbuffer_pass_shader.frag" suffix(, '\0')
};

// -------- Шейдер прохода освещения: --------

static const unsigned char LIGHTING_SHD_VERT[] = {
    #embed "lighting_pass_shader.vert" suffix(, '\0')
};

static const unsigned char LIGHTING_SHD_FRAG[] = {
    #embed "lighting_pass_shader.frag" suffix(, '\0')
};

// -------- Шейдер прохода теней: --------

static const unsigned char SHADOW_SHD_VERT[] = {
    #embed "shadow_pass_shader.vert" suffix(, '\0')
};

static const unsigned char SHADOW_SHD_FRAG[] = {
    #embed "shadow_pass_shader.frag" suffix(, '\0')
};

// -------- Шейдер финального прохода: --------

static const unsigned char FINAL_SHD_VERT[] = {
    #embed "final_pass_shader.vert" suffix(, '\0')
};

static const unsigned char FINAL_SHD_FRAG[] = {
    #embed "final_pass_shader.frag" suffix(, '\0')
};

// -------- Шейдер 2D освещения: --------

static const unsigned char LIGHT2D_SHD_VERT[] = {
    #embed "light2d_shader.vert" suffix(, '\0')
};

static const unsigned char LIGHT2D_SHD_FRAG[] = {
    #embed "light2d_shader.frag" suffix(, '\0')
};

// -------- Шейдер пакетной отрисовки спрайтов: --------

static const unsigned char SPRITEBATCH_SHD_VERT[] = {
    #embed "spritebatch_shader.vert" suffix(, '\0')
};

static const unsigned char SPRITEBATCH_SHD_FRAG[] = {
    #embed "spritebatch_shader.frag" suffix(, '\0')
};
