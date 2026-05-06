//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>
#include <cgdf/graphics/opengl/texunit.h>
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
static Shader *atmosphere;
static FontPixmap *font;
static Mesh *sphere;
static float planet_rad = 1.0f;
static Vertex *vertices;
static uint32_t *indices;
static Node *sun, *earth, *moon;
static Model *model;


void generate_sphere(float radius, int sectors, int stacks, Vertex** vertices, unsigned int** indices, int* numVertices, int* numIndices) {
    *numVertices = (stacks + 1) * (sectors + 1);
    *numIndices = (stacks - 2) * sectors * 6 + 2 * sectors * 3;

    *vertices = (Vertex*)mm_alloc((*numVertices) * sizeof(Vertex));
    *indices = (unsigned int*)mm_alloc((*numIndices) * sizeof(unsigned int));

    float x, y, z, xy;
    float sectorStep = 2 * GLM_PI / sectors;
    float stackStep = GLM_PI / stacks;
    float sectorAngle, stackAngle;

    int vCount = 0;
    for (int i = 0; i <= stacks; ++i) {
        stackAngle = GLM_PI / 2 - i * stackStep;        // от pi/2 до -pi/2
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;           

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);

            // Позиция
            (*vertices)[vCount].px = x;
            (*vertices)[vCount].py = y;
            (*vertices)[vCount].pz = z;

            // Нормаль (для сферы это просто нормализованная позиция)
            // Если сфера в (0,0,0), то нормаль = pos / radius
            (*vertices)[vCount].nx = x / radius;
            (*vertices)[vCount].ny = y / radius;
            (*vertices)[vCount].nz = z / radius;

            // Цвет (белый по умолчанию)
            (*vertices)[vCount].r = 1.0f;
            (*vertices)[vCount].g = 1.0f;
            (*vertices)[vCount].b = 1.0f;
            (*vertices)[vCount].a = 1.0f;

            // UV координаты
            (*vertices)[vCount].u = (float)j / sectors;
            (*vertices)[vCount].v = (float)i / stacks;
            
            vCount++;
        }
    }

    int iCount = 0;
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                (*indices)[iCount++] = k1;
                (*indices)[iCount++] = k2;
                (*indices)[iCount++] = k1 + 1;
            }

            if (i != (stacks - 1)) {
                (*indices)[iCount++] = k1 + 1;
                (*indices)[iCount++] = k2;
                (*indices)[iCount++] = k2 + 1;
            }
        }
    }
}


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

    camera2d = Camera2D_create(self, Window_get_width(self), Window_get_height(self), (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);
    // Camera2D_set_meter(camera2d, 1.0f);
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
    ctrl_orbit = CameraOrbitController3D_create(self, camera3d, (Vec3d){0.0f, 0.0f, 0.0f}, 0.1f, 5.0f, 0.75f);
    ctrl_planet = CameraPlanetController3D_create(self, camera3d, 0.1f, 1.0f, 5.0f, 25.0f, 0.75f, false);

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

    blue_noise = Texture_create(self->renderer);
    Texture_load(blue_noise, "data/textures/blue-noise.bmp", false);

    int vert_count, idx_count;
    generate_sphere(planet_rad, 16, 8, &vertices, &indices, &vert_count, &idx_count);
    sphere = Mesh_create(vertices, vert_count, indices, idx_count, false);

    sun = Node_create(NULL);
    Node_set_scale(sun, (Vec3d){10.0f, 10.0f, 10.0f});

    earth = Node_create(sun);
    Node_set_position(earth, (Vec3d){5.0f, 0.0f, 0.0f});
    Node_set_scale(earth, (Vec3d){0.25f, 0.25f, 0.25f});

    moon = Node_create(earth);
    Node_set_position(moon, (Vec3d){5.0f, 0.0f, 0.0f});
    Node_set_scale(moon, (Vec3d){0.25f, 0.25f, 0.25f});

    model = Model_create(self->renderer);
    Model_add_mesh(model, sphere);

    printf("data loaded\n");
    // tinyobj_parse_mtl_file
}


