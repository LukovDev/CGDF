//
// hashtable.c - Реализует работу с хэш-таблицами.
// Основан на линейном пробировании, работе с указателями (а не копиями!).
// Поддерживает несколько триггеров для поддержания производительности:
// 1. Авторасширение (при достижении порога заполненности в таблице).
// 2. Автосжатение (при достижении порога свободного места в таблицы).
// 3. Лимит пробирования (перераспределяем таблицу и расширяем).
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "hashtable.h"


// Выбираем какую функцию хэша использовать (можно сменить, главное сохранить сигнатуру):
uint64_t (*hash_func) (const void* data, size_t len) = hash_fnv1a;


// Вывод данных для функции HashTable_print():
static void print_data(FILE *out, void *data, size_t size, HashTablePrintMode mode) {
    if (!data) {
        fprintf(out, "(null)");
        return;
    }
    switch (mode) {
        case HASHTABLE_PRINT_PTR: {
            fprintf(out, "0x%p", *(void**)data);
        } break;
        case HASHTABLE_PRINT_HEX: {
            unsigned char *bytes = (unsigned char*)data;
            for (size_t j = 0; j < size; j++) {
                fprintf(out, "%02X", bytes[j]);
            }
        } break;
        case HASHTABLE_PRINT_BOOL: {
            unsigned char b = 0;
            memcpy(&b, data, sizeof(unsigned char));
            fprintf(out, b ? "true" : "false");
        } break;
        case HASHTABLE_PRINT_CHAR: {
            char c = 0;
            memcpy(&c, data, sizeof(char));
            fprintf(out, "'%c'", c);
        } break;
        case HASHTABLE_PRINT_INT: {
            int i = 0;
            memcpy(&i, data, size < sizeof(int) ? size : sizeof(int));
            fprintf(out, "%d", i);
        } break;
        case HASHTABLE_PRINT_LONG: {
            long l = 0;
            memcpy(&l, data, size < sizeof(long) ? size : sizeof(long));
            fprintf(out, "%ld", l);
        } break;
        case HASHTABLE_PRINT_LLONG: {
            long long ll = 0;
            memcpy(&ll, data, size < sizeof(long long) ? size : sizeof(long long));
            fprintf(out, "%lld", ll);
        } break;
        case HASHTABLE_PRINT_FLOAT: {
            float f = 0;
            memcpy(&f, data, sizeof(float));
            fprintf(out, "%f", f);
        } break;
        case HASHTABLE_PRINT_DOUBLE: {
            double d = 0;
            memcpy(&d, data, sizeof(double));
            fprintf(out, "%lf", d);
        } break;
        case HASHTABLE_PRINT_STRING: {
            char *str = (char*)data;
            if (!str) str = "(null)";
            fprintf(out, "\"%s\"", str);
        } break;
        default: {
            fprintf(out, "(unknown)");
        } break;
    }
}


// Очищаем счетчики пробирований:
static inline void reset_probs(HashTable *table) {
    if (!table) return;
    for (size_t i = 0; i < HASHTABLE_PROBING_COUNT; i++) table->prob_count[i] = 0;
    table->prob_index = 0;
}


// Перераспределение хэш-таблицы:
static inline void rehash(HashTable *table, size_t new_capacity) {
    if (!table || table->len <= 0 || table->capacity <= 0) return;

    // Подготавливаем данные:
    HashSlot *old_data = table->data;
    HashSlot *new_data = mm_calloc(new_capacity, sizeof(HashSlot));
    size_t old_capacity = table->capacity, new_len = 0;

    // Переносим данные:
    for (size_t i = 0; i < old_capacity; i++) {
        HashSlot *slot = &old_data[i];
        if (!slot->key || slot->deleted) continue;  // Пропускаем пустые и удалённые элементы.

        // Проходим от 0 до конца массива с wrap-around:
        size_t idx = slot->hash % new_capacity;
        for (size_t j = 0; j < new_capacity; j++) {
            // Мы проверяем все слоты массива от idx до конца, а потом от начала до idx-1 (вокруг idx):
            size_t index = (idx + j) % new_capacity;
            HashSlot *new_slot = &new_data[index];
            // Если слот пуст (нет ключа = нет элемента):
            if (!new_slot->key) {
                new_slot->key = slot->key;
                new_slot->key_size = slot->key_size;
                new_slot->value = slot->value;
                new_slot->value_size = slot->value_size;
                new_slot->hash = slot->hash;
                new_slot->deleted = false;
                new_len++;
                break;
            }
            // Иначе коллизия. Пробуем дальше.
        }
    }

    // Освобождаем старую таблицу:
    table->data = new_data;
    table->capacity = new_capacity;
    table->len = new_len;
    mm_free(old_data);
    reset_probs(table);
}


