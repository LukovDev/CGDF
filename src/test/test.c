//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>


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
static Shader *atmo;
static FontPixmap *font;
static Material *material;
static Material *floor_material;
static OBJFile objfile;
static OBJFile objfile2;
static OBJFile objfile3;
static OBJFile objfile_ship1;
static OBJFile objfile_ship2;
static OBJFile model;


static void print_before_free(void) {
    log_msg("[I] (Before free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
}


static void print_after_free(void) {
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


static void destroy_objfile(OBJFile file) {
    for (size_t i=0; i < Array_len(file.models); i++) {
        Model *model = Array_get_ptr(file.models, i);
        Model_destroy(&model);
    }
    for (size_t i=0; i < Array_len(file.materials); i++) {
        Material *mat = Array_get_ptr(file.materials, i);
        Material_destroy(&mat);
    }
    Array_destroy(&file.models);
    Array_destroy(&file.materials);
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
        0.01f, 5000.0f,
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

    atmo = Shader_create(self->renderer, NULL, NULL, NULL);
    load_shader(atmo, "data/shaders/atmosphere.vert", "data/shaders/atmosphere.frag");

    font = FontPixmap_create(self->renderer, "data/fonts/inter/inter-semibold.ttf", 32);
    FontPixmap_set_pixelized(font, true);

    blue_noise = Texture_create(self->renderer);
    Texture_load(blue_noise, "data/textures/blue-noise.bmp", false);

    Texture *albedo = Texture_create(self->renderer);
    Texture *normal = Texture_create(self->renderer);
    Texture *occlusion = Texture_create(self->renderer);
    Texture *roughness = Texture_create(self->renderer);
    Texture *metallic = Texture_create(self->renderer);
    Texture *emission = Texture_create(self->renderer);
    Texture *height = Texture_create(self->renderer);

    #define PACKTYPE "roads"
    #define PACKNAME "wedged-cobblestone"  // beige-stonework, square-block-vegetation, wedged-cobblestone
    Texture_load(albedo, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/albedo.png", true);
    Texture_load(normal, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/normal.png", true);
    Texture_load(occlusion, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/ao.png", true);
    Texture_load(roughness, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/roughness.png", true);
    Texture_load(metallic, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/metallic.png", true);
    Texture_load(emission, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/albedo.png", true);
    Texture_load(height, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/height.png", true);

    material = Material_create(
        "Material 01",
        (Vec4f){1, 1, 1, 1},
        (Vec3f){1, 1, 1},
        1.0f, 1.0f, 0.0f, 1.0f,
        (Vec3f){1, 1, 1}, 1.0f, 0.05f, 16.0f, 128.0f, false,
        0.0f, false, false, 0.0f, 0.0f,
        albedo, normal, occlusion,
        roughness, metallic, emission,
        height
    );
    material->owns_albedo_map = true;
    material->owns_normal_map = true;
    material->owns_occlusion_map = true;
    material->owns_roughness_map = true;
    material->owns_metallic_map = true;
    material->owns_emissive_map = true;
    material->owns_height_map = true;

    Texture *floor_albedo = Texture_create(self->renderer);
    Texture *floor_normal = Texture_create(self->renderer);
    Texture *floor_occlusion = Texture_create(self->renderer);
    Texture *floor_roughness = Texture_create(self->renderer);
    Texture *floor_metallic = Texture_create(self->renderer);
    Texture *floor_emission = Texture_create(self->renderer);
    Texture *floor_height = Texture_create(self->renderer);

    #define PACKTYPE "roads"
    #define PACKNAME "square-block-vegetation"  // beige-stonework, square-block-vegetation, wedged-cobblestone
    Texture_load(floor_albedo, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/albedo.png", true);
    Texture_load(floor_normal, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/normal.png", true);
    Texture_load(floor_occlusion, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/ao.png", true);
    Texture_load(floor_roughness, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/roughness.png", true);
    Texture_load(floor_metallic, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/metallic.png", true);
    Texture_load(floor_emission, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/albedo.png", true);
    Texture_load(floor_height, "data/packs/pbr-pack/"PACKTYPE"/"PACKNAME"/height.png", true);

    floor_material = Material_create(
        "Material 02",
        (Vec4f){1, 1, 1, 1},
        (Vec3f){1, 1, 1},
        1.0f, 1.0f, 0.0f, 1.0f,
        (Vec3f){1, 1, 1}, 1.0f, 0.05f, 16.0f, 128.0f, false,
        0.0f, false, false, 0.0f, 0.0f,
        floor_albedo, floor_normal, floor_occlusion,
        floor_roughness, floor_metallic, floor_emission,
        floor_height
    );
    floor_material->owns_albedo_map = true;
    floor_material->owns_normal_map = true;
    floor_material->owns_occlusion_map = true;
    floor_material->owns_roughness_map = true;
    floor_material->owns_metallic_map = true;
    floor_material->owns_emissive_map = true;
    floor_material->owns_height_map = true;

    objfile = ObjLoader_load(self->renderer, "data/obj/cat/cat.obj");
    objfile2 = ObjLoader_load(self->renderer, "data/obj/demo_scene/demo_scene.obj");
    objfile3 = ObjLoader_load(self->renderer, "data/obj/demo_scene/sphere.obj");
    objfile_ship1 = ObjLoader_load(self->renderer, "data/obj/example/SpaceShip1.obj");
    objfile_ship2 = ObjLoader_load(self->renderer, "data/obj/example/SpaceShip2.obj");
    model = ObjLoader_load(self->renderer, "data/obj/home/Cottage.obj");

    // Model *model0 = Array_get_ptr(model.models, 0);
    // for (size_t i=0; i<Array_len(model0->meshes); i++) {
    //     Mesh *mesh = Array_get_ptr(model0->meshes, i);
    //     Mesh_set_material(mesh, material);
    // }
    
    Model *floor = Array_get_ptr(objfile2.models, 0);
    for (size_t i=0; i<Array_len(floor->meshes); i++) {
        Mesh *mesh = Array_get_ptr(floor->meshes, i);
        Mesh_set_material(mesh, floor_material);
    }
    Model *tmp = NULL;
    Array_remove(objfile3.models, 0, &tmp);
    Model_destroy(&tmp);

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
    Shader_destroy(&atmo);
    FontPixmap_destroy(&font);
    Camera2D_destroy(&camera2d);
    Camera3D_destroy(&camera3d);
    CameraController3D_destroy(&ctrl3d);
    CameraOrbitController3D_destroy(&ctrl_orbit);
    CameraPlanetController3D_destroy(&ctrl_planet);

    Material_destroy(&material);
    Material_destroy(&floor_material);

    destroy_objfile(objfile);
    destroy_objfile(objfile2);
    destroy_objfile(objfile3);
    destroy_objfile(objfile_ship1);
    destroy_objfile(objfile_ship2);
    destroy_objfile(model);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, float dtime) {
    if (Input_get_key_down(self)[K_f]) Window_set_fullscreen(self, !Window_get_fullscreen(self));

    static bool orbit_enabled = false;
    if (Input_get_key_down(self)[K_1]) orbit_enabled = !orbit_enabled;
    if (Input_get_key_down(self)[K_2]) {
        for (size_t i=0; i < Array_len(objfile.models); i++) {
            Model *model = Array_get_ptr(objfile.models, i);
            Model_destroy(&model);
        }
        Array_clear(objfile.models, false);
        for (size_t i=0; i < Array_len(objfile.materials); i++) {
            Material *mat = Array_get_ptr(objfile.materials, i);
            Material_destroy(&mat);
        }
        Array_clear(objfile.materials, false);

        for (size_t i=0; i < Array_len(objfile2.models); i++) {
            Model *model = Array_get_ptr(objfile2.models, i);
            Model_destroy(&model);
        }
        Array_clear(objfile2.models, false);
        for (size_t i=0; i < Array_len(objfile2.materials); i++) {
            Material *mat = Array_get_ptr(objfile2.materials, i);
            Material_destroy(&mat);
        }
        Array_clear(objfile2.materials, false);

        for (size_t i=0; i < Array_len(objfile3.models); i++) {
            Model *model = Array_get_ptr(objfile3.models, i);
            Model_destroy(&model);
        }
        Array_clear(objfile3.models, false);
        for (size_t i=0; i < Array_len(objfile3.materials); i++) {
            Material *mat = Array_get_ptr(objfile3.materials, i);
            Material_destroy(&mat);
        }
        Array_clear(objfile3.materials, false);
    }
    if (orbit_enabled) {
        ctrl_orbit->target_pos = (Vec3d){0.0f, 1.5f, 0.0f};
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
    if (cat) {
        glm_mat4_identity(cat->transform);
        glm_translate(cat->transform, (vec3){-7.0f, 0.2f, 3.0f});
        glm_rotate(cat->transform, radians(90.0f), (vec3){0, 1, 0});
        glm_rotate(cat->transform, radians(-90.0f), (vec3){1, 0, 0});
        glm_scale(cat->transform, (vec3){0.2f, 0.2f, 0.2f});
        Model_render(cat, false);
    }

    Model *sphere = Array_get_ptr(model.models, 0);
    if (sphere) {
        glm_mat4_identity(sphere->transform);
        glm_translate(sphere->transform, (vec3){0, 0, 0});
        // glm_rotate(sphere->transform, radians(-time*10), (vec3){0, 1, 0});
        // glm_rotate(sphere->transform, radians(time*10), (vec3){1, 0, 0});
        // glm_rotate(sphere->transform, radians(-time*10), (vec3){0, 0, 1});
        glm_scale(sphere->transform, (vec3){0.5f, 0.5f, 0.5f});
        Model_render(sphere, false);
    }

    for (size_t i=0; i < Array_len(objfile2.models); i++) {
        Model *model = Array_get_ptr(objfile2.models, i);
        Model_render(model, false);
    }

    for (size_t i=0; i < Array_len(objfile_ship1.models); i++) {
        Model *model = Array_get_ptr(objfile_ship1.models, i);
        glm_mat4_identity(model->transform);
        glm_rotate(model->transform, radians(time*60), (vec3){0, 1, 0});
        glm_rotate(model->transform, radians(0), (vec3){1, 0, 0});
        glm_rotate(model->transform, radians(0), (vec3){0, 0, 1});
        glm_translate(model->transform, (vec3){-3, 5.0f, 0.0f});
        glm_rotate(model->transform, radians(180), (vec3){0, 1, 0});
        glm_rotate(model->transform, radians(0), (vec3){1, 0, 0});
        glm_rotate(model->transform, radians(sinf(time)*45.0f), (vec3){0, 0, 1});
        // glm_scale(model->transform, (vec3){0.1f, 0.1f, 0.1f});
        Model_render(model, false);
    }
    for (size_t i=0; i < Array_len(objfile_ship2.models); i++) {
        Model *model = Array_get_ptr(objfile_ship2.models, i);
        glm_mat4_identity(model->transform);
        glm_rotate(model->transform, radians(time*60), (vec3){0, 1, 0});
        glm_rotate(model->transform, radians(0), (vec3){1, 0, 0});
        glm_rotate(model->transform, radians(0), (vec3){0, 0, 1});
        glm_translate(model->transform, (vec3){3, 5.0f, 0.0f});
        glm_rotate(model->transform, radians(0), (vec3){0, 1, 0});
        glm_rotate(model->transform, radians(0), (vec3){1, 0, 0});
        glm_rotate(model->transform, radians(sinf(time)*45.0f), (vec3){0, 0, 1});
        // glm_scale(model->transform, (vec3){0.1f, 0.1f, 0.1f});
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
        // float line_dist = 1000;
        // SimpleDraw_line(draw, (Vec4f){1, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){line_dist, 0, 0}, 1.0f);      // X+
        // SimpleDraw_line(draw, (Vec4f){0.25, 0, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){-line_dist, 0, 0}, 3.0f);  // X-
        // SimpleDraw_line(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, line_dist, 0}, 3.0f);      // Y+
        // SimpleDraw_line(draw, (Vec4f){0, 0.25, 0, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, -line_dist, 0}, 3.0f);  // Y-
        // SimpleDraw_line(draw, (Vec4f){0, 0, 1, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 0, line_dist}, 3.0f);      // Z+
        // SimpleDraw_line(draw, (Vec4f){0, 0, 0.25, 1}, (Vec3f){0, 0, 0}, (Vec3f){0, 0, -line_dist}, 3.0f);  // Z-
    }

    int width = Renderer_get_width(self->renderer);
    int height = Renderer_get_height(self->renderer);
    Texture *albedo_roughness = GBuffer_get_tex_albedo_roughness(self->renderer->gbuffer);
    Texture *normal_ao = GBuffer_get_tex_normal_ao(self->renderer->gbuffer);
    Texture *pbr_properties = GBuffer_get_tex_pbr_properties(self->renderer->gbuffer);
    Texture *emissive = GBuffer_get_tex_emissive(self->renderer->gbuffer);
    Texture *depth = GBuffer_get_tex_depth(self->renderer->gbuffer);

    Camera2D_update(camera2d);
    Camera2D_ui_begin(camera2d);

    static bool text_en = true;
    if (Input_get_key_down(self)[K_t]) text_en = !text_en;

    float w = albedo_roughness->width * 0.25f, h = albedo_roughness->height * 0.25f;
    float x1 = width, y1 = height-h;
    float x2 = width, y2 = height-h - h;
    float x3 = width, y3 = height-h - h * 2;
    float x4 = width, y4 = height-h - h * 3;
    if (true) {
        Shader_begin(self->renderer->shader);
        Shader_set_bool(self->renderer->shader, "u_use_gbuffer", true);
        Shader_set_int(self->renderer->shader, "u_gbuffer_view", 0);
        Shader_end(self->renderer->shader);
        Sprite2D_render(self->renderer, albedo_roughness, 0, 0, albedo_roughness->width, albedo_roughness->height, 0.0f, (Vec4f){1, 1, 1, 1}, false);
        Sprite2D_render(self->renderer, albedo_roughness, x1 - w, y1, w, h, 0.0f, (Vec4f){1, 1, 1, 1}, false);

        Shader_begin(self->renderer->shader);
        Shader_set_int(self->renderer->shader, "u_gbuffer_view", 1);
        Shader_end(self->renderer->shader);
        Sprite2D_render(self->renderer, normal_ao, x2 - w, y2, w, h, 0.0f, (Vec4f){1, 1, 1, 1}, false);

        Shader_begin(self->renderer->shader);
        Shader_set_int(self->renderer->shader, "u_gbuffer_view", 2);
        Shader_end(self->renderer->shader);
        Sprite2D_render(self->renderer, pbr_properties, x3 - w, y3, w, h, 0.0f, (Vec4f){1, 1, 1, 1}, false);

        Shader_begin(self->renderer->shader);
        Shader_set_int(self->renderer->shader, "u_gbuffer_view", 3);
        Shader_end(self->renderer->shader);
        Sprite2D_render(self->renderer, emissive, x4 - w, y4, w, h, 0.0f, (Vec4f){1, 1, 1, 1}, false);

        Shader_begin(self->renderer->shader);
        Shader_set_int(self->renderer->shader, "u_gbuffer_view", 4);
        Shader_end(self->renderer->shader);
        Sprite2D_render(self->renderer, depth, x4 - w-w, y1, w, h, 0.0f, (Vec4f){1, 1, 1, 1}, false);

        Shader_begin(self->renderer->shader);
        Shader_set_bool(self->renderer->shader, "u_use_gbuffer", false);
        Shader_end(self->renderer->shader);
    }

    // Рисуем текст:
    Vec2f text_pos = {16, 16};
    FontPixmap_set_scale_factor(font, (Vec2f){0.5f, 0.5f});
    FontPixmap_set_align(font, FONT_ALIGN_BOTTOM_LEFT);
    FontPixmap_set_color(font, (Vec4f){1, 1, 1, 1});
    FontPixmap_set_bg_color(font, (Vec4f){0, 0, 0, 0.5f});
    FontPixmap_set_bg_padding(font, (Vec4f){8, 8, 8, 8});
    FontPixmap_set_line_height(font, 16.0f);

    if (true) {
        FontPixmap_render(font, 8 + x1 - w, 8 + y1, 0, "Albedo/Roughness");
        FontPixmap_render(font, 8 + x2 - w, 8 + y2, 0, "Normal+AO");
        FontPixmap_render(font, 8 + x3 - w, 8 + y3, 0, "PBR (R-Metall, G-Height, B-Aberration, A-Distortion)");
        FontPixmap_render(font, 8 + x4 - w, 8 + y4, 0, "Emissive");
        FontPixmap_render(font, 8 + x4 - w-w, 8 + y1, 0, "Depth");
    }

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
    if (text_en) {
    FontPixmap_render(font, text_pos.x, text_pos.y, 0,
        "CPU:\n"
        "%s [%s]\n"
        "Threads: %d\n\n"
        "GPU:\n"
        "%s\n"
        "OpenGL %s\n"
        "Renderer draw calls: %zu\n"
        "USED: %.2f MB\n"
        "FREE: %.2f MB\n"
        "TOTAL: %.2f MB\n\n"
        "RAM:\n"
        "USED: %.2f MB\n"
        "FREE: %.2f MB\n"
        "TOTAL: %.2f MB\n\n"
        "Memory Manager:\n"
        "Used: %.2f MB\n"
        "Mesh used: %.2f MB\n"
        "GBuffer Textures used: %.2f MB\n\n"
        "FPS: %.2f\n"
        "CamPos: %.2f %.2f %.2f\n"
        "CamFov: %.2f\n",
        cpu_info.model,
        Info_get_cpu_arch_name(cpu_info.arch),
        cpu_info.threads,
        Renderer_get_renderer(self->renderer),
        Renderer_get_version(self->renderer),
        Renderer_get_draw_calls_count(self->renderer),
        Renderer_get_used_memory(self->renderer) / 1024.0,
        Renderer_get_free_memory(self->renderer) / 1024.0,
        Renderer_get_total_memory(self->renderer) / 1024.0,
        (double)mem_info.used / 1024.0 / 1024.0,
        (double)mem_info.free / 1024.0 / 1024.0,
        (double)mem_info.total / 1024.0 / 1024.0,
        mm_get_used_size_mb(),
        (double)(calculate_mesh_size(&objfile) + calculate_mesh_size(&objfile2)) / 1024.0f / 1024.0f,
        (double)(Texture_get_size(albedo_roughness)+Texture_get_size(normal_ao)+Texture_get_size(pbr_properties)+Texture_get_size(emissive)+Texture_get_size(depth)) / 1024.0 / 1024.0,
        fps, camera3d->position.x, camera3d->position.y, camera3d->position.z, camera3d->fov
    );
    }

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
