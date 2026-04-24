//
// mm.c - Исходник реализовывающий базовую работу менеджера памяти.
//
// Пока что просто обертка над обычным malloc, но позволяет отслеживать
// использование памяти, и получать размер блока памяти. Отслеживание
// памяти является атомарным, что подходит для многопоточности.
//


// Подключаем:
#include "std.h"
#include "logger.h"
#include "mm.h"


// Определения:
#define MM_RETRY_ALLOC_AGAIN 1  // 0 = В случае ошибки выделения - крах. 1 = Повторять выделение в случае ошибки.


// Определения функций аллокатора которые используются в этой обертке (пока что используется базовый аллокатор):
void* (*_m_alloc)   (size_t s)           = malloc;
void* (*_m_calloc)  (size_t c, size_t s) = calloc;
void* (*_m_realloc) (void *p, size_t s)  = realloc;
void  (*_m_free)    (void *p)            = free;


// Структура заголовка блока памяти:
typedef struct MM_BlockHeader {
    void *base_ptr;    // Сырой указатель от аллокатора.
    size_t size;       // Размер выделяемого блока.
    size_t alignment;  // Выравнивание.
} MM_BlockHeader;


// Локальные переменные:
static const size_t _header_size_ = sizeof(MM_BlockHeader);  // Размер заголовка блока.
static atomic_size_t mm_allocated_blocks = 0;                // Количество выделенных блоков.
static atomic_size_t mm_used_size = 0;                       // Количество используемой виртуальной памяти.
static atomic_size_t mm_last_request_size = 0;               // Размер последнего запроса на выделение (в байтах).


// -------- Вспомогательные функции: --------


// Возвращает минимально безопасное выравнивание:
static inline size_t mm_required_alignment(void) {
    size_t a = alignof(max_align_t);
    #if defined(__AVX512F__)
        if (a < 64u) a = 64u;
    #elif defined(__AVX__) || defined(__AVX2__)
        if (a < 32u) a = 32u;
    #elif defined(__SSE__) || defined(__SSE2__) || defined(__ARM_NEON)
        if (a < 16u) a = 16u;
    #endif
    return a;
}

// Округляет адрес вверх до ближайшей границы alignment:
static inline uintptr_t mm_align_up_uintptr(uintptr_t p, size_t alignment) {
    return (p + (uintptr_t)(alignment - 1u)) & ~((uintptr_t)alignment - 1u);
}

// Возвращает указатель на заголовок блока, который лежит непосредственно перед ptr:
static inline MM_BlockHeader* mm_get_header(void *ptr) {
    return (MM_BlockHeader*)((char*)ptr - sizeof(MM_BlockHeader));
}


// -------- Основной код: --------


// Получить размер заголовка блока в байтах:
size_t mm_get_block_header_size(void) { return _header_size_; }


// Получить количество выделенных блоков:
size_t mm_get_allocated_blocks(void) { return mm_allocated_blocks; }


// Получить абсолютный размер используемой памяти в байтах с учётом заголовков блоков:
size_t mm_get_absolute_used_size(void) { return mm_used_size + _header_size_ * mm_allocated_blocks; }


// Получить сколько всего используется памяти в байтах этим менеджером памяти:
size_t mm_get_used_size(void) { return mm_used_size; }


// Получить сколько всего используется памяти в килобайтах этим менеджером памяти:
double mm_get_used_size_kb(void) { return mm_get_used_size() / 1024.0; }  // b -> kb.


// Получить сколько всего используется памяти в мегабайтах этим менеджером памяти:
double mm_get_used_size_mb(void) { return mm_get_used_size() / 1024.0 / 1024.0; }  // b -> kb -> mb.


// Получить сколько всего используется памяти в гигабайтах этим менеджером памяти:
double mm_get_used_size_gb(void) { return mm_get_used_size() / 1024.0 / 1024.0 / 1024.0; }  // b -> kb -> mb -> gb.


// Получить размер блока в байтах:
size_t mm_get_block_size(void *ptr) {
    if (!ptr) return 0;
    return mm_get_header(ptr)->size;
}


