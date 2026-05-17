//
// pixmap.h - Код для работы с растровыми картинками.
//

#pragma once


// Подключаем:
#include "std.h"


// Количество каналов в пикселе:
typedef enum PixmapFormat {
    PIXMAP_R = 1,
    PIXMAP_RG,
    PIXMAP_RGB,
    PIXMAP_RGBA
} PixmapFormat;


// Определяем глобальные переменные стандартной картинки:
extern const unsigned char g_Pixmap_default_icon[];
extern const size_t g_Pixmap_default_icon_size;
extern const int g_Pixmap_default_icon_width;
extern const int g_Pixmap_default_icon_height;


// Объявление структур:
typedef struct Pixmap Pixmap;  // Картинка.


// Структура картинки:
struct Pixmap {
    int width;       // Ширина картинки.
    int height;      // Высота картинки.
    int channels;    // Количество байт на пиксель.
    bool from_stbi;  // Флаг, что картинка загружена с помощью stbi.
    bool is_hdr;     // Флаг, что картинка HDR.
    void *data;      // Указатель на блок данных (либо unsigned char* либо float*).
};


// Создать картинку:
Pixmap* Pixmap_create(int width, int height, int channels);

// Уничтожить картинку:
void Pixmap_destroy(Pixmap **pixmap);

// Загрузить картинку:
Pixmap* Pixmap_load(const char *filepath, int channels);

// Сохранить картинку:
bool Pixmap_save(Pixmap *self, const char *filepath, const char *format);

// Копировать картинку в памяти:
Pixmap* Pixmap_copy(const Pixmap *source);

// Создать стандартную картинку:
Pixmap* Pixmap_create_default(void);

// Получить размер картинки в байтах:
size_t Pixmap_get_size(Pixmap *self);
