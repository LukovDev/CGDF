//
// font.c - Реализация функционала для рендеринга текста.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/libs.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include <cgdf/core/hashtable.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/files.h>
#include <cgdf/core/logger.h>
#include "renderer.h"
#include "texture.h"
#include "spritebatch.h"
#include "font.h"


// -------- Вспомогательные функции: --------


// Проверка пути файла шрифта. Если его нет, используем системный шрифт:
static char* check_file_path(const char* file_path) {
    // Сначала пользовательский путь:
    if (file_path && file_path[0] != '\0') {
        FILE *f = fopen(file_path, "rb");
        if (f) {
            fclose(f);
            char *path = mm_alloc(strlen(file_path) + 1);
            strcpy(path, file_path);
            return path;
        }
    }

    // Иначе пробуем системный путь:
    const char *candidates[16];
    int n = 0;
    #ifdef _WIN32
        const char *windir = getenv("WINDIR");
        if (!windir) windir = "C:\\Windows";

        static char p1[512], p2[512], p3[512], p4[512];
        snprintf(p1, sizeof(p1), "%s\\Fonts\\arial.ttf", windir);
        snprintf(p2, sizeof(p2), "%s\\Fonts\\segoeui.ttf", windir);
        snprintf(p3, sizeof(p3), "%s\\Fonts\\tahoma.ttf", windir);
        snprintf(p4, sizeof(p4), "%s\\Fonts\\calibri.ttf", windir);

        candidates[n++] = p1;
        candidates[n++] = p2;
        candidates[n++] = p3;
        candidates[n++] = p4;
    #elif defined(__APPLE__)
        candidates[n++] = "/System/Library/Fonts/SFNS.ttf";
        candidates[n++] = "/System/Library/Fonts/Supplemental/Arial.ttf";
        candidates[n++] = "/Library/Fonts/Arial.ttf";
    #else
        candidates[n++] = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        candidates[n++] = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
        candidates[n++] = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";

        char *home = Files_get_home();
        static char p5[512];
        if (home && home[0]) {
            snprintf(p5, sizeof(p5), "%s/.fonts/DejaVuSans.ttf", home);
            candidates[n++] = p5;
        }
    #endif

    // Перебираем кандидатов и проверяем, можем ли прочитать их:
    for (int i = 0; i < n; i++) {
        FILE *f = fopen(candidates[i], "rb");
        if (f) {
            fclose(f);
            char *path = mm_alloc(strlen(candidates[i]) + 1);
            strcpy(path, candidates[i]);
            return path;
        }
    }
    return NULL;
}

