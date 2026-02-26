//
// main.c - Основной файл программы.
//


// Подключаем:
#include <cgdf/cgdf.h>               // Подключаем ядро и основные базовые вещи (без модулей).
#include <cgdf/graphics/graphics.h>  // Графика, и другие модули подключаются отдельно.


// Объявялем ресурсы:
static Texture *tex1;
static Camera2D *camera;
static Sprite2D *sprite;


// Вызывается после создания окна:
void Main_start(Window *self) {
    // Загружаем и устанавливаем иконку:
    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Pixmap_destroy(&icon);

    // Создаём камеру:
    int width, height;
    Window_get_size(self, &width, &height);
    camera = Camera2D_create(self, width, height, (Vec2d){0.0f, 0.0f}, 0.0f, 1.0f);

    // Создаём текстуру и загружаем в неё данные:
    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/logo/CGDF2x2.png", true);

    // Создаём спрайт:
    sprite = Sprite2D_create(
        self->renderer, tex1,             // Рендерер и текстура спрайта.
        0.0f, 0.0f, 100.0f, 100.0f,       // Позиция и размер (x, y, w, h).
        0.0f, (Vec4f){1, 1, 1, 1}, false  // Угол поворота, цвет и кастомный шейдер.
    );
}


// Вызывается при закрытии окна:
void Main_destroy(Window *self) {
    // Тут мы уничтожаем все объекты, что создали.
    Camera2D_destroy(&camera);
    Texture_destroy(&tex1);
    Sprite2D_destroy(&sprite);
}


// Вызывается каждый кадр (цикл окна):
void Main_update(Window *self, float dtime) {

    // Пример перемещения камеры:
    float speed = 4.0f;
    bool *keys = Input_get_key_pressed(self);
    if (keys[K_w]) camera->position.y += 100.0f * speed * dtime;
    if (keys[K_a]) camera->position.x -= 100.0f * speed * dtime;
    if (keys[K_s]) camera->position.y -= 100.0f * speed * dtime;
    if (keys[K_d]) camera->position.x += 100.0f * speed * dtime;

    // Обновляем камеру (применяем её параметры):
    Camera2D_update(camera);
}


// Вызывается каждый кадр (отрисовка окна):
void Main_render(Window *self, float dtime) {
    // Очищаем содержимое окна:
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    // Рисуем спрайт:
    sprite->render(sprite);

    // Также можно рисовать спрайт, не создавая отдельный объект спрайта:
    float angle = sinf(Window_get_time(self)) * 180.0f;
    Sprite2D_render(self->renderer, tex1, 100.0f, 100.0f, 50.0f, 50.0f, angle, (Vec4f){1, 1, 1, 1}, false);

    // Обновляем содержимое окна:
    Window_display(self);
}


// Вызывается при изменении размера окна:
void Main_resize(Window *self, int width, int height) {
    Camera2D_resize(camera, width, height);  // Масштабируем камеру под новый размер окна.
}


// Вызывается при разворачивании окна:
void Main_show(Window *self) {
    // Логика при разворачивании окна.
}


// Вызывается при сворачивании окна:
void Main_hide(Window *self) {
    // Логика при скрытии окна.
}


// Наша сцена:
// Это просто структура с нашими функциями, которые будут вызываться нашим окном.
// Можно указать NULL вместо функции, чтобы она не вызывалась, но лучше оставить.
WindowScene MainScene = {
    .start   = Main_start,
    .destroy = Main_destroy,
    .update  = Main_update,
    .render  = Main_render,
    .resize  = Main_resize,
    .show    = Main_show,
    .hide    = Main_hide
};


// Точка входа в программу:
int main(int argc, char *argv[]) {
    // В первую очередь, нам надо инициализировать библиотеку:
    if (!CGDF_Init()) {
        printf("CGDF initialization failed.\n");
        return 1;
    }

    // Также можно узнать версию библиотеки:
    printf("CGDF version: %s\n", CGDF_GetVersion());

    // Теперь нам надо создать конфигурацию окна (настройки окна):
    // При создании, надо передать нашу сцену.
    WinConfig *config = Window_create_config(MainScene);

    // Конфигурацию можно настраивать, меняя её поля:
    config->title = "Hello my First Game!";
    config->width = 960;
    config->height = 540;
    // Есть и другие параметры. Вы можете посмотреть по подсказкам,
    // или перейти к определению этой структуры.

    // Создаём окно. Оно принимает нашу конфигурацию для настройки:
    Window *window = Window_create(config);

    // Мы создали просто объект окна, но его ещё надо открыть:
    // Открываем окно по примеру ниже. Функция возвращает false,
    // если открыть окно не удалось. Это надо обработать, иначе
    // могут быть проблемы. Возвращает true, после того как окно
    // было создано, главный цикл отработал и завершился успешно.
    // Пометка: Управление окном и графикой, варьируется в зависимости
    // от выбранной реализации графики (opengl, vulkan, ...).
    // Это значит, что некоторые функции могут немного отличаться.
    // Конкретно при opengl, мы передаём объект окна, старшую версию
    // и младшую версию OpenGL (минимум 3.3!):
    if (!Window_open(window, 3, 3)) {
        printf("Window creation failed.\n");
    }

    // В случае отрабатывания функции Window_open(), последующий код
    // выполняется уже после закрытия окна. Это значит, что нам теперь
    // надо удалить созданные нами объекты, чтобы не было утечек памяти:
    Window_destroy(&window);
    Window_destroy_config(&config);

    // Необязательно, но мы можем посмотреть, есть ли утечка памяти после освобождения ресурсов:
    size_t used_size = mm_get_used_size();         // Сколько байтов всё ещё используется.
    size_t allocated = mm_get_allocated_blocks();  // Всё ещё существующие блоки аллокаций.

    printf("\nMemory info:\n");
    printf("Blocks allocated: %zu\n", allocated);
    printf("Used: %zu kb (%zu b).\n", (size_t)(used_size / 1024), used_size);

    // Если используется больше 0 байтов, значит есть какая то утечка, либо мы забыли что-то освободить:
    if (used_size > 0) {
        printf("Memory leak!\n");
        return 1;
    }

    return 0;
}