// Расширяем хэш-таблицу:
static inline void growth(HashTable *table, float factor) {
    if (!table) return;
    if (factor <= 0) factor = HASHTABLE_GROWTH_FACTOR;
    size_t new_capacity = (size_t)(table->capacity * factor);
    if (new_capacity <= table->capacity) new_capacity = table->capacity + 1;
    rehash(table, new_capacity);
}


// Сжимаем хэш-таблицу:
static inline void shrink(HashTable *table, float factor) {
    if (!table) return;
    if (factor <= 0) factor = HASHTABLE_SHRINK_FACTOR;
    size_t new_capacity = (size_t)(table->len * factor);
    if (new_capacity < table->len) new_capacity = table->len * 2;
    if (new_capacity < HASHTABLE_MIN_CAPACITY) new_capacity = HASHTABLE_MIN_CAPACITY;  // Минимальный размер таблицы.
    if (new_capacity < table->capacity) rehash(table, new_capacity);
}


// Проверка на необходимость расширения таблицы (для поддержания свободного места):
static inline void check_maybe_growth(HashTable *table) {
    if (!table || table->capacity <= 0) return;
    if ((float)table->len / (float)table->capacity >= HASHTABLE_GROWTH_THRESHOLD) {
        growth(table, HASHTABLE_GROWTH_FACTOR);
    }
}


// Проверка на необходимость сжатия таблицы (для освобождения памяти):
static inline void check_maybe_shrink(HashTable *table) {
    if (!table || table->capacity <= 0) return;
    if ((float)table->len / (float)table->capacity <= HASHTABLE_SHRINK_THRESHOLD) {
        shrink(table, HASHTABLE_SHRINK_FACTOR);
    }
}


// Проверить на превышение лимита пробирований:
static inline void check_maybe_problimit(HashTable *table) {
    if (!table) return;
    size_t sum = 0;
    for (size_t i = 0; i < HASHTABLE_PROBING_COUNT; i++) { sum += table->prob_count[i]; }
    sum /= HASHTABLE_PROBING_COUNT;

    // Проверяем больше ли чем лимит пробирований:
    if (sum > HASHTABLE_PROBING_LIMIT) {
        growth(table, HASHTABLE_GROWTH_FACTOR);
        reset_probs(table);
    }
}


// Создать хэш-таблицу:
HashTable* HashTable_create() {
    size_t capacity = HASHTABLE_DEFAULT_CAPACITY;
    HashTable *table = (HashTable*)mm_alloc(sizeof(HashTable));
    table->data = mm_calloc(capacity, sizeof(HashSlot));
    table->len = 0;
    table->capacity = capacity;
    table->prob_index = 0;
    reset_probs(table);
    return table;
}


// Уничтожить хэш-таблицу (не удаляет блоки по указателям):
void HashTable_destroy(HashTable **table) {
    if (!table || !*table) return;
    mm_free((*table)->data);
    (*table)->data = NULL;
    mm_free(*table);
    *table = NULL;
}


