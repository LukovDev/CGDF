//
// main.c - Основной файл программы.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>


static const float f = 1.0f;
static const Vertex quad_vertices[] = {
    {-f, -f, 0,  0,0,0,  1,1,1,  0,1}, // 0
    {+f, -f, 0,  0,0,0,  1,1,1,  1,1}, // 1
    {+f, +f, 0,  0,0,0,  1,1,1,  1,0}, // 2
    {-f, +f, 0,  0,0,0,  1,1,1,  0,0}  // 3
};
static const uint32_t quad_indices[] = {
    0, 1, 2,
    2, 3, 0
};

Texture *tex1;
CameraController2D *ctrl;
Camera2D *camera;
Mesh *mesh;


void print_before_free() {
    printf("(Before free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_total_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
}


void print_after_free() {
    printf("(After free) MM used: %g kb (%zu b). Blocks allocated: %zu. Absolute: %zu b. BlockHeaderSize: %zu b.\n",
            mm_get_used_size_kb(), mm_get_used_size(), mm_get_total_allocated_blocks(), mm_get_absolute_used_size(),
            mm_get_block_header_size());
    if (mm_get_used_size() > 0) printf("Memory leak!\n");
}


// Вызывается после создания окна:
void start(Window *self) {
    printf("Start called.\n");
    Window_set_fps(self, 60);
    Window_set_vsync(self, false);

    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Pixmap_destroy(&icon);

    int width = Window_get_width(self);
    int height = Window_get_height(self);
    camera = Camera2D_create(self, width, height, (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);
    Camera2D_set_meter(camera, 1.0f);

    ctrl = CameraController2D_create(self, camera, 1.0f, 0.001f, 128000.0f, 0.2f);

    mesh = Mesh_create(
        quad_vertices, sizeof(quad_vertices) / sizeof(Vertex),
        quad_indices, sizeof(quad_indices) / sizeof(uint32_t),
        false
    );

    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/logo/CGDF2x2.png", true);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, Input *input, float dtime) {
    if (Window_get_is_focused(self)) {
        Window_set_fps(self, 60.0f);
    }
    if (Window_get_is_defocused(self)) {
        Window_set_fps(self, 10.0f);
    }

    CameraController2D_update(ctrl, dtime, false);
    Camera2D_update(camera);
}


// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, Input *input, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);
    Shader_set_bool(self->renderer->shader, "u_use_texture", true);
    Shader_set_tex2d(self->renderer->shader, "u_texture", tex1->id);
    Mesh_render(mesh, false);
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
    Mesh_destroy(&mesh);
    CameraController2D_destroy(&ctrl);
    Texture_destroy(&tex1);
}


// Точка входа в программу:
int main(int argc, char *argv[]) {
    CGDF_Init();

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("CGDF version: %s\n", cgdf_version);

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
