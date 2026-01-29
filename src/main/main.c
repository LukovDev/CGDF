//
// main.c - Основной файл программы.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>


Camera2D *camera;


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

    int width = Window_get_width(self);
    int height = Window_get_height(self);
    camera = Camera2D_create(self, width, height, (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);
}


// Вызывается каждый кадр (цикл окна):
void update(Window *self, Input *input, float dtime) {
    if (Window_get_is_focused(self)) {
        Window_set_fps(self, 60.0f);
    }
    if (Window_get_is_defocused(self)) {
        Window_set_fps(self, 10.0f);
    }

    Camera2D_update(camera);
}


// Вызывается каждый кадр (отрисовка окна):
void render(Window *self, Input *input, float dtime) {
    Window_clear(self, 0.0f, 0.0f, 0.0f);

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