// Добавить элемент в хэш-таблицу или обновить его значение:
bool HashTable_set(HashTable *table, const void *key, size_t key_size, const void *value, size_t value_size) {
    if (!table || !key) return false;

    // Проверяем лимит пробирований и заполненность таблицы:
    check_maybe_problimit(table);
    check_maybe_growth(table);

    // Инициализируем данные для пробирований:
    size_t hash = hash_func(key, key_size);
    size_t idx = hash % table->capacity;
    size_t prob_idx = table->prob_index++ % HASHTABLE_PROBING_COUNT;
    table->prob_count[prob_idx] = 0;  // Обнуляем для этой сессии пробингов.

    // Проходим от 0 до конца массива с wrap-around:
    for (size_t i = 0; i < table->capacity; i++) {
        table->prob_count[prob_idx]++;  // Увеличиваем пробинг.

        // Мы проверяем все слоты массива от idx до конца, а потом от начала до idx-1 (вокруг idx):
        size_t index = (idx + i) % table->capacity;
        HashSlot *slot = &table->data[index];

        // Если слот пуст (нет ключа = нет элемента) или помечен как удалённый:
        if (!slot->key || slot->deleted) {
            slot->key = (void*)key;
            slot->key_size = key_size;
            slot->value = (void*)value;
            slot->value_size = value_size;
            slot->hash = hash;
            slot->deleted = false;
            table->len++;
            return true;
        }

        // Если нашли слот, совпадают хэш и размер ключа, и ключи равны - обновляем значение:
        if (slot->hash == hash && slot->key_size == key_size && memcmp(slot->key, key, key_size) == 0) {
            slot->value = (void*)value;
            slot->value_size = value_size;
            slot->deleted = false;
            return true;
        }
        // Иначе пробуем искать дальше...
    }
    return false;  // Не удалось добавить элемент.
}


// Получить элемент по ключу. Возвращает указатель на value, иначе NULL:
void* HashTable_get(HashTable *table, const void *key, size_t key_size, size_t *out_value_size) {
    if (!table || !key) return NULL;

    // Проверяем лимит пробирований:
    check_maybe_problimit(table);

    // Инициализируем данные для пробирований:
    size_t hash = hash_func(key, key_size);
    size_t idx = hash % table->capacity;
    size_t prob_idx = table->prob_index++ % HASHTABLE_PROBING_COUNT;
    table->prob_count[prob_idx] = 0;  // Обнуляем для этой сессии пробингов.

    // Проходим от 0 до конца массива с wrap-around:
    for (size_t i = 0; i < table->capacity; i++) {
        table->prob_count[prob_idx]++;  // Увеличиваем пробинг.

        // Мы проверяем все слоты массива от idx до конца, а потом от начала до idx-1 (вокруг idx):
        size_t index = (idx + i) % table->capacity;
        HashSlot *slot = &table->data[index];

        // Если слот удалён, пробуем искать дальше:
        if (slot->deleted) continue;

        // Если слот пуст (нет ключа = нет элемента):
        if (!slot->key) {
            return NULL;
        }

        // Иначе сравниваем ключи:
        if (slot->hash == hash && slot->key_size == key_size && memcmp(slot->key, key, key_size) == 0) {
            if (out_value_size) *out_value_size = slot->value_size;  // Возвращаем размер значения.
            return slot->value;
        }
    }
    return NULL;  // Не удалось найти элемент.
}


// Получить слот из таблицы по индексу:
HashSlot* HashTable_get_slot(HashTable *table, size_t index) {
    if (!table || index >= table->capacity) return NULL;
    return &table->data[index];
}


