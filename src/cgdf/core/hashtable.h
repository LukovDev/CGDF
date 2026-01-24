//
// hashtable.h
//

#pragma once


// Подключаем:
#include "std.h"


// Определения:
#define HASHTABLE_DEFAULT_CAPACITY 4096  // Размер хэш-таблицы по-умолчанию.
#define HASHTABLE_MIN_CAPACITY     1024  // Минимальный размер хэш-таблицы.
#define HASHTABLE_GROWTH_FACTOR    2     // Коэффициент увеличения таблицы.
#define HASHTABLE_SHRINK_FACTOR    2     // Коэффициент сжатия таблицы (формула: cap = len*SHRINK_FACTOR).
#define HASHTABLE_GROWTH_THRESHOLD 0.66  // Порог количества заполненности таблицы для расширения (%).
#define HASHTABLE_SHRINK_THRESHOLD 0.25  // Порог количества заполненности таблицы для сжатия (%).
#define HASHTABLE_PROBING_COUNT    64    // Массив записей последних пробирований (аналитика).
#define HASHTABLE_PROBING_LIMIT    32    // Лимит пробирований при поиске слота.


// Перечисление режимов печати:
typedef enum {
    HASHTABLE_PRINT_PTR,
    HASHTABLE_PRINT_HEX,
    HASHTABLE_PRINT_BOOL,
    HASHTABLE_PRINT_CHAR,
    HASHTABLE_PRINT_INT,
    HASHTABLE_PRINT_LONG,
    HASHTABLE_PRINT_LLONG,
    HASHTABLE_PRINT_FLOAT,
    HASHTABLE_PRINT_DOUBLE,
    HASHTABLE_PRINT_STRING,
} HashTablePrintMode;


// Объявление структур:
typedef struct HashSlot HashSlot;    // Слот в таблице.
typedef struct HashTable HashTable;  // Хэш-таблица.


// Структура слота таблицы:
struct HashSlot {
    void  *key;         // Указатель на ключ.
    size_t key_size;    // Размер блока ключа.
    void  *value;       // Указатель на значение.
    size_t value_size;  // Размер блока значения.
    size_t hash;        // Хэш ключа (высчитывается один раз для оптимизации).
    bool   deleted;     // Флаг удаления (нужен для различия удалённых слотов от пустых).
};


// Структура хэш-таблицы:
struct HashTable {
    HashSlot *data;       // Таблица слотов.
    size_t   len;         // Длина таблицы (сколько ячеек занято).
    size_t   capacity;    // Всего выделенных ячеек в памяти (вместимость).
    size_t   prob_count[HASHTABLE_PROBING_COUNT];  // Количество пробирований (поиск слота).
    size_t   prob_index;  // Индекс (счетчик) в массиве prob_count.
};


// Функция хэша на основе FNV-1a:
static inline uint64_t hash_fnv1a(const void* data, size_t len) {
    uint64_t hash = 1469598103934665603ULL;  // Offset basis.
    const unsigned char* ptr = (const unsigned char*)data;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint64_t)ptr[i];
        hash *= 1099511628211ULL;  // FNV prime.
    }
    return hash;
}


// Создать хэш-таблицу:
HashTable* HashTable_create();

// Уничтожить хэш-таблицу (не удаляет блоки по указателям):
void HashTable_destroy(HashTable **table);

// Добавить элемент в хэш-таблицу или обновить его значение:
bool HashTable_set(HashTable *table, const void *key, size_t key_size, const void *value, size_t value_size);

// Получить элемент по ключу. Возвращает указатель на value, иначе NULL:
void* HashTable_get(HashTable *table, const void *key, size_t key_size, size_t *out_value_size);

// Получить слот из таблицы по индексу:
HashSlot* HashTable_get_slot(HashTable *table, size_t index);

// Удалить элемент из таблицы:
bool HashTable_remove(HashTable *table, const void *key, size_t key_size, bool free_data);

// Возвращает true, если ключ есть в таблице:
bool HashTable_has(HashTable *table, const void *key, size_t key_size);

// Получить длину таблицы:
size_t HashTable_len(HashTable *table);

// Получить вместимость таблицы:
size_t HashTable_capacity(HashTable *table);

// Вывести содержимое таблицы:
void HashTable_print(HashTable *table, FILE *out, HashTablePrintMode key_mode, HashTablePrintMode value_mode);

// Очистить таблицу (без освобождения памяти по умолчанию):
void HashTable_clear(HashTable *table, bool free_data);