// Вызывается при закрытии окна:
void destroy(Window *self) {
    printf("Destroy called.\n");
    print_before_free();
    Texture_destroy(&tex1);
    Texture_destroy(&blue_noise);
    SpriteBatch_destroy(&batch);
    SimpleDraw_destroy(&draw);
    Shader_destroy(&grid);
    Shader_destroy(&atmosphere);
    FontPixmap_destroy(&font);
    Camera2D_destroy(&camera2d);
    Camera3D_destroy(&camera3d);
    CameraController3D_destroy(&ctrl3d);
    CameraOrbitController3D_destroy(&ctrl_orbit);
    CameraPlanetController3D_destroy(&ctrl_planet);
    // Mesh_destroy(&sphere);
    mm_free(vertices);
    mm_free(indices);

    Node_destroy(&sun);

    Model_destroy(&model);
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
        // CameraPlanetController3D_update(ctrl_planet, dtime, false);
        ctrl3d->euler = Camera3D_get_euler(camera3d);
    } else {
        CameraController3D_update(ctrl3d, dtime, false);
        ctrl_orbit->euler = Camera3D_get_euler(camera3d);
    }

    if (Input_get_key_down(self)[K_2]) Node_copy(sun, sun);

    Node_rotate(sun, (Vec3d){1.0f, 1.0f, 1.0f}, 5.0f*dtime);
    Node_rotate(earth, (Vec3d){0.0f, 1.0f, 0.0f}, 10.0f*dtime);
    Node_rotate(moon, (Vec3d){0.0f, 0.0f, 1.0f}, 25.0f*dtime);

    ctrl_orbit->target_pos = Node_get_world_position(earth);
    Camera3D_update(camera3d);
}
void node_render(Window *self, Node *parent) {
    Node_get_transform(parent, model->transform);
    Shader_set_mat4(self->renderer->shader, "u_model", model->transform);
    Model_render(model, false);

    for (int i=0; i<Node_count_nodes(parent); i++) {
        node_render(self, Array_get_ptr(Node_get_children(parent), i));
    }
}
// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    double time = Window_get_time(self);
    mat4 view, proj;
    Renderer_get_view_proj(self->renderer, view, proj);


    /*
    if (true) {
        mat4 inv_view;
        glm_mat4_inv(camera3d->view, inv_view);
        Shader_begin(atmosphere);
        Shader_set_vec2(atmosphere, "u_resolution", (Vec2f){Window_get_width(self), Window_get_height(self)});
        Shader_set_mat4(atmosphere, "u_inv_view", inv_view);
        Shader_set_float(atmosphere, "u_camera_fov", camera3d->fov);

        Shader_set_vec3(atmosphere, "u_planet_pos", (Vec3f){0, 0, 0});
        Shader_set_float(atmosphere, "u_planet_rad", planet_rad);
        Shader_set_vec3(atmosphere, "u_sun_dir", Vec3f_norm((Vec3f){1, 0, 0}));

        Shader_set_float(atmosphere, "u_floor_rad", planet_rad);
        Shader_set_float(atmosphere, "u_atm_height", 5.0f);

        Shader_set_int(atmosphere, "u_num_in_scatter_points", 10);
        Shader_set_int(atmosphere, "u_num_optical_depth_points", 5);
        Shader_set_float(atmosphere, "u_density_falloff", 3.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){700, 530, 440});
        Shader_set_float(atmosphere, "u_scattering_strength", 1.0f);
        Shader_set_tex2d(atmosphere, "u_blue_noise", blue_noise->id);
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);
        Shader_end(atmosphere);
    }

    mat4 mdl;
    glm_mat4_identity(mdl);
    glm_rotate(mdl, radians(90), (vec3){1, 0, 0});
    Shader_begin(self->renderer->shader);
    Shader_set_bool(self->renderer->shader, "u_use_texture", false);
    Shader_set_vec4(self->renderer->shader, "u_color", (Vec4f){1, 1, 1, 1});
    Shader_set_mat4(self->renderer->shader, "u_model", mdl);
    Mesh_render(sphere, true);
    Shader_end(self->renderer->shader);

    Vec2d screen_pos = world3d_to_screen(camera3d, (Vec3d){0, 0, 0});
    Vec2f text_pos = {screen_pos.x, screen_pos.y};
    FontPixmap_set_align(font, FONT_ALIGN_CENTER_CENTER);
    float scale = 0.5f;
    FontPixmap_set_scale_factor(font, (Vec2f){scale, scale});
    FontPixmap_set_color(font, (Vec4f){0, 0.75, 0, 1});
    FontPixmap_set_bg_color(font, (Vec4f){0, 0, 0, 0.5f});
    FontPixmap_set_bg_padding(font, (Vec4f){8, 8, 8, 8});

    Camera2D_update(camera2d);
    Camera2D_ui_begin(camera2d);
    Vec3d rot = Camera3D_get_euler(camera3d);
    FontPixmap_render(font, text_pos.x, text_pos.y, 0, "CameraPos: x%.3f y%.3f z%.3f\nCameraRot: x%.3f y%.3f z%.3f\nCameraFov: %.3f",
        camera3d->position.x, camera3d->position.y, camera3d->position.z,
        rot.x, rot.y, rot.z,
        camera3d->fov
    );
    Camera2D_ui_end(camera2d);
    */

    if (true) {
        mat4 inv_view;
        glm_mat4_inv(camera3d->view, inv_view);
        Shader_begin(atmosphere);
        Shader_set_vec2(atmosphere, "u_resolution", (Vec2f){Window_get_width(self), Window_get_height(self)});
        Shader_set_mat4(atmosphere, "u_inv_view", inv_view);
        Shader_set_float(atmosphere, "u_camera_fov", camera3d->fov);

        Shader_set_int(atmosphere, "u_num_in_scatter_points", 10);
        Shader_set_int(atmosphere, "u_num_optical_depth_points", 5);
        Shader_set_float(atmosphere, "u_density_falloff", 3.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){700, 530, 440});
        Shader_set_float(atmosphere, "u_scattering_strength", 1.0f);
        Shader_set_tex2d(atmosphere, "u_blue_noise", blue_noise->id);

        Vec3d _sun_pos = Node_get_world_position(sun);
        Vec3f sun_pos = (Vec3f){_sun_pos.x, _sun_pos.y, _sun_pos.z};
        Shader_set_vec3(atmosphere, "u_planet_pos", sun_pos);
        Shader_set_float(atmosphere, "u_planet_rad", Vec3d_len(Node_get_world_scale(sun))/2.0);
        Shader_set_vec3(atmosphere, "u_sun_dir", Vec3f_norm((Vec3f){0, 0, 0}));
        Shader_set_float(atmosphere, "u_floor_rad", Vec3d_len(Node_get_world_scale(sun))/2.0);
        Shader_set_float(atmosphere, "u_atm_height", 5.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){440, 530, 700});
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);

        Vec3d _earth_pos = Node_get_world_position(earth);
        Vec3f earth_pos = (Vec3f){_earth_pos.x, _earth_pos.y, _earth_pos.z};
        Shader_set_vec3(atmosphere, "u_planet_pos", earth_pos);
        Shader_set_float(atmosphere, "u_planet_rad", Vec3d_len(Node_get_world_scale(earth))/2.0);
        Shader_set_vec3(atmosphere, "u_sun_dir", Vec3f_norm((Vec3f){0.0-earth_pos.x, 0.0-earth_pos.y, 0.0-earth_pos.z}));
        Shader_set_float(atmosphere, "u_floor_rad", Vec3d_len(Node_get_world_scale(earth))/2.0);
        Shader_set_float(atmosphere, "u_atm_height", 3.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){700, 530, 440});
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);

        Vec3d _moon_pos = Node_get_world_position(moon);
        Vec3f moon_pos = (Vec3f){_moon_pos.x, _moon_pos.y, _moon_pos.z};
        Shader_set_vec3(atmosphere, "u_planet_pos", moon_pos);
        Shader_set_float(atmosphere, "u_planet_rad", Vec3d_len(Node_get_world_scale(moon))/2.0);
        Shader_set_vec3(atmosphere, "u_sun_dir", Vec3f_norm((Vec3f){0.0-moon_pos.x, 0.0-moon_pos.y, 0.0-moon_pos.z}));
        Shader_set_float(atmosphere, "u_floor_rad", Vec3d_len(Node_get_world_scale(moon))/2.0);
        Shader_set_float(atmosphere, "u_atm_height", 1.0f);
        Shader_set_vec3(atmosphere, "u_wavelenghts", (Vec3f){500, 500, 500});
        Sprite2D_render(self->renderer, NULL, 0, 0, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, true);

        Shader_end(atmosphere);
    }
    Camera3D_set_depth_test(camera3d, true);
    Shader_begin(self->renderer->shader);
    Shader_set_bool(self->renderer->shader, "u_use_texture", false);
    Shader_set_vec4(self->renderer->shader, "u_color", (Vec4f){1.0f, 1.0f, 1.0f, 1.0f});

    Node_get_transform(sun, model->transform);
    Shader_set_mat4(self->renderer->shader, "u_model", model->transform);
    Model_render(model, false);

    Node_get_transform(earth, model->transform);
    Shader_set_mat4(self->renderer->shader, "u_model", model->transform);
    Model_render(model, false);

    Node_get_transform(moon, model->transform);
    Shader_set_mat4(self->renderer->shader, "u_model", model->transform);
    Model_render(model, false);
    // node_render(self, sun);

    Shader_end(self->renderer->shader);


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

    // Нарисовать линии:
    float line_dist = 1000;
    SimpleDraw_line(draw, (Vec4f){1, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){line_dist, 0, 0}, 3.0f);
    SimpleDraw_line(draw, (Vec4f){0.25, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){-line_dist, 0, 0}, 3.0f);
    SimpleDraw_line(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, line_dist, 0}, 3.0f);
    SimpleDraw_line(draw, (Vec4f){0, 0.25, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, -line_dist, 0}, 3.0f);
    SimpleDraw_line(draw, (Vec4f){0, 0, 1, 1}, (Vec3f){0, 0, 0}, (Vec3f){line_dist}, 3.0f);
    SimpleDraw_line(draw, (Vec4f){0, 0, 0.25, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 0, -line_dist}, 3.0f);

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
        "CPU:\n%s [%s]\nThreads: %d\n\n"
        "RAM:\nUSED: %zu MB\nFREE: %zu MB\nTOTAL: %zu MB\n\n"
        "MEM:\n%.2f MB\n\n"
        "FPS: %.2f\n",
        cpu_info.model,
        Info_get_cpu_arch_name(cpu_info.arch),
        cpu_info.threads,
        mem_info.used / 1024 / 1024,
        mem_info.free / 1024 / 1024,
        mem_info.total / 1024 / 1024,
        mm_get_used_size_mb(),
        fps
    );
    Camera2D_ui_end(camera2d);

    Window_display(self);
}


// Вызывается при изменении размера окна:
void resize(Window *self, int width, int height) {
    printf("Resize called.\n");
    Camera3D_resize(camera3d, width, height, false);
    Camera2D_resize(camera2d, width, height);
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

    CpuInfo cpu_info = Info_get_cpu();
    printf("CPU Model: \"%s\"\n", cpu_info.model);
    printf("CPU Arch: %s\n", Info_get_cpu_arch_name(cpu_info.arch));
    printf("Threads: %d\n", cpu_info.threads);
    MemInfo mem_info = Info_get_mem();
    printf("Total RAM: %zu MB\n", mem_info.total / 1024 / 1024);
    printf("Free RAM: %zu MB\n", mem_info.free / 1024 / 1024);
    printf("Used RAM: %zu MB\n", mem_info.used / 1024 / 1024);

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("[I] CGDF version: %s\n", cgdf_version);

    WinConfig *config = Window_create_config(&TestScene);
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
