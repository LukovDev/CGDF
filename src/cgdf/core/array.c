//
// array.c - Реализация динамического массива в си, на основе принципа работы стека (данные плотно упакованы).
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "array.h"


// Проверяем вместимость массива. Расширяем при необходимости:
static inline void check_maybe_growth(Array *arr) {
    if (arr->len >= arr->capacity) {
        Array_growth(arr, ARRAY_GROWTH_FACTOR);
    }
}


// Создать массив с заданным размером:
Array* Array_create(size_t item_size, size_t initial_capacity) {
    if (item_size <= 0) item_size = sizeof(void*);
    if (initial_capacity == 0) {
        initial_capacity = ARRAY_DEFAULT_CAPACITY;
    }

    // Создаём массив:
    Array *arr = (Array*)mm_alloc(sizeof(Array));
    arr->data = mm_alloc(initial_capacity*item_size);  // Выделяем блок памяти под элементы заданного размера.
    arr->item_size = item_size;
    arr->len = 0;
    arr->capacity = initial_capacity;
    arr->init_cap = initial_capacity;
    return arr;
}


// Уничтожить массив:
void Array_destroy(Array **arr) {
    if (!arr || !*arr) return;
    mm_free((*arr)->data);
    (*arr)->data = NULL;
    mm_free(*arr);
    *arr = NULL;
}


// Расширяем массив:
void Array_growth(Array *arr, float factor) {
    if (!arr) return;
    if (factor <= 0) factor = ARRAY_GROWTH_FACTOR;
    size_t new_capacity = (size_t)(arr->capacity * factor);
    if (new_capacity <= arr->capacity) new_capacity = arr->capacity + 1;
    if (new_capacity > arr->capacity) {  // Расширяем если можно.
        arr->capacity = new_capacity;
        arr->data = mm_realloc(arr->data, arr->item_size * arr->capacity);
    }
}


// Сжимаем массив:
void Array_shrink(Array *arr, float factor) {
    if (!arr) return;
    if (factor <= 0) factor = ARRAY_SHRINK_FACTOR;
    size_t new_capacity = (size_t)(arr->len + arr->len*factor);
    if (arr->len <= 0) new_capacity = arr->init_cap;
    if (new_capacity < arr->capacity) {  // Сжимаем если можно.
        arr->capacity = new_capacity;
        arr->data = mm_realloc(arr->data, arr->item_size * arr->capacity);
    }
}


// Добавить элемент в массив (передайте указатель на данные, которые надо скопировать внутрь массива):
void Array_push(Array *arr, const void *element) {
    if (!arr || !element) return;

    // Проверяем вместимость массива:
    check_maybe_growth(arr);

    // Логика: dst = (char*)data + len*item_size:
    void *dst = (char*)arr->data + (arr->len * arr->item_size);
    memcpy(dst, element, arr->item_size);
    arr->len++;
}


// Перезаписать элемент в массиве:
void Array_set(Array *arr, size_t index, const void *element) {
    if (!arr || index >= arr->len || !element) return;
    memcpy(Array_get(arr, index), element, arr->item_size);
}


// Получение элемента по индексу (адрес ячейки):
void* Array_get(Array *arr, size_t index) {
    if (!arr || index >= arr->len) return NULL;
    return (char*)arr->data + (index * arr->item_size);
}


// Получение элемента по индексу (сам указатель):
void* Array_get_ptr(Array *arr, size_t index) {
    if (!arr || index >= arr->len) return NULL;
    return *(char**)((char*)arr->data + (index * arr->item_size));
}


// Получение элемента по индексу по кругу (индексация замкнута, включая отрицательные индексы):
void* Array_get_round(Array *arr, int64_t index) {
    if (!arr || arr->len == 0) return NULL;
    int64_t len = (int64_t)arr->len;
    int64_t i = index % len;
    if (i < 0) i += len;
    return (char*)arr->data + i * arr->item_size;
}


// Вставка элемента по индексу со сдвигом:
void Array_insert(Array *arr, size_t index, const void *element) {
    if (!arr || !element) return;
    if (index > arr->len) index = arr->len;

    // Проверяем вместимость массива:
    check_maybe_growth(arr);

    // Сдвигаем всё вправо от index до конца:
    void *dst = (char*)arr->data + (index * arr->item_size);
    void *src = dst;
    size_t move_count = arr->len - index;
    if (move_count > 0) {
        memmove((char*)dst + arr->item_size, src, move_count * arr->item_size);
    }

    // Вставляем элемент:
    memcpy(dst, element, arr->item_size);
    arr->len++;
}


// Переворот массива:
void Array_reverse(Array *arr) {
    if (!arr || arr->len < 2) return;
    char *tmp = (char*)mm_alloc(arr->item_size);
    size_t i = 0, j = arr->len - 1;
    while (i < j) {
        void *pi = (char*)arr->data + i * arr->item_size;
        void *pj = (char*)arr->data + j * arr->item_size;
        memcpy(tmp, pi, arr->item_size);
        memcpy(pi, pj, arr->item_size);
        memcpy(pj, tmp, arr->item_size);
        i++;
        j--;
    }
    mm_free(tmp);
}


// Заполнить массив элементами:
void Array_fill(Array *arr, const void *element, size_t count) {
    if (!arr || !element || arr->capacity == 0) return;

    // Выделяем память под элементы (если размер массива меньше нужного):
    if (arr->capacity < count) {
        arr->data = mm_realloc(arr->data, arr->item_size * count);
        arr->capacity = count;
    }

    // Куда:
    char *dst = (char*)arr->data;
    memcpy(dst, element, arr->item_size);  // Копируем первый элемент.
    arr->len = count;

    // Заполняем элементами (экспоненциальное заполнение):
    size_t filled = 1;
    while (filled < count) {
        size_t copy_count = (filled * 2 <= count) ? filled : (count - filled);
        memcpy(dst + filled * arr->item_size, dst, copy_count * arr->item_size);
        filled += copy_count;
    }
}


