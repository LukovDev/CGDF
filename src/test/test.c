//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>
#include "game.h"


static Texture *tex1;
static Texture *blue_noise;
static CameraController3D *ctrl3d;
static CameraOrbitController3D *ctrl_orbit;
static CameraPlanetController3D *ctrl_planet;
static Camera3D *camera3d;
static Camera2D *camera2d;
static SpriteBatch *batch;
static SimpleDraw *draw;
static Shader *grid;
static FontPixmap *font;
static Material *material;
static OBJFile objfile;
static OBJFile objfile2;


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


static size_t calculate_mesh_size(OBJFile *file) {
    size_t size = 0;
    for (size_t i=0; i < Array_len(file->models); i++) {
        Model *model = (Model*)Array_get_ptr(file->models, i);
        for (size_t j=0; j < Array_len(model->meshes); j++) {
            Mesh *mesh = (Mesh*)Array_get_ptr(model->meshes, j);
            size += Mesh_get_size(mesh);
        }
    }
    return size;
}


// Вызывается после создания окна:
void start(Window *self) {
    log_msg("[I] Start called.\n");
    Window_set_fps(self, 0);
    Window_set_vsync(self, false);

    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Window_set_title(self, "CGDF Window");
    Pixmap_destroy(&icon);

    camera2d = Camera2D_create(self, Window_get_width(self), Window_get_height(self), (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);
    // Camera2D_set_meter(camera2d, 1.0f);
    camera3d = Camera3D_create(
        self, Window_get_width(self), Window_get_height(self),
        (Vec3d){0.0f, 1.5f, 10.0f},
        (Vec3d){0.0f, 0.0f, 0.0f},
        (Vec3d){1.0f, 1.0f, 1.0f},
        90.0f,
        0.01f, 10000.0f,
        false
    );
    Renderer_set_cull_faces(self->renderer, false);
    Renderer_set_depth_test(self->renderer, false);
    ctrl3d = CameraController3D_create(self, camera3d, 0.1f, 1.0f, 5.0f, 25.0f, 0.75f, false);
    ctrl_orbit = CameraOrbitController3D_create(self, camera3d, (Vec3d){0.0f, 0.0f, 0.0f}, 0.1f, 5.0f, 0.75f);
    ctrl_planet = CameraPlanetController3D_create(self, camera3d, 0.1f, 1.0f, 5.0f, 25.0f, 0.75f, false);

    log_msg("[I] Loading data...\n");

    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/logo/CGDF2x2.png", true);
    // Texture_set_pixelized(tex1);

    batch = SpriteBatch_create(self->renderer);

    draw = SimpleDraw_create(self->renderer);

    grid = Shader_create(self->renderer, NULL, NULL, NULL);
    load_shader(grid, "data/shaders/grid.vert", "data/shaders/grid.frag");

    font = FontPixmap_create(self->renderer, "data/fonts/pixel.ttf", 32);
    FontPixmap_set_pixelized(font, true);

    blue_noise = Texture_create(self->renderer);
    // Texture_load(blue_noise, "data/DefaultPacks/hdr-skyspheres-pack/moonless_golf_16k.hdr", false);

    material = Material_create_default(NULL);

    objfile = ObjLoader_load(self->renderer, "data/obj/cat/cat.obj");
    objfile2 = ObjLoader_load(self->renderer, "data/obj/demo_scene/demo_scene.obj");

    log_msg("[I] data loaded\n");
}


// Вызывается при закрытии окна:
void destroy(Window *self) {
    log_msg("[I] Destroy called.\n");
    print_before_free();
    Texture_destroy(&tex1);
    Texture_destroy(&blue_noise);
    SpriteBatch_destroy(&batch);
    SimpleDraw_destroy(&draw);
    Shader_destroy(&grid);
    FontPixmap_destroy(&font);
    Camera2D_destroy(&camera2d);
    Camera3D_destroy(&camera3d);
    CameraController3D_destroy(&ctrl3d);
    CameraOrbitController3D_destroy(&ctrl_orbit);
    CameraPlanetController3D_destroy(&ctrl_planet);

    Material_destroy(&material);

    for (size_t i=0; i < Array_len(objfile.models); i++) {
        Model *model = Array_get_ptr(objfile.models, i);
        Model_destroy(&model);
    }
    for (size_t i=0; i < Array_len(objfile.materials); i++) {
        Material *mat = Array_get_ptr(objfile.materials, i);
        Material_destroy(&mat);
    }
    Array_destroy(&objfile.models);
    Array_destroy(&objfile.materials);

    for (size_t i=0; i < Array_len(objfile2.models); i++) {
        Model *model = Array_get_ptr(objfile2.models, i);
        Model_destroy(&model);
    }
    for (size_t i=0; i < Array_len(objfile2.materials); i++) {
        Material *mat = Array_get_ptr(objfile2.materials, i);
        Material_destroy(&mat);
    }
    Array_destroy(&objfile2.models);
    Array_destroy(&objfile2.materials);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, float dtime) {
    static bool orbit_enabled = false;
    if (Input_get_key_down(self)[K_1]) orbit_enabled = !orbit_enabled;
    if (Input_get_key_down(self)[K_2]) {
        for (size_t i=0; i < Array_len(objfile.models); i++) {
            Model *model = Array_get_ptr(objfile.models, i);
            Model_destroy(&model);
        }
        Array_clear(objfile.models, false);
        for (size_t i=0; i < Array_len(objfile2.models); i++) {
            Model *model = Array_get_ptr(objfile2.models, i);
            Model_destroy(&model);
        }
        Array_clear(objfile2.models, false);
        for (size_t i=0; i < Array_len(objfile.materials); i++) {
            Material *mat = Array_get_ptr(objfile.materials, i);
            Material_destroy(&mat);
        }
        Array_clear(objfile.materials, false);
        for (size_t i=0; i < Array_len(objfile2.materials); i++) {
            Material *mat = Array_get_ptr(objfile2.materials, i);
            Material_destroy(&mat);
        }
        Array_clear(objfile2.materials, false);
        Texture_destroy(&blue_noise);
    }
    if (orbit_enabled) {
        CameraOrbitController3D_update(ctrl_orbit, dtime, false);
        // CameraPlanetController3D_update(ctrl_planet, dtime, false);
        ctrl3d->euler = Camera3D_get_euler(camera3d);
    } else {
        CameraController3D_update(ctrl3d, dtime, false);
        ctrl_orbit->euler = Camera3D_get_euler(camera3d);
    }

    Camera3D_update(camera3d);
}

// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    double time = Window_get_time(self);
    mat4 view, proj;
    Renderer_get_view_proj(self->renderer, view, proj);

    Model *cat = Array_get_ptr(objfile.models, 0);
    glm_mat4_identity(cat->transform);
    glm_translate(cat->transform, (vec3){-7.0f, 0.2f, 3.0f});
    glm_rotate(cat->transform, radians(90.0f), (vec3){0, 1, 0});
    glm_rotate(cat->transform, radians(-90.0f), (vec3){1, 0, 0});
    glm_scale(cat->transform, (vec3){0.2f, 0.2f, 0.2f});
    Model_render(cat, false);

    for (size_t i=0; i < Array_len(objfile2.models); i++) {
        Model *model = Array_get_ptr(objfile2.models, i);
        Model_render(model, false);
    }
    Renderer_display(self->renderer);

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

        // Нарисовать линии:
        float line_dist = 1000;
        SimpleDraw_line(draw, (Vec4f){1, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){line_dist, 0, 0}, 1.0f);      // X+
        SimpleDraw_line(draw, (Vec4f){0.25, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){-line_dist, 0, 0}, 3.0f);  // X-
        SimpleDraw_line(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, line_dist, 0}, 3.0f);      // Y+
        SimpleDraw_line(draw, (Vec4f){0, 0.25, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, -line_dist, 0}, 3.0f);  // Y-
        SimpleDraw_line(draw, (Vec4f){0, 0, 1, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 0, line_dist}, 3.0f);      // Z+
        SimpleDraw_line(draw, (Vec4f){0, 0, 0.25, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 0, -line_dist}, 3.0f);  // Z-
    }

    Camera2D_update(camera2d);
    Camera2D_ui_begin(camera2d);

    Vec2f text_pos = {16, 16};
    float scale = 0.5f;
    FontPixmap_set_scale_factor(font, (Vec2f){scale, scale});
    FontPixmap_set_align(font, FONT_ALIGN_BOTTOM_LEFT);
    FontPixmap_set_color(font, (Vec4f){1, 1, 1, 1});
    FontPixmap_set_bg_color(font, (Vec4f){0, 0, 0, 0.5f});
    FontPixmap_set_bg_padding(font, (Vec4f){8, 8, 8, 8});
    FontPixmap_set_line_height(font, 16.0f);

    static float fps = 0.0f;
    static float timer = 0.0f;
    if (timer >= 0.5f) {
        timer = 0.0f;
        fps = Window_get_current_fps(self);
    } else {
        timer += dtime;
    }

    CpuInfo cpu_info = Info_get_cpu();
    MemInfo mem_info = Info_get_mem();
    FontPixmap_render(font, text_pos.x, text_pos.y, 0,
        "CPU:\n"
        "%s [%s]\n"
        "Threads: %d\n\n"
        "GPU:\n"
        "%s\n"
        "OpenGL %s\n"
        "USED: %.2f MB\n"
        "FREE: %.2f MB\n"
        "TOTAL: %.2f MB\n\n"
        "RAM:\n"
        "USED: %.2f MB\n"
        "FREE: %.2f MB\n"
        "TOTAL: %.2f MB\n\n"
        "Memory Manager:\n"
        "Used: %.2f MB\n"
        "Mesh used: %.2f MB\n\n"
        "Textures used: %.2f MB\n\n"
        "FPS: %.2f\n",
        cpu_info.model,
        Info_get_cpu_arch_name(cpu_info.arch),
        cpu_info.threads,
        Renderer_get_renderer(self->renderer),
        Renderer_get_version(self->renderer),
        Renderer_get_used_memory(self->renderer) / 1024.0,
        Renderer_get_free_memory(self->renderer) / 1024.0,
        Renderer_get_total_memory(self->renderer) / 1024.0,
        (double)mem_info.used / 1024.0 / 1024.0,
        (double)mem_info.free / 1024.0 / 1024.0,
        (double)mem_info.total / 1024.0 / 1024.0,
        mm_get_used_size_mb(),
        (double)(calculate_mesh_size(&objfile) + calculate_mesh_size(&objfile2)) / 1024.0f / 1024.0f,
        (double)(Texture_get_size(blue_noise)) / 1024.0 / 1024.0,
        fps
    );
    Camera2D_ui_end(camera2d);

    Window_display(self);
}


