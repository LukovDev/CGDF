//
// game.c - Основной файл игры.
//


// Подключаем:
#include <cgdf/cgdf.h>
#include <cgdf/graphics/graphics.h>


// Объявялем ресурсы:
static Texture *tex1;
static Camera2D *camera;
static Sprite2D *sprite;
static CameraController2D *cam2d_ctrl;
static SpriteBatch *batch;


static int map_width = 256;
static int map_height = 256;
static float map_tile_size = 1.0f;
static int *map_data = NULL;
static float *height_map_data = NULL;

typedef enum {
    TILE_EMPTY = 0,
    TILE_WATER = 1,
    TILE_COUNT
} TileType;

static inline int get_index(int x, int y) {
    return (y * map_width) + x;
}

// Функция плавного перехода (сглаживание)
static double fade(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Линейная интерполяция
static double lerp(double t, double a, double b) {
    return a + t * (b - a);
}

// Вычисление скалярного произведения расстояния и градиентного векторов
static double grad(int hash, double x, double y) {
    int h = hash & 3; // Использование младших 2 бит
    switch (h) {
        case 0: return  x + y; // Градиент (1, 1)
        case 1: return -x + y; // Градиент (-1, 1)
        case 2: return  x - y; // Градиент (1, -1)
        case 3: return -x - y; // Градиент (-1, -1)
        default: return 0.0;
    }
}

// Таблица перестановок Перлина (повторяется дважды)
static const int perm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
    125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
    105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
    135,130,116,188,159,86,164,100,119,198,32,161,91,142,150,67,141,128,210,116,195,191,220,44,249,158,
    // (Полная таблица содержит 256 значений, дублированных дважды для оптимизации)
    // Дополните массив остальными 256 значениями из оригинальной реализации Кена Перлина
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
    125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
    105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
    135,130,116,188,159,86,164,100,119,198,32,161,91,142,150,67,141,128,210,116,195,191,220,44,249,158
};

// Основная функция шума Перлина для координат x и y
double perlinNoise2D(double x, double y) {
    // Находим координаты сетки
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;

    // Получаем смещение внутри клетки
    x -= floor(x);
    y -= floor(y);

    // Вычисляем кривые затухания
    double u = fade(x);
    double v = fade(y);

    // Хеши углов сетки
    int A  = perm[X]     + Y;
    int AA = perm[A];
    int AB = perm[A + 1];
    int B  = perm[X + 1] + Y;
    int BA = perm[B];
    int BB = perm[B + 1];

    // Интерполяция 4 градиентов
    double result = lerp(v, lerp(u, grad(perm[AA], x, y), 
                                     grad(perm[BA], x - 1, y)),
                            lerp(u, grad(perm[AB], x, y - 1), 
                                     grad(perm[BB], x - 1, y - 1)));

    // Приводим результат из диапазона [-1, 1] в [0, 1]
    return (result + 1.0) / 2.0;
}

// Вызывается после создания окна:
static void start(Window *self) {
    // Загружаем и устанавливаем иконку:
    Pixmap *icon = Pixmap_load("data/logo/CGDF2x2.png", PIXMAP_RGBA);
    Window_set_icon(self, icon);
    Pixmap_destroy(&icon);
    Window_set_fps(self, 100);

    // Создаём камеру:
    int width, height;
    Window_get_size(self, &width, &height);
    camera = Camera2D_create(self, width, height, (Vec2d){map_width/2.0f, map_height/2.0f}, 0.0f, 25.0f);
    Camera2D_set_meter(camera, 1.0f);

    // Контроллер камеры:
    cam2d_ctrl = CameraController2D_create(self, camera, 1.0f, 0.01f, 100.0f, 0.9f);

    batch = SpriteBatch_create(self->renderer);

    // Создаём текстуру и загружаем в неё данные:
    tex1 = Texture_create(self->renderer);
    Texture_load(tex1, "data/textures/square.png", true);

    // Создаём спрайт:
    sprite = Sprite2D_create(
        self->renderer, tex1,             // Рендерер и текстура спрайта.
        0.0f, 0.0f, 100.0f, 100.0f,       // Позиция и размер (x, y, w, h).
        0.0f, (Vec4f){1, 1, 1, 1}, false  // Угол поворота, цвет и кастомный шейдер.
    );

    // Создаём карту:
    map_data = mm_alloc(map_width * map_height * sizeof(int));
    height_map_data = mm_alloc(map_width * map_height * sizeof(float));
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            map_data[get_index(x, y)] = ((x + y) % 2 == 0) ? TILE_EMPTY : TILE_WATER;
            height_map_data[get_index(x, y)] = perlinNoise2D((double)x * 0.01, (double)y * 0.01);
        }
    }
}