// Копировать массив:
void Array_copy(Array *dst, Array *src) {
    if (!dst || !src || !dst->data) return;

    // Выделяем память под элементы (если есть разница в размере массивов):
    if (dst->item_size != src->item_size || dst->capacity != src->capacity) {
        dst->data = mm_realloc(dst->data, src->item_size * src->capacity);
    }

    // Копируем параметры массива:
    dst->item_size = src->item_size;
    dst->len = src->len;
    dst->capacity = src->capacity;
    dst->init_cap = src->init_cap;

    // Копируем данные:
    memcpy(dst->data, src->data, src->len * src->item_size);
}


// Удаление элемента со сдвигом:
void Array_remove(Array *arr, size_t index, void *out) {
    if (!arr || index >= arr->len) return;
    void *src = (char*)arr->data + index * arr->item_size;
    if (out) memcpy(out, src, arr->item_size);

    size_t move_count = arr->len - index - 1;
    if (move_count > 0) {
        memmove(src, (char*)src + arr->item_size,
                move_count * arr->item_size);
    }

    arr->len--;
}


// Удаление элемента без сдвига, заменяем удаляемый, последним элементом (порядок нарушается, зато быстро):
void Array_remove_swap(Array *arr, size_t index, void *out) {
    if (!arr || index >= arr->len) return;
    void *src = (char*)arr->data + index * arr->item_size;
    if (out) memcpy(out, src, arr->item_size);

    // Если удаляем не последний элемент, заменяем его последним
    if (index != arr->len - 1) {
        void *last = (char*)arr->data + (arr->len - 1) * arr->item_size;
        memcpy(src, last, arr->item_size);
    }

    arr->len--;
}


// Получить и удалить последний элемент из массива (без alloc):
void Array_pop(Array *arr, void *out) {
    if (!arr || arr->len == 0) return;
    Array_remove(arr, arr->len - 1, out);
}


// Получить и удалить последний элемент из массива (alloc с копированием):
void* Array_pop_copy(Array *arr) {
    if (!arr || arr->len == 0) return NULL;
    void *out = mm_alloc(arr->item_size);
    Array_remove(arr, arr->len - 1, out);
    return out;
}


// Печать содержимого массива:
void Array_print(Array *arr, FILE *out, ArrayPrintMode mode) {
    if (!arr || !out) return;
    fprintf(out, "[");
    // Проходимся по всем элементам:
    for (size_t i = 0; i < arr->len; i++) {
        void *p = Array_get(arr, i);
        switch (mode) {
            case ARRAY_PRINT_PTR: {
                fprintf(out, "0x%p", *(void**)p);
            } break;
            case ARRAY_PRINT_HEX: {
                unsigned char *bytes = (unsigned char*)p;
                for (size_t j = 0; j < arr->item_size; j++) {
                    fprintf(out, "%02X", bytes[j]);  
                }
            } break;
            case ARRAY_PRINT_BOOL: {
                unsigned char b;
                memcpy(&b, p, sizeof(unsigned char));
                fprintf(out, b ? "true" : "false");
            } break;
            case ARRAY_PRINT_CHAR: {
                char c = 0;
                memcpy(&c, p, sizeof(char));
                fprintf(out, "'%c'", c);
            } break;
            case ARRAY_PRINT_INT: {
                int i = 0;
                memcpy(&i, p, arr->item_size < sizeof(int) ? arr->item_size : sizeof(int));
                fprintf(out, "%d", i);
            } break;
            case ARRAY_PRINT_LONG: {
                long l = 0;
                memcpy(&l, p, sizeof(long));
                fprintf(out, "%ld", l);
            } break;
            case ARRAY_PRINT_LLONG: {
                long long ll = 0;
                memcpy(&ll, p, sizeof(long long));
                fprintf(out, "%lld", ll);
            } break;
            case ARRAY_PRINT_FLOAT: {
                float f;
                memcpy(&f, p, sizeof(float));
                fprintf(out, "%f", f);
            } break;
            case ARRAY_PRINT_DOUBLE: {
                double d;
                memcpy(&d, p, sizeof(double));
                fprintf(out, "%lf", d);
            } break;
            case ARRAY_PRINT_STRING: {
                char *str = (char*)Array_get_ptr(arr, i);
                if (!str) str = "(null)";
                fprintf(out, "\"%s\"", str);
            } break;
            default: {
                fprintf(out, "0x%p", p);
            } break;
        }
        if (i < arr->len - 1) fprintf(out, ", ");
    }
    fprintf(out, "]\n");
}


// Получить длину массива:
size_t Array_len(Array *arr) {
    if (!arr) return 0;
    return arr->len;
}


// Получить размер массива:
size_t Array_capacity(Array *arr) {
    if (!arr) return 0;
    return arr->capacity;
}


// Очистка массива:
void Array_clear(Array *arr, bool free_data) {
    if (!arr) return;

    // Удаляем все элементы (освобождаем указатели):
    if (free_data) {
        for (size_t i = 0; i < arr->len; i++) {
            char *p = Array_get_ptr(arr, i);
            if (p) mm_free(p);
        }
    }

    // Обнуляем массив:
    arr->len = 0;
    arr->capacity = arr->init_cap;
    arr->data = mm_realloc(arr->data, arr->item_size * arr->capacity);
}
