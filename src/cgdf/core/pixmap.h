//
// pixmap.h
//

#pragma once


// Подключаем:
#include "std.h"


// Размеры каналов в байтах:
#define PIXMAP_R    1
#define PIXMAP_RG   2
#define PIXMAP_RGB  3
#define PIXMAP_RGBA 4


// Определяем глобальные переменные стандартной картинки:
extern const unsigned char Pixmap_default_icon[];
extern const size_t Pixmap_default_icon_size;
extern const int Pixmap_default_icon_width;
extern const int Pixmap_default_icon_height;


// Объявление структур:
typedef struct Pixmap Pixmap;  // Картинка.


// Структура картинки:
struct Pixmap {
    int width;
    int height;
    int channels;
    bool from_stbi;
    unsigned char* data;
};


// Создать картинку:
Pixmap* Pixmap_create(int width, int height, int channels);

// Уничтожить картинку:
void Pixmap_destroy(Pixmap **pixmap);

// Загрузить картинку:
Pixmap* Pixmap_load(const char *filepath, int format);

// Сохранить картинку:
bool Pixmap_save(Pixmap *pixmap, const char *filepath, const char *format);

// Копировать картинку в памяти:
Pixmap* Pixmap_copy(const Pixmap *source);

// Создать стандартную картинку:
Pixmap* Pixmap_create_default();

// Получить размер картинки в байтах:
size_t Pixmap_get_size(Pixmap *pixmap);