// Простой декодер UTF-8 в кодовую точку Unicode:
static uint32_t utf8_decode(const char *symbol, int *advance) {
    if (!symbol || !advance) return 0;
    const unsigned char *p = (const unsigned char*)symbol;
    *advance = 1;

    // Один байт (ASCII):
    if (p[0] < 0x80) {
        return p[0];
    }

    // Два байта:
    if ((p[0] & 0xE0) == 0xC0) {
        if (p[1] == '\0' || (p[1] & 0xC0) != 0x80) return FONT_FALLBACK_SUMB;
        uint32_t cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        if (cp < 0x80) return FONT_FALLBACK_SUMB;
        *advance = 2;
        return cp;
    }

    // Три байта:
    if ((p[0] & 0xF0) == 0xE0) {
        if (p[1] == '\0' || p[2] == '\0') return FONT_FALLBACK_SUMB;
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return FONT_FALLBACK_SUMB;
        uint32_t cp = ((p[0] & 0x0F) << 12) |
                      ((p[1] & 0x3F) << 6 ) |
                      (p[2] & 0x3F);
        if (cp < 0x800) return FONT_FALLBACK_SUMB;
        if (cp >= 0xD800 && cp <= 0xDFFF) return FONT_FALLBACK_SUMB;
        *advance = 3;
        return cp;
    }

    // Четыре байта:
    if ((p[0] & 0xF8) == 0xF0) {
        if (p[1] == '\0' || p[2] == '\0' || p[3] == '\0') return FONT_FALLBACK_SUMB;
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return FONT_FALLBACK_SUMB;
        uint32_t cp = ((p[0] & 0x07) << 18) |
                      ((p[1] & 0x3F) << 12) |
                      ((p[2] & 0x3F) << 6 ) |
                      (p[3] & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return FONT_FALLBACK_SUMB;
        *advance = 4;
        return cp;
    }

    return FONT_FALLBACK_SUMB;
}

// Добавить глиф в хеш-таблицу:
static bool glyph_insert_to_cache(FontPixmap *self, uint32_t codepoint, FontGlyph *glyph) {
    uint32_t *stored_key = (uint32_t*)mm_alloc(sizeof(uint32_t));
    *stored_key = codepoint;
    if (!HashTable_set(self->glyphs, stored_key, sizeof(uint32_t), glyph, sizeof(FontGlyph*))) {
        mm_free(stored_key);
        mm_free(glyph);
        return false;
    }
    return true;
}

// Создать глиф и добавить в атлас:
static FontGlyph* generate_glyph(FontPixmap *self, uint32_t codepoint) {
    if (!self) return NULL;

    // Если нет такого глифа, то возвращаем FONT_FALLBACK_SUMB:
    if (stbtt_FindGlyphIndex(&self->info, codepoint) == 0) codepoint = FONT_FALLBACK_SUMB;

    // Получаем bitmap символа:
    int width, height, xoff, yoff;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(
        &self->info, 0, self->scale, codepoint, &width, &height, &xoff, &yoff
    );
    if (!bitmap) return NULL;

    // Конвертируем bitmap в RGBA8:
    unsigned char *rgba = mm_alloc(width * height * 4);
    for (int i = 0; i < width * height; i++) {
        rgba[i*4+0] = 255;
        rgba[i*4+1] = 255;
        rgba[i*4+2] = 255;
        rgba[i*4+3] = bitmap[i];  // Это альфа канал.
    }
    stbtt_FreeBitmap(bitmap, NULL);

    // Добавляем глиф в атлас:
    const int pad  = FONT_ATLAS_PADDING;
    const int cell = self->font_size + pad * 2;
    const int grid = self->atlas->width / cell;
    if (grid <= 0) { mm_free(rgba); return NULL; }

    int capacity = grid * grid;
    int index = self->added_glyphs_count;
    if (index >= capacity) { mm_free(rgba); return NULL; }  // Места нет.

    int atlas_x = (index % grid) * cell + pad;
    int atlas_y = (index / grid) * cell + pad;
    if (atlas_x + width  + pad > self->atlas->width || atlas_y + height + pad > self->atlas->height) {
        mm_free(rgba);
        return NULL;
    }

    // Загружаем bitmap в текстуру:
    // Данные вставляются от atlas_x, atlas_y и до atlas_x+width, atlas_y+height:
    Texture_set_subdata(self->atlas, 0, atlas_x, atlas_y, width, height, TEX_RGBA8, TEX_DATA_UBYTE, rgba);
    mm_free(rgba);  // Можно освободить. Больше не понадобится.

    // Получаем advance:
    int advance, lsb;
    stbtt_GetCodepointHMetrics(&self->info, codepoint, &advance, &lsb);
    advance = (int)(advance * self->scale);

    // Создаём глиф в куче:
    FontGlyph *glyph = (FontGlyph*)mm_alloc(sizeof(FontGlyph));
    glyph->width = width;
    glyph->height = height;
    glyph->offset_x = (float)xoff;
    glyph->offset_y = (float)yoff;
    glyph->advance = (float)advance;
    glyph->u0 = (float)atlas_x / (float)self->atlas->width;
    glyph->v0 = (float)atlas_y / (float)self->atlas->height;
    glyph->u1 = (float)(atlas_x + width) / (float)self->atlas->width;
    glyph->v1 = (float)(atlas_y + height) / (float)self->atlas->height;
    self->added_glyphs_count++;
    return glyph;
}

// Расширить атлас и перепаковать глифы:
static bool font_expand_and_repack(FontPixmap *self) {
    if (!self || !self->atlas) return false;

    // Получаем размеры и корректируем их до максимума:
    int old_size = self->atlas->width;
    int max_size = Renderer_get_max_texture_size(self->renderer);
    if (old_size >= max_size) return false;  // Крайне странная ситуация. Возвращаем что ничего не получилось.

    int new_size = ceil(old_size * FONT_ATLAS_SCALING);  // Увеличиваем во столько то раз.
    if (new_size > max_size) new_size = max_size;        // Ограничиваем максимумом.

    // Расширяем атлас:
    Texture_empty(self->atlas, new_size, new_size, false, TEX_RGBA8, TEX_DATA_UBYTE);
    HashTable_clear(self->glyphs, true);  // Удаляем старые key+glyph.
    self->added_glyphs_count = 0;         // Обнуляем счетчик добавленных глифов.

    // Заново генерируем глифы:
    for (size_t i = 0; i < Array_len(self->glyphs_array); i++) {
        uint32_t cp = *(uint32_t*)Array_get(self->glyphs_array, i);  // Получаем codepoint.
        FontGlyph *g = generate_glyph(self, cp);  // Генерируем глиф.
        if (!g || !glyph_insert_to_cache(self, cp, g)) {
            return false;  // Если глиф не создался или не кэшировался, то это плохо.
        }
    }
    return true;  // Победа! Мы смогли пересобрать атлас в новом размере.
}


// -------- API шрифта: --------


// Создать растровый шрифт:
FontPixmap* FontPixmap_create(Renderer *renderer, const char *file_path, int font_size) {
    if (!renderer) return NULL;

    // Проверяем путь до шрифта, иначе используем системный шрифт:
    char *font_path = check_file_path(file_path);
    if (!font_path) {
        log_msg("[E] FontPixmap_create: no valid font found (requested: %s).\n", file_path ? file_path : "<null>");
        return NULL;  // Без шрифта.
    }

    FontPixmap *font = (FontPixmap*)mm_alloc(sizeof(FontPixmap));

    // Загружаем шрифт и инициализируем его:
    font->ttf_buffer = Files_load_bin(font_path, "rb", NULL);
    stbtt_InitFont(&font->info, font->ttf_buffer, 0);
    mm_free(font_path);  // Освобождаем текст пути до файла.

    // Обрабатываем размер текста (от 1 до максимального размера текстуры):
    int min_size = 1;  // Минимальный размер текста - 1 пиксель.
    int max_size = Renderer_get_max_texture_size(renderer)-FONT_ATLAS_PADDING*2-1; // Чтобы хотя бы один глиф влез.
    if (max_size < min_size) max_size = min_size;
    font_size = glm_clamp(font_size, min_size, max_size);

    // Заполняем поля:
    font->renderer = renderer;
    font->atlas = Texture_create(renderer);
    font->batch = SpriteBatch_create(renderer);
    font->glyphs_array = Array_create(sizeof(uint32_t), FONT_ATLAS_SIZE);
    font->glyphs = HashTable_create();
    font->color = (Vec4f){1.0f, 1.0f, 1.0f, 1.0f};
    font->pixelized = false;  // Чисто технически, нужен как флаг-кэш для включения/отключения пикселизации один раз.
    font->added_glyphs_count = 0;

    // Метрики которые нельзя редактировать:
    font->font_size = font_size;
    font->scale = stbtt_ScaleForPixelHeight(&font->info, font_size);
    stbtt_GetFontVMetrics(&font->info, &font->ascent, &font->descent, &font->line_gap);
    font->line_advance = (font->ascent - font->descent + font->line_gap) * font->scale;

    // Метрики которые можно редактировать:
    font->line_height = 0.0f;
    font->letter_spacing = 0.0f;
    font->tab_size = 4;  // 4 пробела по умолчанию.

    // Получаем размер пробела:
    int sp_adv = 0, sp_lsb = 0;
    stbtt_GetCodepointHMetrics(&font->info, ' ', &sp_adv, &sp_lsb);
    font->space_advance = sp_adv * font->scale;

    // Инициализируем атлас:
    int atlas_size = (int)sqrt(FONT_ATLAS_SIZE) * (font->font_size + FONT_ATLAS_PADDING * 2);
    if (atlas_size > max_size) atlas_size = max_size;  // Ограничиваем максимумом.
    Texture_empty(font->atlas, atlas_size, atlas_size, false, TEX_RGBA8, TEX_DATA_UBYTE);
    return font;
}

// Уничтожить растровый шрифт:
void FontPixmap_destroy(FontPixmap **font) {
    if (!font || !*font) return;

    // Уничтожаем объекты:
    Texture_destroy(&(*font)->atlas);
    SpriteBatch_destroy(&(*font)->batch);
    Array_destroy(&(*font)->glyphs_array);
    mm_free((*font)->ttf_buffer);  // Уничтожаем загруженный шрифт.

    HashTable_clear((*font)->glyphs, true);  // Уничтожаем ключи и глифы из памяти.
    HashTable_destroy(&(*font)->glyphs);

    mm_free(*font);
    *font = NULL;
}

// Получить глиф (из кэша либо создать новый):
FontGlyph* FontPixmap_get_glyph(FontPixmap *self, uint32_t codepoint) {
    if (!self) return NULL;

    // Все отсутствующие в шрифте символы сводим к одному fallback-глифу:
    if (stbtt_FindGlyphIndex(&self->info, codepoint) == 0) {
        codepoint = FONT_FALLBACK_SUMB;
    }

    // Ищем глиф в хэш-таблице:
    uint32_t lookup_key = codepoint;  // Для поиска можно временный ключ на стеке.
    FontGlyph *glyph = (FontGlyph*)HashTable_get(self->glyphs, &lookup_key, sizeof(lookup_key), NULL);
    if (glyph) return glyph;  // Нашли глиф. Возвращаем его.

    // Пытаемся создать глиф:
    while (1) {
        /*
            Если glyph = NULL, мы не смогли создать глиф и надо пересобрать атлас расширив его.
            Если font_expand_and_repack() возвращает false, то мы не смогли пересобрать атлас. Возвращаем NULL.
            Если же получили true, то при помощи цикла снова пытаемся создать глиф и опять проверяем, смогли ли.
        */
        glyph = generate_glyph(self, codepoint);  // Пытаемся создать глиф и добавить в атлас.
        if (glyph) break;                         // Мы смогли создать и добавить глиф в атлас.
        if (!font_expand_and_repack(self)) return NULL;  // Пытаемся увеличить и пересобрать атлас.
    }

    // Сохраняем созданный глиф в хэш-таблицу:
    if (!glyph_insert_to_cache(self, codepoint, glyph)) return NULL;  // Если не получилось, возвращаем NULL.
    Array_push(self->glyphs_array, &codepoint);  // Добавляем кодпоинт в массив глифов.
    return glyph;
}

// Установить цвет текста:
void FontPixmap_set_color(FontPixmap *self, Vec4f color) {
    if (!self) return;
    self->color = color;
}

// Получить цвет текста:
Vec4f FontPixmap_get_color(FontPixmap *self) {
    if (!self) return (Vec4f){1.0f, 1.0f, 1.0f, 1.0f};
    return self->color;
}

// Установить пикселизацию текста:
void FontPixmap_set_pixelized(FontPixmap *self, bool pixelized) {
    if (!self) return;
    if (self->pixelized != pixelized) {
        self->pixelized = pixelized;
        pixelized ? Texture_set_pixelized(self->atlas) : Texture_set_linear(self->atlas);
    }
}

// Получить пикселизацию текста:
bool FontPixmap_get_pixelized(FontPixmap *self) {
    if (!self) return false;
    return self->pixelized;
}

// Отрисовать текст:
void FontPixmap_render(FontPixmap *self, float x, float y, float angle, const char *text, ...) {
    if (!self || !text) return;

    // Форматируем text как f-строку:
    char stack_text[1024];
    char *heap_text = NULL;  // Нужен в случае если символов текста больше чем 1024 байта символов.
    const char *render_text = text;

    va_list args;
    va_start(args, text);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(stack_text, sizeof(stack_text), text, args);
    va_end(args);
    if (needed < 0) { va_end(args_copy); return; }

    // Если текст умещается в стек, то используем стек:
    if ((size_t)needed < sizeof(stack_text)) {
        render_text = stack_text;
    } else {  // Иначе используем кучу:
        size_t heap_size = (size_t)needed + 1;
        heap_text = (char*)mm_alloc(heap_size);
        if (!heap_text) {
            va_end(args_copy);
            return;
        }
        vsnprintf(heap_text, heap_size, text, args_copy);
        render_text = heap_text;
    }
    va_end(args_copy);

    // Состояние для кернинга:
    uint32_t prev_cp = 0;   // Кодпоинт предыдущего символа.
    bool has_prev = false;  // Есть ли предыдущий символ.

    // Центрирование:
    float pen_x = x;
    float baseline = y + self->ascent * self->scale;
    float pivot_x = x;
    float pivot_y = baseline;

    // Для вращения:
    float angle_rad = -radians(angle);
    float s = sinf(angle_rad);
    float c = cosf(angle_rad);

    // Рисуем текст:
    SpriteBatch_begin(self->batch);

    // Перебираем байты текста:
    int i = 0;
    while (render_text[i] != '\0') {
        int adv_bytes = 1;  // Количество байтов символа. Нужен для перехода к следующему символу.
        uint32_t cp = utf8_decode(&render_text[i], &adv_bytes);  // Декодируем символ.

        // (игнорируем):
        if (cp == '\r') {
            i += adv_bytes;
            continue;
        }

        // Перенос строки:
        if (cp == '\n') {
            // Перо (курсор) в начало строки.
            // Базовая линия опускается на высоту строки + смещение.
            pen_x = x;
            baseline -= (self->line_advance + self->line_height);
            has_prev = false;
            i += adv_bytes;
            continue;
        }

        // Табуляция (просто добавляем ширину пробелов):
        if (cp == '\t') {
            pen_x += (self->space_advance + self->letter_spacing) * (float)self->tab_size;
            has_prev = false;
            i += adv_bytes;
            continue;
        }

        // Пробел:
        if (cp == ' ') {
            if (has_prev) {
                int kern = stbtt_GetCodepointKernAdvance(&self->info, prev_cp, cp);
                pen_x += (float)kern * self->scale;
            }
            pen_x += self->space_advance + self->letter_spacing;
            prev_cp = cp;
            has_prev = true;
            i += adv_bytes;
            continue;
        }

        // Глиф (другие символы которые не обработали выше):
        FontGlyph *g = FontPixmap_get_glyph(self, cp);
        if (!g) {  // Не удалось получить глиф. Пропускаем его:
            i += adv_bytes;
            continue;
        }

        // Парный кернинг:
        if (has_prev) {
            int kern = stbtt_GetCodepointKernAdvance(&self->info, prev_cp, cp);
            pen_x += (float)kern * self->scale;
        }

        // Точки отрисовки символов:
        float drawX = pen_x + g->offset_x;
        float drawY = baseline - g->offset_y - g->height;

        // Если размер глифа больше нуля, то рисуем его:
        if (g->width > 0 && g->height > 0) {
            SpriteBatch_set_color(self->batch, self->color);
            SpriteBatch_set_texcoord(self->batch, (Vec4f){g->u0, g->v0, g->u1, g->v1});

            // Центр глифа:
            float gcx = drawX + g->width  * 0.5f;
            float gcy = drawY + g->height * 0.5f;

            // Поворот центра вокруг pivot:
            float dx = gcx - pivot_x;
            float dy = gcy - pivot_y;
            float rcx = pivot_x + dx * c - dy * s;
            float rcy = pivot_y + dx * s + dy * c;

            // Обратно в левый нижний угол:
            float rx = rcx - g->width  * 0.5f;
            float ry = rcy - g->height * 0.5f;
            SpriteBatch_draw(self->batch, self->atlas, rx, ry, (float)g->width, (float)g->height, angle);
        }

        // Сдвигаем курсор:
        pen_x += g->advance + self->letter_spacing;
        prev_cp = cp;
        has_prev = true;
        i += adv_bytes;  // Переходим к следующему символу (advanced bytes).
    }

    SpriteBatch_end(self->batch);
    if (heap_text) mm_free(heap_text);
}
