//
// test.c - Предназначен для тестирования во время разработки фреймворка.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>
#include <cgdf/graphics/opengl/texunit.h>


Texture *tex1;
Texture *tex2;
CameraController2D *ctrl;
Camera2D *camera;
Sprite2D *sprite;
SpriteBatch *batch;
SimpleDraw *draw;


void print_before_free() {
    printf("(Before free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
}


void print_after_free() {
    printf("(After free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
    if (mm_get_used_size() > 0) printf("Memory leak!\n");
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

    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/logo/CGDF2x2.png", true);

    tex2 = Texture_create(self->renderer);
    Texture_load(tex2, "data/textures/gradient_uv_checker.png", true);

    sprite = Sprite2D_create(self->renderer, tex2, -100.0f, -100.0f, 10.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, false);

    batch = SpriteBatch_create(self->renderer);

    draw = SimpleDraw_create(self->renderer);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, float dtime) {
    // if (Window_get_is_focused(self)) {
    //     Window_set_fps(self, 60.0f);
    // }
    // if (Window_get_is_defocused(self)) {
    //     Window_set_fps(self, 10.0f);
    // }

    CameraController2D_update(ctrl, dtime, false);
    Camera2D_update(camera);
}


// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    Vec2i mouse_pos = Input_get_mouse_pos(self);
    Vec2d globpos = local_to_global_2d(camera, mouse_pos);

    static bool enable = true;
    if (Input_get_key_down(self)[K_1]) enable = !enable;
    if (enable) {
        SpriteBatch_begin(batch);
        for (int y=0; y < 16; y++) {
            for (int x=0; x < 16; x++) {
                SpriteBatch_draw(batch, tex1, x, y, 1.0f, 1.0f, 0.0f);
            }
        }
        SpriteBatch_end(batch);
        // 30 fps стабильно, ура!!!
    }

    Sprite2D_render(self->renderer, tex1, globpos.x-0.5f, globpos.y-0.5f, 1.0f, 1.0f, 0.0f, (Vec4f){1, 1, 1, 1}, false);

    sprite->render(sprite);

    // Нарисовать точку:
    SimpleDraw_point(draw, (Vec4f){0, 1, 0, 1}, (Vec3f){0, 2, 0}, 16.0f);

    // Нарисовать точки:
    SimpleDraw_points(draw, (Vec4f){1, 0, 0, 1}, (Vec3f[]){
        {0, 0, 0}, {1, 0.5, 0}, {2, 1, 0}, {1, 2, 0}
    }, 4, 8.0f);

    // Нарисовать линию:
    SimpleDraw_line(draw, (Vec4f){1, 1, 0, 1}, (Vec3f){-3, -2, 0}, (Vec3f){3, 2, 0}, 1.0f);

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

    Window_display(self);
}


// Вызывается при изменении размера окна:
void resize(Window *self, int width, int height) {
    printf("Resize called.\n");
    Camera2D_resize(camera, width, height);
}


// Вызывается при разворачивании окна:
void show(Window *self) {
    printf("Show called.\n");
}


// Вызывается при сворачивании окна:
void hide(Window *self) {
    printf("Hide called.\n");
}


// Вызывается при закрытии окна:
void destroy(Window *self) {
    printf("Destroy called.\n");
    Camera2D_destroy(&camera);
    CameraController2D_destroy(&ctrl);
    Texture_destroy(&tex1);
    Texture_destroy(&tex2);
    Sprite2D_destroy(&sprite);
    SpriteBatch_destroy(&batch);
    SimpleDraw_destroy(&draw);
}


// Точка входа в программу:
int main(int argc, char *argv[]) {
    CGDF_Init();

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("[I] CGDF version: %s\n", cgdf_version);

    WinConfig *config = Window_create_config(start, update, render, resize, show, hide, destroy);
    Window *window = Window_create(config);
    if (!Window_open(window, 3, 3)) {
        log_msg("Window creation failed.\n");
    }

    print_before_free();

    Window_destroy_config(&config);
    Window_destroy(&window);

    print_after_free();

    return 0;
}
