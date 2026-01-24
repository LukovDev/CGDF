//
// array.h
//

#pragma once


// Подключаем:
#include "std.h"


// Определения:
#define ARRAY_DEFAULT_CAPACITY 1024  // Размер массива по умолчанию.
#define ARRAY_GROWTH_FACTOR    2     // Коэффициент расширения массива.
#define ARRAY_SHRINK_FACTOR    0.25  // Коэффициент сжатия массива.


// Перечисление режимов печати:
typedef enum {
    ARRAY_PRINT_PTR,
    ARRAY_PRINT_HEX,
    ARRAY_PRINT_BOOL,
    ARRAY_PRINT_CHAR,
    ARRAY_PRINT_INT,
    ARRAY_PRINT_LONG,
    ARRAY_PRINT_LLONG,
    ARRAY_PRINT_FLOAT,
    ARRAY_PRINT_DOUBLE,
    ARRAY_PRINT_STRING,
} ArrayPrintMode;


// Объявление структур:
typedef struct Array Array;  // Динамический массив.


// Структура массива:
struct Array {
    void *data;        // Базовый адрес блока элементов.
    size_t item_size;  // Размер одного элемента.
    size_t len;        // Длина массива (сколько ячеек занято).
    size_t capacity;   // Всего выделенных ячеек в памяти (вместимость).
    size_t init_cap;   // Размер массива по умолчанию.
};


// Создать массив с заданным размером:
Array* Array_create(size_t item_size, size_t initial_capacity);

// Уничтожить массив:
void Array_destroy(Array **arr);

// Расширяем массив:
void Array_growth(Array *arr, float factor);

// Сжимаем массив:
void Array_shrink(Array *arr, float factor);

// Добавить элемент в массив (передайте указатель на данные, которые надо скопировать внутрь массива):
void Array_push(Array *arr, const void *element);

// Перезаписать элемент в массиве:
void Array_set(Array *arr, size_t index, const void *element);

// Получение элемента по индексу (адрес ячейки):
void* Array_get(Array *arr, size_t index);

// Получение элемента по индексу (сам указатель):
void* Array_get_ptr(Array *arr, size_t index);

// Получение элемента по индексу по кругу (индексация замкнута, включая отрицательные индексы):
void* Array_get_round(Array *arr, int64_t index);

// Вставка элемента по индексу со сдвигом:
void Array_insert(Array *arr, size_t index, const void *element);

// Переворот массива:
void Array_reverse(Array *arr);

// Заполнить массив элементами:
void Array_fill(Array *arr, const void *element, size_t count);

// Копировать массив:
void Array_copy(Array *dst, Array *src);

// Удаление элемента со сдвигом:
void Array_remove(Array *arr, size_t index, void *out);

// Удаление элемента без сдвига, заменяем удаляемый последний элементом (порядок нарушается, зато быстро):
void Array_remove_swap(Array *arr, size_t index, void *out);

// Получить и удалить последний элемент из массива (без alloc):
void Array_pop(Array *arr, void *out);

// Получить и удалить последний элемент из массива (alloc с копированием):
void* Array_pop_copy(Array *arr);

// Печать содержимого массива:
void Array_print(Array *arr, FILE *out, ArrayPrintMode mode);

// Получить длину массива:
size_t Array_len(Array *arr);

// Получить размер массива:
size_t Array_capacity(Array *arr);

// Очистка массива:
void Array_clear(Array *arr, bool free_data);
