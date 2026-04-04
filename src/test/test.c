//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>
#include <cgdf/graphics/opengl/texunit.h>
#include "game.h"


static Texture *tex1;
static CameraController3D *ctrl3d;
static CameraOrbitController3D *ctrl_orbit;
static Camera3D *camera3d;
static SpriteBatch *batch;
static SimpleDraw *draw;
static Shader *grid;
static Shader *atmosphere;
FontPixmap *font;


static void print_before_free() {
    log_msg("[I] (Before free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
}


static void print_after_free() {
    log_msg("[I] (After free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
    if (mm_get_used_size() > 0) log_msg("[W] Memory leak!\n");
}


static void load_shader(Shader *shader, const char *vert, const char *frag) {
    char *grid_vert = Files_load(vert, "r");
    char *grid_frag = Files_load(frag, "r");
    shader->vertex = grid_vert;
    shader->fragment = grid_frag;
    Shader_compile(shader);
    mm_free(grid_vert);
    mm_free(grid_frag);
}


// Вызывается после создания окна:
void start(Window *self) {
    printf("Start called.\n");
    Window_set_fps(self, 0);
    Window_set_vsync(self, false);

    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Window_set_title(self, "CGDF Window");
    Pixmap_destroy(&icon);

    camera3d = Camera3D_create(
        self, Window_get_width(self), Window_get_height(self),
        (Vec3d){0.0f, 0.0f, 10.0f},
        (Vec3d){0.0f, 0.0f, 0.0f},
        (Vec3d){1.0f, 1.0f, 1.0f},
        90.0f,
        0.01f, 10000.0f,
        false
    );
    Camera3D_set_cull_faces(camera3d, false);
    Camera3D_set_depth_test(camera3d, false);
    ctrl3d = CameraController3D_create(self, camera3d, 0.1f, 1.0f, 5.0f, 25.0f, 0.75f, false);
    ctrl_orbit = CameraOrbitController3D_create(self, camera3d, (Vec3d){0.0f, 0.0f, 0.0f}, 0.1f, 5.0f, 0.75f, true);

    printf("Loading data...\n");

    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/logo/CGDF2x2.png", true);
    // Texture_set_pixelized(tex1);

    batch = SpriteBatch_create(self->renderer);

    draw = SimpleDraw_create(self->renderer);

    grid = Shader_create(self->renderer, NULL, NULL, NULL);
    atmosphere = Shader_create(self->renderer, NULL, NULL, NULL);
    load_shader(grid, "data/shaders/grid.vert", "data/shaders/grid.frag");
    load_shader(atmosphere, "data/shaders/atmosphere.vert", "data/shaders/atmosphere.frag");

    font = FontPixmap_create(self->renderer, "data/fonts/pixel.ttf", 32);
    FontPixmap_set_pixelized(font, true);

    printf("data loaded\n");
}


// Вызывается при закрытии окна:
void destroy(Window *self) {
    printf("Destroy called.\n");
    print_before_free();
    Texture_destroy(&tex1);
    SpriteBatch_destroy(&batch);
    SimpleDraw_destroy(&draw);
    Shader_destroy(&grid);
    Shader_destroy(&atmosphere);
    FontPixmap_destroy(&font);
    Camera3D_destroy(&camera3d);
    CameraController3D_destroy(&ctrl3d);
    CameraOrbitController3D_destroy(&ctrl_orbit);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, float dtime) {
    if (Input_get_key_up(self)[K_r]) {
        load_shader(atmosphere, "data/shaders/atmosphere.vert", "data/shaders/atmosphere.frag");
        log_msg("[I] Atmosphere shader reloaded.\n");
    }

    static bool orbit_enabled = false;
    if (Input_get_key_down(self)[K_1]) orbit_enabled = !orbit_enabled;
    if (orbit_enabled) {
        CameraOrbitController3D_update(ctrl_orbit, dtime, false);
    } else {
        CameraController3D_update(ctrl3d, dtime, false);
    }    
    Camera3D_update(camera3d);
}


// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    double time = Window_get_time(self);
    mat4 view, proj;
    Renderer_get_view_proj(self->renderer, view, proj);

    if (true) {
        Shader_begin(grid);
        mat4 gridmodel;
        glm_mat4_identity(gridmodel);
        float grid_size = 100.0f;
        glm_scale(gridmodel, (vec3){grid_size, grid_size, grid_size});
        // glm_translate(gridmodel, (vec3){camera3d->position.x, camera3d->position.y, camera3d->position.z});
        Shader_set_mat4(grid, "u_model", gridmodel);
        Shader_set_mat4(grid, "u_view", view);
        Shader_set_mat4(grid, "u_proj", proj);
        Shader_set_float(grid, "u_grid_size", 1.0f);
        Shader_set_float(grid, "u_line_width", 1.0f);
        Shader_set_float(grid, "u_fade_radius", grid_size*0.25f);
        Shader_set_float(grid, "u_fade_softness", grid_size*0.75f);
        Shader_set_vec3(grid, "u_grid_color", (Vec3f){0.5f, 0.5f, 0.5f});
        Shader_set_vec3(grid, "u_camera_pos", (Vec3f){camera3d->position.x, camera3d->position.y, camera3d->position.z});
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);
        Shader_end(grid);
    }

    if (true) {
        Shader_begin(atmosphere);
        Shader_set_vec2(atmosphere, "u_resolution", (Vec2f){Window_get_width(self), Window_get_height(self)});
        Shader_set_vec3(atmosphere, "u_camera_pos", (Vec3f){camera3d->position.x, camera3d->position.y, camera3d->position.z});
        Shader_set_vec3(atmosphere, "u_camera_rot", (Vec3f){camera3d->rotation.x, camera3d->rotation.y, camera3d->rotation.z});
        Shader_set_float(atmosphere, "u_camera_fov", camera3d->fov);

        Shader_set_vec3(atmosphere, "u_planet_pos", (Vec3f){0, 0, 0});
        Shader_set_float(atmosphere, "u_planet_rad", 1.0f);
        Shader_set_vec3(atmosphere, "u_sun_dir", Vec3f_norm((Vec3f){1, 0, 0}));

        Shader_set_float(atmosphere, "u_floor_rad", 1.0f);
        Shader_set_float(atmosphere, "u_atm_height", 1.0f);

        Shader_set_int(atmosphere, "u_num_in_scatter_points", 10);
        Shader_set_int(atmosphere, "u_num_optical_depth_points", 10);
        Shader_set_float(atmosphere, "u_density_falloff", 3.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){720, 530, 440});
        Shader_set_float(atmosphere, "u_scattering_strength", 1.0f);
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);
        Shader_end(atmosphere);
    }

    // Vec2f text_pos = {0.0f, 16.0f};
    // FontPixmap_set_align(font, FONT_ALIGN_BOTTOM_LEFT);
    // FontPixmap_set_scale_factor(font, (Vec2f){1.0f/100.0f, 1.0f/100.0f});
    // FontPixmap_render(font, text_pos.x, text_pos.y, 0, "CameraPos: x%.3f y%.3f z%.3f", camera3d->position.x, camera3d->position.y, camera3d->position.z);

    Window_display(self);
}


// Вызывается при изменении размера окна:
void resize(Window *self, int width, int height) {
    printf("Resize called.\n");
    Camera3D_resize(camera3d, width, height, false);
}


// Вызывается при разворачивании окна:
void show(Window *self) {
    printf("Show called.\n");
}


// Вызывается при сворачивании окна:
void hide(Window *self) {
    printf("Hide called.\n");
}


WindowScene TestScene = {
    .start   = start,
    .destroy = destroy,
    .update  = update,
    .render  = render,
    .resize  = resize,
    .show    = show,
    .hide    = hide
};


// Точка входа в программу:
int main(int argc, char *argv[]) {
    CGDF_Init();

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("[I] CGDF version: %s\n", cgdf_version);

    WinConfig *config = Window_create_config(TestScene);
    config->gl_major = 3;
    config->gl_minor = 3;
    Window *window = Window_create(config);
    Renderer_debug_config.debug_enabled = true;
    if (!Window_open(window)) {
        log_msg("[E] Window creation failed.\n");
    }

    Window_destroy(&window);
    Window_destroy_config(&config);

    print_after_free();

    return 0;
}