// Удалить элемент из таблицы:
bool HashTable_remove(HashTable *table, const void *key, size_t key_size, bool free_data) {
    if (!table || !key) return false;

    // Проверяем лимит пробирований:
    check_maybe_problimit(table);

    // Инициализируем данные для пробирований:
    size_t hash = hash_func(key, key_size);
    size_t idx = hash % table->capacity;
    size_t prob_idx = table->prob_index++ % HASHTABLE_PROBING_COUNT;
    table->prob_count[prob_idx] = 0;  // Обнуляем для этой сессии пробингов.

    // Проходим от 0 до конца массива с wrap-around:
    for (size_t i = 0; i < table->capacity; i++) {
        table->prob_count[prob_idx]++;  // Увеличиваем пробинг.

        // Мы проверяем все слоты массива от idx до конца, а потом от начала до idx-1 (вокруг idx):
        size_t index = (idx + i) % table->capacity;
        HashSlot *slot = &table->data[index];

        // Если слот удалён, пробуем искать дальше:
        if (slot->deleted) continue;

        // Если слот пуст (нет ключа = нет элемента):
        if (!slot->key) return false;

        // Иначе сравниваем ключи:
        if (slot->hash == hash && slot->key_size == key_size && memcmp(slot->key, key, key_size) == 0) {
            if (free_data) {
                if (slot->key == slot->value) mm_free(slot->key);
                else {
                    if (slot->key) mm_free(slot->key);
                    if (slot->value) mm_free(slot->value);
                }
            }
            slot->key = NULL;
            slot->value = NULL;
            slot->key_size = 0;
            slot->value_size = 0;
            slot->hash = 0;
            slot->deleted = true;
            table->len--;
            check_maybe_shrink(table);
            return true;
        }
    }
    return false;  // Не удалось найти элемент.
}


// Возвращает true, если ключ есть в таблице:
bool HashTable_has(HashTable *table, const void *key, size_t key_size) {
    if (!table) return false;
    return HashTable_get(table, key, key_size, NULL) != NULL;
}


// Получить длину таблицы:
size_t HashTable_len(HashTable *table) {
    if (!table) return 0;
    return table->len;
}


// Получить вместимость таблицы:
size_t HashTable_capacity(HashTable *table) {
    if (!table) return 0;
    return table->capacity;
}


// Вывести содержимое таблицы:
void HashTable_print(HashTable *table, FILE *out, HashTablePrintMode key_mode, HashTablePrintMode value_mode) {
    if (!table || !out) return;
    const char* separator = "------------------------------------------------";
    fprintf(out, "%s\n", separator);
    fprintf(out, "Hash Table overview:\n");

    // Выводим состояние таблицы:
    size_t len = table->len, capacity = table->capacity;
    float load = ((float)len / (float)capacity) * 100.0f;
    float max_load = HASHTABLE_GROWTH_THRESHOLD * 100.0f;
    fprintf(out, "Len: %zu | Capacity: %zu | Load: %.1f%% (max: %.1f%%).\n\n", len, capacity, load, max_load);

    // Проходимся по всей таблице:
    for (size_t idx = 0; idx < table->capacity; idx++) {
        HashSlot *slot = &table->data[idx];  // Получаем слот.

        // Выводим информацию о слоте:
        fprintf(out, "[IDX %zu | REQ %zu | DEL: %s | ",
                idx, slot->hash ? (slot->hash % table->capacity) : 0,
                slot->deleted ? "true" : "false");

        // Выводим информацию о ключе:
        fprintf(out, "K: <");
        print_data(out, slot->key, slot->key_size, key_mode);
        fprintf(out, "> (%zub) | ", slot->key_size);

        // Выводим информацию о значении:
        fprintf(out, "V: <");
        print_data(out, slot->value, slot->value_size, value_mode);
        fprintf(out, "> (%zub)]\n", slot->value_size);
    }
    fprintf(out, "%s\n", separator);
}


// Очистить таблицу (без освобождения памяти по умолчанию):
void HashTable_clear(HashTable *table, bool free_data) {
    if (!table) return;

    // Если надо удалять данные (ключ и значение):
    if (free_data) {
        for (size_t i = 0; i < table->capacity; i++) {
            HashSlot *slot = &table->data[i];
            if (slot->key == slot->value) mm_free(slot->key);
            else {
                if (slot->key) mm_free(slot->key);
                if (slot->value) mm_free(slot->value);
            }
        }
    }

    // Обнуляем таблицу:
    table->len = 0;
    memset(table->data, 0, sizeof(HashSlot) * table->capacity);
    check_maybe_shrink(table);
    reset_probs(table);  // Точно сбрасываем статистику.
}