// Вызывается при закрытии окна:
static void destroy(Window *self) {
    // Тут мы уничтожаем все объекты, что создали.
    Camera2D_destroy(&camera);
    CameraController2D_destroy(&cam2d_ctrl);
    SpriteBatch_destroy(&batch);
    Texture_destroy(&tex1);
    Sprite2D_destroy(&sprite);
    mm_free(map_data);
    mm_free(height_map_data);
}


// Вызывается каждый кадр (цикл окна):
static void update(Window *self, float dtime) {

    static float regen_timer = 0.0f;
    static float next_regen_interval = 10.0f; // Начальный интервал (например, 20 секунд)

    // 1. Обновляем таймер регенерации
    regen_timer += dtime;

    if (regen_timer >= next_regen_interval) {
        regen_timer = 0.0f;

        // Перегенерируем воду по всей карте на основе шума Перлина
        double seed = Time_now(NULL);
        for (int y = 0; y < map_height; y++) {
            for (int x = 0; x < map_width; x++) {
                map_data[get_index(x, y)] = ((x + y) % 2 == 0) ? TILE_EMPTY : TILE_WATER;
                height_map_data[get_index(x, y)] = perlinNoise2D((double)x * 0.01 + seed, (double)y * 0.01 + seed);
            }
        }
        
        // Опционально: можно вывести лог в консоль для дебага, чтобы видеть, что таймер сработал
        // printf("Карта регенерирована! Следующий тик через: %.2f сек.\n", next_regen_interval);
    }

    // Внутри игрового цикла (Update):
    static float counter = 0.0f;
    const float tps = 10.0f;
    counter += dtime;

    if (counter >= 1.0f / tps) {
        counter = 0.0f;
        
        int* next_map_data = mm_alloc(map_width * map_height * sizeof(int));
        memcpy(next_map_data, map_data, map_width * map_height * sizeof(int));

        for (int y = 0; y < map_height; y++) {
            for (int x = 0; x < map_width; x++) {
                int current_idx = get_index(x, y);
                int tile = map_data[current_idx];
                
                if (tile == TILE_WATER) {
                    float current_height = height_map_data[current_idx];
                    int target_idx = current_idx;
                    
                    // Ищем максимальный уклон (крутизну спуска)
                    // Инициализируем нулем: течь имеет смысл только если уклон вниз (больше 0)
                    float max_slope = 0.0f; 

                    // Массивы смещений для 8 направлений (4 прямых + 4 диагональных)
                    int dx[] = {1, -1, 0, 0,   1, -1,  1, -1};
                    int dy[] = {0, 0, 1, -1,   1, -1, -1,  1};
                    
                    // Коэффициент расстояния: для первых 4-х (прямых) равен 1.0, для следующих 4-х (диагоналей) равен 1.414f
                    float distances[] = {1.0f, 1.0f, 1.0f, 1.0f,   1.414f, 1.414f, 1.414f, 1.414f};

                    for (int i = 0; i < 8; i++) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];

                        // Защита от выхода за границы массива
                        if (nx >= 0 && nx < map_width && ny >= 0 && ny < map_height) {
                            int neighbor_idx = get_index(nx, ny);
                            
                            // Проверяем, свободна ли целевая плитка от воды в следующем тике
                            if (next_map_data[neighbor_idx] != TILE_WATER) {
                                float neighbor_height = height_map_data[neighbor_idx];
                                
                                // Вычисляем перепад высот
                                float height_diff = current_height - neighbor_height;
                                
                                if (height_diff > 0.0f) {
                                    // Вычисляем крутизну склона с учетом расстояния до соседа
                                    float slope = height_diff / distances[i];
                                    
                                    if (slope > max_slope) {
                                        max_slope = slope;
                                        target_idx = neighbor_idx;
                                    }
                                }
                            }
                        }
                    }

                    // Если нашли плитку с уклоном вниз — перемещаем воду туда
                    if (target_idx != current_idx) {
                        next_map_data[target_idx] = TILE_WATER;
                        next_map_data[current_idx] = TILE_EMPTY;
                    }
                }
            }
        }

        memcpy(map_data, next_map_data, map_width * map_height * sizeof(int));
        mm_free(next_map_data);
    }


    // Пример перемещения камеры:
    CameraController2D_update(cam2d_ctrl, dtime, false);

    // Обновляем камеру (применяем её параметры):
    Camera2D_update(camera);
}


