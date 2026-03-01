//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>
#include <cgdf/graphics/opengl/texunit.h>
#include "game.h"


static Texture *tex1;
static Texture *tex2;
static Texture *light;
static Texture *flash_light;
static CameraController2D *ctrl;
static CameraController3D *ctrl3d;
static Camera2D *camera;
static Camera3D *camera3d;
static Sprite2D *sprite;
static SpriteBatch2D *batch;
static SimpleDraw *draw;
static Texture *anim[18];
static Texture *animation;
static FrameAnimator2D *anim2d;
static Light2D *light2d;


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


// Вызывается после создания окна:
void start(Window *self) {
    printf("Start called.\n");
    Window_set_fps(self, 0);
    Window_set_vsync(self, false);

    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Window_set_title(self, "CGDF Window");
    Pixmap_destroy(&icon);

    int width = Window_get_width(self);
    int height = Window_get_height(self);
    camera = Camera2D_create(self, width, height, (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);
    Camera2D_set_meter(camera, 1.0f);

    ctrl = CameraController2D_create(self, camera, 1.0f, 0.001f, 128000.0f, 0.9f);

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

    printf("Loading data...\n");
    for (int i=0; i < 18; i++) {
        anim[i] = Texture_create(self->renderer);
        char path[256];
        snprintf(path, sizeof(path), "data/animation/%d.png", i+1);
        Texture_load(anim[i], path, false);
        Texture_set_pixelized(anim[i]);
    }
    animation = Texture_create(self->renderer);
    Texture_load(animation, "data/animation/animation.png", false);
    Texture_set_pixelized(animation);
    anim2d = FrameAnimator2D_create(18, 1.0f/18.0f);

    tex1 = Texture_create(self->renderer);
    // Texture_load(tex1, "data/logo/CGDF2x2.png", true);
    Texture_load(tex1, "data/textures/snow.png", true);
    Texture_set_pixelized(tex1);

    tex2 = Texture_create(self->renderer);
    Texture_load(tex2, "data/textures/gradient_uv_checker.png", true);

    sprite = Sprite2D_create(self->renderer, tex2, -10.0f, -10.0f, 10.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, false);

    batch = SpriteBatch2D_create(self->renderer);

    draw = SimpleDraw_create(self->renderer);

    light2d = Light2D_create(self->renderer, (Vec3f){0.0, 0.0, 0.0}, 1.0f);

    light = Texture_create(self->renderer);
    Texture_load(light, "data/textures/light.png", true);

    flash_light = Texture_create(self->renderer);
    Texture_load(flash_light, "data/textures/flash-light.png", true);
    printf("data loaded\n");
}


// Вызывается при закрытии окна:
void destroy(Window *self) {
    printf("Destroy called.\n");
    Camera2D_destroy(&camera);
    CameraController2D_destroy(&ctrl);
    Texture_destroy(&tex1);
    Texture_destroy(&tex2);
    Texture_destroy(&light);
    Texture_destroy(&flash_light);
    Sprite2D_destroy(&sprite);
    SpriteBatch2D_destroy(&batch);
    SimpleDraw_destroy(&draw);

    Camera3D_destroy(&camera3d);
    CameraController3D_destroy(&ctrl3d);
    FrameAnimator2D_destroy(&anim2d);
    for (int i=0; i < 18; i++) {
        Texture_destroy(&anim[i]);
    }
    Texture_destroy(&animation);

    Light2D_destroy(&light2d);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, float dtime) {
    // if (Window_get_is_focused(self)) {
    //     Window_set_fps(self, 60.0f);
    // }
    // if (Window_get_is_defocused(self)) {
    //     Window_set_fps(self, 10.0f);
    // }

    if (Input_get_key_down(self)[K_0]) {
        Window_set_scene(self, GameScene);
        return;
    }

    static bool cam2d = true;
    if (Input_get_key_down(self)[K_2]) cam2d = !cam2d;
    if (cam2d) {
        CameraController2D_update(ctrl, dtime, false);
        Camera2D_update(camera);
        ctrl3d->target_pos.x = ctrl->target_pos.x;
        ctrl3d->target_pos.y = ctrl->target_pos.y;
        camera3d->position = ctrl3d->target_pos;
    } else {
        CameraController3D_update(ctrl3d, dtime, false);
        Camera3D_update(camera3d);
        ctrl->target_pos.x = ctrl3d->target_pos.x;
        ctrl->target_pos.y = ctrl3d->target_pos.y;
        camera->position = ctrl->target_pos;
    }
    float itensity = sinf(Window_get_time(self)/2.0f) * 0.5f + 0.5f;
    float ambient = 1.0 - itensity;
    Light2D_set_intensity(light2d, itensity);
    Light2D_set_ambient(light2d, (Vec3f){ambient, ambient, ambient});
}


// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    Light2D_scene_begin(light2d);
    Vec2i mouse_pos = Input_get_mouse_pos(self);
    Vec2d globpos = {0};

    static Vec3f hit_pos;
    Vec3f plane_point  = {0.0f, 0.0f, 0.0f};
    Vec3f plane_normal = {0.0f, 0.0f, 1.0f};

    mat4 view, proj;
    Renderer_get_view_proj(self->renderer, view, proj);
    if (camera_screen_to_plane(self, view, proj, mouse_pos, plane_point, plane_normal, &hit_pos)) {
        globpos.x = hit_pos.x;
        globpos.y = hit_pos.y;
    }

    static bool enable = true;
    if (Input_get_key_down(self)[K_1]) enable = !enable;
    if (enable) {
        SpriteBatch2D_begin(batch);
        int size = 32;
        for (int y=-size/2; y < size/2; y++) {
            for (int x=-size/2; x < size/2; x++) {
                // float delta = FrameAnimator2D_get_frame(anim2d) / 18.0f;
                // SpriteBatch2D_set_texcoord(batch, (Vec4f){
                //     delta, 0.0f,
                //     delta+1.0f/18.0f, 1.0f
                // });
                SpriteBatch2D_draw(batch, tex1, x, y, 1.0f, 1.0f, 0.0f);
            }
        }
        SpriteBatch2D_reset_texcoord(batch);
        SpriteBatch2D_end(batch);
    }

    FrameAnimator2D_update(anim2d, dtime);
    Sprite2D_render(self->renderer, anim[FrameAnimator2D_get_frame(anim2d)], globpos.x-0.5f, globpos.y-0.5f, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, false);

    Light2D_scene_end(light2d);

    Light2D_light_begin(light2d);
    for (int i=0; i < 10; i++) {
        float t = (float)i / 10.0f; // 0..1 по всем 10 спрайтам

        float r, g, b;
        if (t < 0.5f) {
            float k = t * 2.0f;    // 0..1
            r = 1.0f - k;          // red -> 0
            g = k;                 // 0 -> green
            b = 0.0f;
        } else {
            float k = (t - 0.5f) * 2.0f; // 0..1
            r = 0.0f;
            g = 1.0f - k;                // green -> 0
            b = k;                       // 0 -> blue
        }

        Vec4f color = (Vec4f){ r, g, b, 2.0f };
        Vec2f pos = (Vec2f){ 
            sinf(i*2.0f*GLM_PI/10.0f) * 10.0f - 5,
            cosf(i*2.0f*GLM_PI/10.0f) * 10.0f - 5
        };
        Sprite2D_render(self->renderer, light, pos.x, pos.y, 10.0f, 10.0f, t*360.0f, color, false);
    }

    sprite->render(sprite);
    float intensity = 2.0f;
    Sprite2D_render(self->renderer, light, globpos.x-5, globpos.y-5, 10.0f, 10.0f, 0.0f, (Vec4f){1, 1, 1, intensity}, false);
    // Sprite2D_render(self->renderer, light, -10, -5, 10.0f, 10.0f, 0.0f, (Vec4f){0, 0, 1, intensity}, false);
    // Sprite2D_render(self->renderer, light, -5, -5, 10.0f, 10.0f, 0.0f, (Vec4f){1, 0, 0, intensity}, false);

    // Нарисовать точку:
    SimpleDraw_point(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){0, 2, 0}, 16.0f);

    // Нарисовать точки:
    SimpleDraw_points(draw, (Vec4f){1, 0, 0, 1}, (Vec3f[]){
        {0, 0, 0}, {1, 0.5, 0}, {2, 1, 0}, {1, 2, 0}
    }, 4, 8.0f);

    // Нарисовать линию:
    SimpleDraw_line(draw, (Vec4f){1, 1, 0, 1}, (Vec3f){-3, -2, 0}, (Vec3f){3, 2, 0}, 1.0f);

    // Нарисовать линии:
    SimpleDraw_lines(draw, (Vec4f){0, 0, 1, 1}, (Vec3f[]){
        {0, 0, 100}, {0, 0, -100}, {100, 0, 0}, {-100, 0, 0}, {0, 100, 0}, {0, -100, 0}
    }, 6, 1.0f);

    // Нарисовать ломаную линию:
    SimpleDraw_line_strip(draw, (Vec4f){1, 0, 1, 1}, (Vec3f[]){
        {-2, 2, 0}, {-1, 0.5, 0}, {2, -1, 0}, {-1, -2, 0}
    }, 4, 8.0f);

    // Нарисовать замкнутую ломаную линию:
    SimpleDraw_line_loop(draw, (Vec4f){0, 1, 1, 1}, (Vec3f[]){
        {3, -1, 0}, {4, -1.5, 0}, {2, -1, 0}, {-2, 4, 0}
    }, 4, 3.0f);

    // Нарисовать треугольники:
    SimpleDraw_triangles(draw, (Vec4f){1, 1, 1, 1}, (Vec3f[]){
        {-1, -1, 0}, {1, -1, 0}, {0, 1, 0}
    }, 3);

    // Нарисовать треугольники последняя вершина которой будет соединена с первой:
    SimpleDraw_triangle_fan(draw, (Vec4f){1, 1, 1, 0.5}, (Vec3f[]){
        {-4, -4, 0}, {-3, -4, 0}, {0, 4, 0}, {1, 4, 0}
    }, 4);

    // Нарисовать квадрат:
    SimpleDraw_quad(draw, (Vec4f){1, 0, 1, 0.5}, (Vec3f){-4, 4, 0}, (Vec2f){2, 2}, 4.0f);

    // Нарисовать квадрат с заливкой:
    SimpleDraw_quad_fill(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){-1, 4, 0}, (Vec2f){2, 2});

    // Нарисовать круг:
    SimpleDraw_circle(draw, (Vec4f){1, 1, 0, 1}, (Vec3f){4, 4, 0}, 2.0f, 32, 1.0f);

    // Нарисовать круг с заливкой:
    SimpleDraw_circle_fill(draw, (Vec4f){1, 1, 0, 1}, (Vec3f){4, 0, 0}, 2.0f, 8);

    // Нарисовать звезду:
    SimpleDraw_star(draw, (Vec4f){0, 0, 1, 1}, (Vec3f){0, -4, 0}, 2.0f, 1.0f, 5, 1.0f);

    // Нарисовать звезду с заливкой:
    SimpleDraw_star_fill(draw, (Vec4f){1, 0, 0, 1}, (Vec3f){-4, 0, 0}, 2.0f, 1.0f, 5);

    Light2D_light_end(light2d);

    Light2D_scene_begin(light2d);
    Sprite2D_render(self->renderer, anim[FrameAnimator2D_get_frame(anim2d)], -1, 10, 2.0f, 2.0f, 0.0f, (Vec4f){1, 1, 1, 1}, false);
    Light2D_scene_end(light2d);

    // Рисуем освещение:
    Light2D_render(light2d);

    Window_display(self);
}


// Вызывается при изменении размера окна:
void resize(Window *self, int width, int height) {
    printf("Resize called.\n");
    Camera2D_resize(camera, width, height);
    Camera3D_resize(camera3d, width, height, false);
    Light2D_resize(light2d, width, height);
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
    config->gl_major = 4;
    config->gl_minor = 6;
    Window *window = Window_create(config);
    if (!Window_open(window, true)) {
        log_msg("[E] Window creation failed.\n");
    }

    print_before_free();

    Window_destroy(&window);
    Window_destroy_config(&config);

    print_after_free();

    return 0;
}