// Добавить байты к использованной памяти (атомарно):
void mm_used_size_add(size_t size) {
    atomic_fetch_add(&mm_used_size, size);
}


// Вычесть байты из использованной памяти (атомарно):
void mm_used_size_sub(size_t size) {
    atomic_fetch_sub(&mm_used_size, size);
}


// Выделение памяти с явным выравниванием:
void* mm_alloc_aligned(size_t size, size_t alignment) {
        if (!(alignment && ((alignment & (alignment - 1u)) == 0u))) {
        mm_last_request_size = alignment;
        mm_alloc_error();
        return NULL;
    }

    size_t min_align = alignof(max_align_t);
    if (alignment < min_align) alignment = min_align;

    /* Если хочешь не дать пользователю случайно попросить меньше SIMD-минимума */
    size_t simd_align = mm_required_alignment();
    if (alignment < simd_align) alignment = simd_align;

    if (size > SIZE_MAX - sizeof(MM_BlockHeader) - (alignment - 1u)) {
        mm_last_request_size = size;
        mm_alloc_error();
        return NULL;
    }

    size_t total = sizeof(MM_BlockHeader) + size + (alignment - 1u);
    mm_last_request_size = total;

    char *base_ptr = NULL;
    if (MM_RETRY_ALLOC_AGAIN) {
        while (!base_ptr) base_ptr = (char*)_m_alloc(total);
    } else {
        base_ptr = (char*)_m_alloc(total);
    }
    if (!base_ptr) { mm_alloc_error(); return NULL; }

    uintptr_t aligned_up = mm_align_up_uintptr((uintptr_t)base_ptr + sizeof(MM_BlockHeader), alignment);
    void *ptr = (void*)aligned_up;

    MM_BlockHeader *header = mm_get_header(ptr);
    header->base_ptr = base_ptr;
    header->size = size;
    header->alignment = alignment;

    mm_used_size_add(size);
    mm_allocated_blocks++;
    return ptr;
}


// Выделение памяти:
void* mm_alloc(size_t size) {
    return mm_alloc_aligned(size, mm_required_alignment());
}


// Выделение памяти с обнулением:
void* mm_calloc(size_t count, size_t size) {
    if (count != 0 && size > SIZE_MAX / count) {
        mm_last_request_size = SIZE_MAX;
        mm_alloc_error();
        return NULL;
    }
    void *ptr = mm_alloc(count * size);
    if (ptr) memset(ptr, 0, count * size);
    return ptr;
}


// Расширение блока памяти:
void* mm_realloc(void *ptr, size_t new_size) {
    if (!ptr) return mm_alloc(new_size);
    if (new_size == 0) { mm_free(ptr); return NULL; }
    MM_BlockHeader *old_h = mm_get_header(ptr);
    size_t old_size = old_h->size;
    size_t old_alignment = old_h->alignment;
    void *new_ptr = mm_alloc_aligned(new_size, old_alignment);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    mm_free(ptr);
    return new_ptr;
}


// Копирование строки:
char* mm_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = (char*)mm_alloc(len);
    if (!copy) { mm_alloc_error(); return NULL; }
    memcpy(copy, str, len);
    return copy;
}


// Освобождение памяти:
void mm_free(void *ptr) {
    if (!ptr) return;
    MM_BlockHeader *header = mm_get_header(ptr);
    mm_used_size_sub(header->size);
    mm_allocated_blocks--;
    _m_free(header->base_ptr);
}


// Вызовите если получите проблему при выделении памяти:
void mm_alloc_error(void) {
    log_msg("----------------\n");
    log_msg("[E] Memory Allocation Error!\n");
    log_msg("Memory used: %g kb (%zu b).\n", mm_get_used_size_kb(), mm_get_used_size());
    log_msg("Allocated blocks: %zu.\n", mm_get_allocated_blocks());
    log_msg("Absolute memory used: %zu b.\n", mm_get_absolute_used_size());
    log_msg("Block Header Size: %zu b.\n", mm_get_block_header_size());
    log_msg("Last request for allocation: %zu b.\n", mm_last_request_size);
    log_msg("----------------\n");
    exit(ENOMEM);
}