// Вызывается каждый кадр (отрисовка окна):
static void render(Window *self, float dtime) {
    // Очищаем содержимое окна:
    Window_clear(self, 0.0f, 0.0f, 0.0f);

    SpriteBatch_begin(batch);
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            int tile = map_data[get_index(x, y)];
            float height = height_map_data[get_index(x, y)];
            switch (tile) {
                case TILE_EMPTY: {
                    SpriteBatch_set_color(batch, (Vec4f){height, 0, 1 - height, 1});
                    SpriteBatch_draw(batch, tex1, x * map_tile_size, y * map_tile_size, map_tile_size, map_tile_size, 0.0f);
                    break;
                }
                
                case TILE_WATER: {
                    SpriteBatch_set_color(batch, (Vec4f){0, 0.25*height, 1*height, 1});
                    SpriteBatch_draw(batch, tex1, x * map_tile_size, y * map_tile_size, map_tile_size, map_tile_size, 0.0f);
                    break;
                }
            }
        }
    }
    SpriteBatch_end(batch);

    // Обновляем содержимое окна:
    Window_display(self);
}


// Вызывается при изменении размера окна:
static void resize(Window *self, int width, int height) {
    Camera2D_resize(camera, width, height);  // Масштабируем камеру под новый размер окна.
}


// Вызывается при разворачивании окна:
static void show(Window *self) {
    // Логика при разворачивании окна.
}


// Вызывается при сворачивании окна:
static void hide(Window *self) {
    // Логика при скрытии окна.
}


// Наша сцена:
// Это просто структура с нашими функциями, которые будут вызываться нашим окном.
// Можно указать NULL вместо функции, чтобы она не вызывалась, но лучше оставить.
WindowScene MainScene = {
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
    // В первую очередь, нам надо инициализировать фреймворк:
    if (!CGDF_init()) {
        printf("CGDF initialization failed.\n");
        return 1;
    }

    // Также можно узнать версию фреймворка:
    printf("CGDF version: %s\n", CGDF_GetVersion());

    // Теперь нам надо создать конфигурацию окна (настройки окна):
    // При создании, надо передать нашу сцену.
    WinConfig *config = Window_create_config(&MainScene);

    // Конфигурацию можно настраивать, меняя её поля:
    config->title = "My first game!";
    config->width = 960;
    config->height = 540;
    // Пометка: Управление окном и графикой, варьируется в зависимости
    // от выбранной реализации графики (opengl, vulkan, ...).
    // Это значит, что некоторые функции могут немного отличаться.
    // Мы зададим версию OpenGL (3.3 самая минимальная в этом фреймворке):
    config->gl_major = 3;  // Старшая версия.
    config->gl_minor = 3;  // Младшая версия.
    // Есть и другие параметры. Вы можете посмотреть по подсказкам,
    // или перейти к определению этой структуры.

    // Создаём окно. Оно принимает нашу конфигурацию для настройки:
    Window *window = Window_create(config);

    // Мы создали просто объект окна, но его ещё надо открыть:
    // Открываем окно по примеру ниже. Функция возвращает false,
    // если открыть окно не удалось. Это надо обработать, иначе
    // могут быть проблемы. Возвращает true, после того как окно
    // было создано, главный цикл отработал и завершился успешно.
    if (!Window_open(window)) {
        printf("Window creation failed.\n");
    }

    // В случае отрабатывания функции Window_open(), последующий код
    // выполняется уже после закрытия окна. Это значит, что нам теперь
    // надо удалить созданные нами объекты, чтобы не было утечек памяти:
    Window_destroy(&window);
    Window_destroy_config(&config);

    // Перед завершением программы, надо освободить внутренние системы фреймворка:
    CGDF_destroy();

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

    return EXIT_SUCCESS;
}