// Вызывается при изменении размера окна:
void resize(Window *self, int width, int height) {
    log_msg("[I] Resize called.\n");
    Camera3D_resize(camera3d, width, height, false);
    Camera2D_resize(camera2d, width, height);
}


// Вызывается при разворачивании окна:
void show(Window *self) {
    log_msg("[I] Show called.\n");
}


// Вызывается при сворачивании окна:
void hide(Window *self) {
    log_msg("[I] Hide called.\n");
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
    CGDF_init();

    log_msg("[I] CWD: \"%s\"\n", Files_get_cwd(NULL, 0));

    CpuInfo cpu_info = Info_get_cpu();
    log_msg("[I] CPU Model: \"%s\"\n", cpu_info.model);
    log_msg("[I] CPU Arch: \"%s\"\n", Info_get_cpu_arch_name(cpu_info.arch));
    log_msg("[I] Threads: %d\n", cpu_info.threads);
    MemInfo mem_info = Info_get_mem();
    log_msg("[I] Total RAM: %zu MB\n", mem_info.total / 1024 / 1024);
    log_msg("[I] Free RAM: %zu MB\n", mem_info.free / 1024 / 1024);
    log_msg("[I] Used RAM: %zu MB\n", mem_info.used / 1024 / 1024);

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("[I] CGDF version: \"%s\"\n", cgdf_version);

    WinConfig *config = Window_create_config(&TestScene);
    config->gl_major = 3;
    config->gl_minor = 3;
    Window *window = Window_create(config);
    g_Renderer_debug_config.debug_enabled = true;
    if (!Window_open(window)) {
        log_msg("[E] Window creation failed.\n");
    }

    Window_destroy(&window);
    Window_destroy_config(&config);

    CGDF_destroy();
    print_after_free();

    return EXIT_SUCCESS;
}
