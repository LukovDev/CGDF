# API CGDF

Обратно к [**главной документации.**](readme.md)

_Сгенерировано автоматически. Версия генератора: 1.0.0_

<a id="content"></a>
## Содержание

- **src/cgdf/**
  - [cgdf.h](#api-src-cgdf-cgdf-h)
- **src/cgdf/core/**
  - [array.h](#api-src-cgdf-core-array-h)
  - [constants.h](#api-src-cgdf-core-constants-h)
  - [core.h](#api-src-cgdf-core-core-h)
  - [files.h](#api-src-cgdf-core-files-h)
  - [hashtable.h](#api-src-cgdf-core-hashtable-h)
  - [logger.h](#api-src-cgdf-core-logger-h)
  - [math.h](#api-src-cgdf-core-math-h)
  - [mm.h](#api-src-cgdf-core-mm-h)
  - [pixmap.h](#api-src-cgdf-core-pixmap-h)
  - [platform.h](#api-src-cgdf-core-platform-h)
  - [time.h](#api-src-cgdf-core-time-h)
  - [vector.h](#api-src-cgdf-core-vector-h)
- **src/cgdf/graphics/core/**
  - [animator.h](#api-src-cgdf-graphics-core-animator-h)
  - [camera.h](#api-src-cgdf-graphics-core-camera-h)
  - [draw.h](#api-src-cgdf-graphics-core-draw-h)
  - [font.h](#api-src-cgdf-graphics-core-font-h)
  - [input.h](#api-src-cgdf-graphics-core-input-h)
  - [light.h](#api-src-cgdf-graphics-core-light-h)
  - [mesh.h](#api-src-cgdf-graphics-core-mesh-h)
  - [renderer.h](#api-src-cgdf-graphics-core-renderer-h)
  - [scene.h](#api-src-cgdf-graphics-core-scene-h)
  - [shader.h](#api-src-cgdf-graphics-core-shader-h)
  - [sprite.h](#api-src-cgdf-graphics-core-sprite-h)
  - [spritebatch.h](#api-src-cgdf-graphics-core-spritebatch-h)
  - [texture.h](#api-src-cgdf-graphics-core-texture-h)
  - [utils.h](#api-src-cgdf-graphics-core-utils-h)
  - [vertex.h](#api-src-cgdf-graphics-core-vertex-h)
  - [window.h](#api-src-cgdf-graphics-core-window-h)
- **src/cgdf/graphics/core/controllers/**
  - [controllers.h](#api-src-cgdf-graphics-core-controllers-controllers-h)
- **src/cgdf/graphics/opengl/**
  - [buffer_gc.h](#api-src-cgdf-graphics-opengl-buffer-gc-h)
  - [gl.h](#api-src-cgdf-graphics-opengl-gl-h)
  - [texunit.h](#api-src-cgdf-graphics-opengl-texunit-h)
- **src/cgdf/graphics/opengl/buffers/**
  - [buffers.h](#api-src-cgdf-graphics-opengl-buffers-buffers-h)

#

### src/cgdf/

<a id="api-src-cgdf-cgdf-h"></a>
- ### cgdf.h:
  > Описание: C Game Development Framework. Фреймворк для разработки игр на Си.

  [Назад](#content)

  **Определения:**</br>
  `CGDF_VERSION`:
  - Значение: `"v1.0.0-alpha"`.

  **Функции:**</br>
  > Инициализировать фреймворк:</br>
  > `static inline bool CGDF_Init(void);`

  > Получить версию фреймворка:</br>
  > `static inline const char* CGDF_GetVersion(void);`

#

### src/cgdf/core/

<a id="api-src-cgdf-core-array-h"></a>
- ### array.h:
  [Назад](#content)

  **Определения:**</br>
  `ARRAY_DEFAULT_CAPACITY`:
  - Размер массива по умолчанию.
  - Значение: `1024`.

  `ARRAY_GROWTH_FACTOR`:
  - Коэффициент расширения массива.
  - Значение: `2`.

  `ARRAY_SHRINK_FACTOR`:
  - Коэффициент сжатия массива.
  - Значение: `0.25`.

  **Перечисления:**</br>
  enum `ArrayPrintMode`:
  - Перечисление режимов печати.
  - Значения:
    - `ARRAY_PRINT_PTR`
    - `ARRAY_PRINT_HEX`
    - `ARRAY_PRINT_BOOL`
    - `ARRAY_PRINT_CHAR`
    - `ARRAY_PRINT_INT`
    - `ARRAY_PRINT_LONG`
    - `ARRAY_PRINT_LLONG`
    - `ARRAY_PRINT_FLOAT`
    - `ARRAY_PRINT_DOUBLE`
    - `ARRAY_PRINT_STRING`

  **Структуры:**</br>
  struct `Array`:
  - Структура массива.
  - `void *data;` - Базовый адрес блока элементов.
  - `size_t item_size;` - Размер одного элемента.
  - `size_t len;` - Длина массива (сколько ячеек занято).
  - `size_t capacity;` - Всего выделенных ячеек в памяти (вместимость).
  - `size_t init_cap;` - Размер массива по умолчанию.

  **Типы данных:**</br>
  typedef `Array`:
  - Динамический массив.
  - Объявление: `typedef struct Array Array;`

  **Функции:**</br>
  > Создать массив с заданным размером:</br>
  > `Array* Array_create(size_t item_size, size_t initial_capacity);`

  > Уничтожить массив:</br>
  > `void Array_destroy(Array **arr);`

  > Расширяем массив:</br>
  > `void Array_growth(Array *arr, float factor);`

  > Сжимаем массив:</br>
  > `void Array_shrink(Array *arr, float factor);`

  > Добавить элемент в массив (передайте указатель на данные, которые надо скопировать внутрь массива):</br>
  > `void Array_push(Array *arr, const void *element);`

  > Перезаписать элемент в массиве:</br>
  > `void Array_set(Array *arr, size_t index, const void *element);`

  > Получение элемента по индексу (адрес ячейки):</br>
  > `void* Array_get(Array *arr, size_t index);`

  > Получение элемента по индексу (сам указатель):</br>
  > `void* Array_get_ptr(Array *arr, size_t index);`

  > Получение элемента по индексу по кругу (индексация замкнута, включая отрицательные индексы):</br>
  > `void* Array_get_round(Array *arr, int64_t index);`

  > Вставка элемента по индексу со сдвигом:</br>
  > `void Array_insert(Array *arr, size_t index, const void *element);`

  > Переворот массива:</br>
  > `void Array_reverse(Array *arr);`

  > Заполнить массив элементами:</br>
  > `void Array_fill(Array *arr, const void *element, size_t count);`

  > Копировать массив:</br>
  > `void Array_copy(Array *dst, Array *src);`

  > Удаление элемента со сдвигом:</br>
  > `void Array_remove(Array *arr, size_t index, void *out);`

  > Удаление элемента без сдвига, заменяем удаляемый, последним элементом (порядок нарушается, зато быстро):</br>
  > `void Array_remove_swap(Array *arr, size_t index, void *out);`

  > Получить и удалить последний элемент из массива (без alloc):</br>
  > `void Array_pop(Array *arr, void *out);`

  > Получить и удалить последний элемент из массива (alloc с копированием):</br>
  > `void* Array_pop_copy(Array *arr);`

  > Печать содержимого массива:</br>
  > `void Array_print(Array *arr, FILE *out, ArrayPrintMode mode);`

  > Получить длину массива:</br>
  > `size_t Array_len(Array *arr);`

  > Получить размер массива:</br>
  > `size_t Array_capacity(Array *arr);`

  > Очистка массива:</br>
  > `void Array_clear(Array *arr, bool free_data);`


<a id="api-src-cgdf-core-constants-h"></a>
- ### constants.h:
  > Описание: Определяет константы.

  [Назад](#content)

  **Определения:**</br>
  `LOG_FILE_PATH`:
  - Путь файла краха.
  - Значение: `"data/last.log"`.


<a id="api-src-cgdf-core-core-h"></a>
- ### core.h:
  > Описание: Заголовочный файл ядра. Подключает все части ядра здесь.

  [Назад](#content)

  **Функции:**</br>
  > Инициализация ядра:</br>
  > `static inline bool core_init(void);`


<a id="api-src-cgdf-core-files-h"></a>
- ### files.h:
  [Назад](#content)

  **Функции:**</br>
  > Код для исправления путей для OS X:</br>
  > `void Files_fix_apple_path();`

  > Получить текущую директорию:</br>
  > `char *Files_get_cwd(char *buf, size_t size);`

  > Переход к каталогу:</br>
  > `bool Files_chdir(const char *path);`

  > Получить путь домашнего каталога:</br>
  > `char *Files_get_home();`

  > Загружаем файл в строку:</br>
  > `char* Files_load(const char* file_path, const char* mode);`

  > Сохраняем строку в файл:</br>
  > `bool Files_save(const char* file_path, const char* data, const char* mode);`

  > Загружаем файл в буфер бинарно:</br>
  > `unsigned char* Files_load_bin(const char* file_path, const char* mode, size_t* out_size);`

  > Сохраняем буфер в файл бинарно:</br>
  > `bool Files_save_bin(const char* file_path, const void* data, size_t size, const char* mode);`


<a id="api-src-cgdf-core-hashtable-h"></a>
- ### hashtable.h:
  [Назад](#content)

  **Определения:**</br>
  `HASHTABLE_DEFAULT_CAPACITY`:
  - Размер хэш-таблицы по-умолчанию.
  - Значение: `4096`.

  `HASHTABLE_MIN_CAPACITY`:
  - Минимальный размер хэш-таблицы.
  - Значение: `1024`.

  `HASHTABLE_GROWTH_FACTOR`:
  - Коэффициент увеличения таблицы.
  - Значение: `2`.

  `HASHTABLE_SHRINK_FACTOR`:
  - Коэффициент сжатия таблицы (формула: cap = len*SHRINK_FACTOR).
  - Значение: `2`.

  `HASHTABLE_GROWTH_THRESHOLD`:
  - Порог количества заполненности таблицы для расширения (%).
  - Значение: `0.66`.

  `HASHTABLE_SHRINK_THRESHOLD`:
  - Порог количества заполненности таблицы для сжатия (%).
  - Значение: `0.25`.

  `HASHTABLE_PROBING_COUNT`:
  - Массив записей последних пробирований (аналитика).
  - Значение: `64`.

  `HASHTABLE_PROBING_LIMIT`:
  - Лимит пробирований при поиске слота.
  - Значение: `32`.

  **Перечисления:**</br>
  enum `HashTablePrintMode`:
  - Перечисление режимов печати.
  - Значения:
    - `HASHTABLE_PRINT_PTR`
    - `HASHTABLE_PRINT_HEX`
    - `HASHTABLE_PRINT_BOOL`
    - `HASHTABLE_PRINT_CHAR`
    - `HASHTABLE_PRINT_INT`
    - `HASHTABLE_PRINT_LONG`
    - `HASHTABLE_PRINT_LLONG`
    - `HASHTABLE_PRINT_FLOAT`
    - `HASHTABLE_PRINT_DOUBLE`
    - `HASHTABLE_PRINT_STRING`

  **Структуры:**</br>
  struct `HashSlot`:
  - Структура слота таблицы.
  - `void  *key;` - Указатель на ключ.
  - `size_t key_size;` - Размер блока ключа.
  - `void  *value;` - Указатель на значение.
  - `size_t value_size;` - Размер блока значения.
  - `size_t hash;` - Хэш ключа (высчитывается один раз для оптимизации).
  - `bool   deleted;` - Флаг удаления (нужен для различия удалённых слотов от пустых).

  struct `HashTable`:
  - Структура хэш-таблицы.
  - `HashSlot *data;` - Таблица слотов.
  - `size_t   len;` - Длина таблицы (сколько ячеек занято).
  - `size_t   capacity;` - Всего выделенных ячеек в памяти (вместимость).
  - `size_t   prob_count[HASHTABLE_PROBING_COUNT];` - Количество пробирований (поиск слота).
  - `size_t   prob_index;` - Индекс (счетчик) в массиве prob_count.

  **Типы данных:**</br>
  typedef `HashSlot`:
  - Слот в таблице.
  - Объявление: `typedef struct HashSlot HashSlot;`

  typedef `HashTable`:
  - Хэш-таблица.
  - Объявление: `typedef struct HashTable HashTable;`

  **Функции:**</br>
  > Функция хэша на основе FNV-1a:</br>
  > `static inline uint64_t hash_fnv1a(const void* data, size_t len);`

  > Создать хэш-таблицу:</br>
  > `HashTable* HashTable_create();`

  > Уничтожить хэш-таблицу (не удаляет блоки по указателям):</br>
  > `void HashTable_destroy(HashTable **table);`

  > Добавить элемент в хэш-таблицу или обновить его значение:</br>
  > `bool HashTable_set(HashTable *table, const void *key, size_t key_size, const void *value, size_t value_size);`

  > Получить элемент по ключу. Возвращает указатель на value, иначе NULL:</br>
  > `void* HashTable_get(HashTable *table, const void *key, size_t key_size, size_t *out_value_size);`

  > Получить слот из таблицы по индексу:</br>
  > `HashSlot* HashTable_get_slot(HashTable *table, size_t index);`

  > Удалить элемент из таблицы:</br>
  > `bool HashTable_remove(HashTable *table, const void *key, size_t key_size, bool free_data);`

  > Возвращает true, если ключ есть в таблице:</br>
  > `bool HashTable_has(HashTable *table, const void *key, size_t key_size);`

  > Получить длину таблицы:</br>
  > `size_t HashTable_len(HashTable *table);`

  > Получить вместимость таблицы:</br>
  > `size_t HashTable_capacity(HashTable *table);`

  > Вывести содержимое таблицы:</br>
  > `void HashTable_print(HashTable *table, FILE *out, HashTablePrintMode key_mode, HashTablePrintMode value_mode);`

  > Очистить таблицу (без освобождения памяти по умолчанию):</br>
  > `void HashTable_clear(HashTable *table, bool free_data);`


<a id="api-src-cgdf-core-logger-h"></a>
- ### logger.h:
  [Назад](#content)

  **Функции:**</br>
  > Инициализация логгера:</br>
  > `void logger_init();`

  > Вывод сообщения в лог-файл и в консоль:</br>
  > `void log_msg(const char *fmt, ...);`


<a id="api-src-cgdf-core-math-h"></a>
- ### math.h:
  > Описание: Заголовочный файл, который определяет включения математики и определения других типов данных.

  [Назад](#content)

  **Функции:**</br>
  > Перевести градусы в радианы:</br>
  > `static inline double radians(double degrees);`

  > Перевести радианы в градусы:</br>
  > `static inline double degrees(double radians);`

  > Сравнение двух вещественных чисел:</br>
  > `static inline bool cmp_float(float a, float b);`

  > Зациклить вещественное число:</br>
  > `static inline float wrap_float(float v, float min, float max);`

  > Нормализовать угол:</br>
  > `static inline double normalize_deg(double a);`


<a id="api-src-cgdf-core-mm-h"></a>
- ### mm.h:
  [Назад](#content)

  **Функции:**</br>
  > Получить размер заголовка блока в байтах:</br>
  > `size_t mm_get_block_header_size();`

  > Получить количество выделенных блоков:</br>
  > `size_t mm_get_allocated_blocks();`

  > Получить абсолютный размер используемой памяти в байтах с учётом заголовков блоков:</br>
  > `size_t mm_get_absolute_used_size();`

  > Получить сколько всего используется памяти в байтах этим менеджером памяти:</br>
  > `size_t mm_get_used_size();`

  > Получить сколько всего используется памяти в килобайтах этим менеджером памяти:</br>
  > `double mm_get_used_size_kb();`

  > Получить сколько всего используется памяти в мегабайтах этим менеджером памяти:</br>
  > `double mm_get_used_size_mb();`

  > Получить сколько всего используется памяти в гигабайтах этим менеджером памяти:</br>
  > `double mm_get_used_size_gb();`

  > Получить размер блока в байтах:</br>
  > `size_t mm_get_block_size(void *ptr);`

  > Добавить байты к использованной памяти (атомарно):</br>
  > `void mm_used_size_add(size_t size);`

  > Вычесть байты из использованной памяти (атомарно):</br>
  > `void mm_used_size_sub(size_t size);`

  > Выделение памяти:</br>
  > `void* mm_alloc(size_t size);`

  > Выделение памяти с обнулением:</br>
  > `void* mm_calloc(size_t count, size_t size);`

  > Расширение блока памяти:</br>
  > `void* mm_realloc(void *ptr, size_t new_size);`

  > Копирование строки:</br>
  > `char* mm_strdup(const char *str);`

  > Освобождение памяти:</br>
  > `void mm_free(void *ptr);`

  > Вызовите если получите проблему при выделении памяти:</br>
  > `void mm_alloc_error();`


<a id="api-src-cgdf-core-pixmap-h"></a>
- ### pixmap.h:
  [Назад](#content)

  **Определения:**</br>
  `PIXMAP_R`:
  - Размеры каналов в байтах.
  - Значение: `1`.

  `PIXMAP_RG`:
  - Значение: `2`.

  `PIXMAP_RGB`:
  - Значение: `3`.

  `PIXMAP_RGBA`:
  - Значение: `4`.

  **Структуры:**</br>
  struct `Pixmap`:
  - Структура картинки.
  - `int width;` - Ширина картинки.
  - `int height;` - Высота картинки.
  - `int channels;` - Количество байт на пиксель.
  - `bool from_stbi;` - Флаг, что картинка загружена с помощью stbi.
  - `unsigned char* data;` - Указатель на блок данных.

  **Типы данных:**</br>
  typedef `Pixmap`:
  - Картинка.
  - Объявление: `typedef struct Pixmap Pixmap;`

  **Функции:**</br>
  > Создать картинку:</br>
  > `Pixmap* Pixmap_create(int width, int height, int channels);`

  > Уничтожить картинку:</br>
  > `void Pixmap_destroy(Pixmap **pixmap);`

  > Загрузить картинку:</br>
  > `Pixmap* Pixmap_load(const char *filepath, int format);`

  > Сохранить картинку:</br>
  > `bool Pixmap_save(Pixmap *pixmap, const char *filepath, const char *format);`

  > Копировать картинку в памяти:</br>
  > `Pixmap* Pixmap_copy(const Pixmap *source);`

  > Создать стандартную картинку:</br>
  > `Pixmap* Pixmap_create_default();`

  > Получить размер картинки в байтах:</br>
  > `size_t Pixmap_get_size(Pixmap *pixmap);`

  **Глобальные переменные:**</br>
  `Pixmap_default_icon`:
  - Определяем глобальные переменные стандартной картинки.
  - Объявление: `extern const unsigned char Pixmap_default_icon[];`.

  `Pixmap_default_icon_size`:
  - Объявление: `extern const size_t Pixmap_default_icon_size;`.

  `Pixmap_default_icon_width`:
  - Объявление: `extern const int Pixmap_default_icon_width;`.

  `Pixmap_default_icon_height`:
  - Объявление: `extern const int Pixmap_default_icon_height;`.


<a id="api-src-cgdf-core-platform-h"></a>
- ### platform.h:
  > Описание: Определяет платформу, на которой компилируется код.

  [Назад](#content)

  **Функции:**</br>
  > Платформа Windows:</br>
  > `static inline bool is_windows(void);`

  > Платформа MacOS:</br>
  > `static inline bool is_macos(void);`

  > Платформа Linux:</br>
  > `static inline bool is_linux(void);`

  > Получить название платформы:</br>
  > `static inline const char* get_platform_name(void);`


<a id="api-src-cgdf-core-time-h"></a>
- ### time.h:
  > Описание: Заголовок с полезными кроссплатформенными способами работы с временем.

  [Назад](#content)

  **Структуры:**</br>
  struct `TimeCurrent`:
  - Текущее время.
  - `uint64_t ticks;`.
  - `uint32_t year;`.
  - `uint32_t month;`.
  - `uint32_t day;`.
  - `uint32_t hour;`.
  - `uint32_t min;`.
  - `uint32_t sec;`.
  - `uint32_t ms;`.

  struct `TimeOffsetUTC`:
  - Смещение пояса UTC.
  - `int32_t offset;`.
  - `int32_t hour;`.
  - `int32_t min;`.

  **Типы данных:**</br>
  typedef `TimeCurrent`:
  - Текущее время.
  - Объявление: `typedef struct TimeCurrent TimeCurrent;`

  typedef `TimeOffsetUTC`:
  - Смещение пояса UTC.
  - Объявление: `typedef struct TimeOffsetUTC TimeOffsetUTC;`

  **Функции:**</br>
  > Возвращает время с начала Unix-эпохи в секундах (double) с точностью до мс:</br>
  > `static inline double Time_now(double *x);`

  > Остановить выполнение кода на определенное время в секундах с дробной частью (малая точность чем у Time_delay):</br>
  > `static inline void Time_sleep(double seconds);`

  > Задержать выполнение кода на определенное время в секундах с дробной частью (большая точность чем у Time_sleep) Комбинированный sleep (по возможности) + active wait для докрутки ожидания в коде:</br>
  > `static inline void Time_delay(double seconds);`

  > Узнать у устройства смещение часового времени:</br>
  > `static inline TimeOffsetUTC Time_get_utc_offset();`

  > Функция `Time_get_current`:</br>
  > `static inline TimeCurrent Time_get_current(bool local_time);`

  > Инициализация времени:</br>
  > `static inline void Time_init();`


<a id="api-src-cgdf-core-vector-h"></a>
- ### vector.h:
  > Описание: Реализует базовую простую работу с векторами.

  [Назад](#content)

  **Структуры:**</br>
  struct `Vec2i`:
  - Вектор двумерный целочисленный.
  - `int x, y;`.

  struct `Vec3i`:
  - Вектор трехмерный целочисленный.
  - `int x, y, z;`.

  struct `Vec4i`:
  - Вектор четырехмерный целочисленный.
  - `int x, y, z, w;`.

  struct `Vec2f`:
  - Вектор двумерный вещественный.
  - `float x, y;`.

  struct `Vec3f`:
  - Вектор трехмерный вещественный.
  - `float x, y, z;`.

  struct `Vec4f`:
  - Вектор четырехмерный вещественный.
  - `float x, y, z, w;`.

  struct `Vec2d`:
  - Вектор двумерный вещественный с двойной точностью.
  - `double x, y;`.

  struct `Vec3d`:
  - Вектор трехмерный вещественный с двойной точностью.
  - `double x, y, z;`.

  struct `Vec4d`:
  - Вектор четырехмерный вещественный с двойной точностью.
  - `double x, y, z, w;`.

  **Функции:**</br>
  > Функция `Vec2i_sub`:</br>
  > `static inline void Vec2i_sub(Vec2i *a, Vec2i b);`

  > Функция `Vec2i_add`:</br>
  > `static inline void Vec2i_add(Vec2i *a, Vec2i b);`

  > Функция `Vec2i_mul`:</br>
  > `static inline void Vec2i_mul(Vec2i *a, Vec2i b);`

  > Функция `Vec2i_div`:</br>
  > `static inline void Vec2i_div(Vec2i *a, Vec2i b);`

  > Функция `Vec3i_sub`:</br>
  > `static inline void Vec3i_sub(Vec3i *a, Vec3i b);`

  > Функция `Vec3i_add`:</br>
  > `static inline void Vec3i_add(Vec3i *a, Vec3i b);`

  > Функция `Vec3i_mul`:</br>
  > `static inline void Vec3i_mul(Vec3i *a, Vec3i b);`

  > Функция `Vec3i_div`:</br>
  > `static inline void Vec3i_div(Vec3i *a, Vec3i b);`

  > Функция `Vec4i_sub`:</br>
  > `static inline void Vec4i_sub(Vec4i *a, Vec4i b);`

  > Функция `Vec4i_add`:</br>
  > `static inline void Vec4i_add(Vec4i *a, Vec4i b);`

  > Функция `Vec4i_mul`:</br>
  > `static inline void Vec4i_mul(Vec4i *a, Vec4i b);`

  > Функция `Vec4i_div`:</br>
  > `static inline void Vec4i_div(Vec4i *a, Vec4i b);`

  > Функция `Vec2f_sub`:</br>
  > `static inline void Vec2f_sub(Vec2f *a, Vec2f b);`

  > Функция `Vec2f_add`:</br>
  > `static inline void Vec2f_add(Vec2f *a, Vec2f b);`

  > Функция `Vec2f_mul`:</br>
  > `static inline void Vec2f_mul(Vec2f *a, Vec2f b);`

  > Функция `Vec2f_div`:</br>
  > `static inline void Vec2f_div(Vec2f *a, Vec2f b);`

  > Функция `Vec3f_sub`:</br>
  > `static inline void Vec3f_sub(Vec3f *a, Vec3f b);`

  > Функция `Vec3f_add`:</br>
  > `static inline void Vec3f_add(Vec3f *a, Vec3f b);`

  > Функция `Vec3f_mul`:</br>
  > `static inline void Vec3f_mul(Vec3f *a, Vec3f b);`

  > Функция `Vec3f_div`:</br>
  > `static inline void Vec3f_div(Vec3f *a, Vec3f b);`

  > Функция `Vec4f_sub`:</br>
  > `static inline void Vec4f_sub(Vec4f *a, Vec4f b);`

  > Функция `Vec4f_add`:</br>
  > `static inline void Vec4f_add(Vec4f *a, Vec4f b);`

  > Функция `Vec4f_mul`:</br>
  > `static inline void Vec4f_mul(Vec4f *a, Vec4f b);`

  > Функция `Vec4f_div`:</br>
  > `static inline void Vec4f_div(Vec4f *a, Vec4f b);`

  > Функция `Vec2d_sub`:</br>
  > `static inline void Vec2d_sub(Vec2d *a, Vec2d b);`

  > Функция `Vec2d_add`:</br>
  > `static inline void Vec2d_add(Vec2d *a, Vec2d b);`

  > Функция `Vec2d_mul`:</br>
  > `static inline void Vec2d_mul(Vec2d *a, Vec2d b);`

  > Функция `Vec2d_div`:</br>
  > `static inline void Vec2d_div(Vec2d *a, Vec2d b);`

  > Функция `Vec3d_sub`:</br>
  > `static inline void Vec3d_sub(Vec3d *a, Vec3d b);`

  > Функция `Vec3d_add`:</br>
  > `static inline void Vec3d_add(Vec3d *a, Vec3d b);`

  > Функция `Vec3d_mul`:</br>
  > `static inline void Vec3d_mul(Vec3d *a, Vec3d b);`

  > Функция `Vec3d_div`:</br>
  > `static inline void Vec3d_div(Vec3d *a, Vec3d b);`

  > Функция `Vec4d_sub`:</br>
  > `static inline void Vec4d_sub(Vec4d *a, Vec4d b);`

  > Функция `Vec4d_add`:</br>
  > `static inline void Vec4d_add(Vec4d *a, Vec4d b);`

  > Функция `Vec4d_mul`:</br>
  > `static inline void Vec4d_mul(Vec4d *a, Vec4d b);`

  > Функция `Vec4d_div`:</br>
  > `static inline void Vec4d_div(Vec4d *a, Vec4d b);`

#

### src/cgdf/graphics/core/

<a id="api-src-cgdf-graphics-core-animator-h"></a>
- ### animator.h:
  > Описание: Определяет апи для работы с анимацией.

  [Назад](#content)

  **Структуры:**</br>
  struct `FrameAnimator2D`:
  - Структура 2D кадрового аниматора.
  - `uint32_t frames;` - Количество кадров анимации.
  - `float    duration;` - Продолжительность кадра в секундах.
  - `float    count;` - Счётчик кадров.
  - `bool     _paused_;` - Анимация приостановлена.

  **Типы данных:**</br>
  typedef `FrameAnimator2D`:
  - 2D кадровый аниматор.
  - Объявление: `typedef struct FrameAnimator2D FrameAnimator2D;`

  **Функции:**</br>
  > Создать кадровый аниматор:</br>
  > `FrameAnimator2D* FrameAnimator2D_create(uint32_t frames, float duration);`

  > Уничтожить кадровый аниматор:</br>
  > `void FrameAnimator2D_destroy(FrameAnimator2D **animator);`

  > Обновить анимацию:</br>
  > `void FrameAnimator2D_update(FrameAnimator2D *self, float dtime);`

  > Запустить анимацию:</br>
  > `void FrameAnimator2D_start(FrameAnimator2D *self);`

  > Остановить анимацию и вернуть к первому кадру:</br>
  > `void FrameAnimator2D_stop(FrameAnimator2D *self);`

  > Остановить анимацию:</br>
  > `void FrameAnimator2D_pause(FrameAnimator2D *self);`

  > Возобновить анимацию:</br>
  > `void FrameAnimator2D_resume(FrameAnimator2D *self);`

  > Вернуть к первому кадру:</br>
  > `void FrameAnimator2D_reset(FrameAnimator2D *self);`

  > Получить активность анимации:</br>
  > `bool FrameAnimator2D_get_active(FrameAnimator2D *self);`

  > Получить кадр анимации:</br>
  > `int FrameAnimator2D_get_frame(FrameAnimator2D *self);`


<a id="api-src-cgdf-graphics-core-camera-h"></a>
- ### camera.h:
  > Описание: Определяет функционал 2D и 3D камеры.

  [Назад](#content)

  **Структуры:**</br>
  struct `Camera2D`:
  - Структура 2D камеры.
  - `mat4 view;` - Матрица вида.
  - `mat4 proj;` - Матрица проекции.
  - `mat4 old_view;` - Старая матрица вида (используется при ui_begin/end).
  - `mat4 old_proj;` - Старая матрица проекции (используется при ui_begin/end).
  - `Window *window;` - Указатель на окно.
  - `Vec2d position;` - Позиция камеры.
  - `float angle;` - Угол наклона камеры.
  - `float zoom;` - Масштаб камеры.
  - `float meter;` - Масштаб единицы измерения.
  - `bool _ui_begin_;` - Отрисовывается ли интерфейс.

  struct `Camera3D`:
  - Структура 3D камеры.
  - `mat4 view;` - Матрица вида.
  - `mat4 proj;` - Матрица проекции.
  - `Window *window;` - Указатель на окно.
  - `Vec3d position;` - Позиция камеры.
  - `Vec3d rotation;` - Поворот камеры (x=pitch, y=yaw, z=roll).
  - `Vec3d size;` - Размер камеры.
  - `float fov;` - Угол обзора.
  - `float z_far;` - Дальняя плоскость отсечения.
  - `float z_near;` - Ближняя плоскость отсечения.
  - `bool is_ortho;` - Ортографическая камера.
  - `int width;` - Ширина.
  - `int height;` - Высота.
  - `float _oldfov_;` - Старый угол обзора.
  - `float _oldfar_;` - Старая дальняя плоскость отсечения.
  - `float _oldnear_;` - Старая ближняя плоскость отсечения.

  **Типы данных:**</br>
  typedef `Camera2D`:
  - 2D Камера.
  - Объявление: `typedef struct Camera2D Camera2D;`

  typedef `Camera3D`:
  - 3D Камера.
  - Объявление: `typedef struct Camera3D Camera3D;`

  **Функции:**</br>
  > Создать 2D камеру:</br>
  > `Camera2D* Camera2D_create(Window *window, int width, int height, Vec2d position, float angle, float zoom);`

  > Уничтожить 2D камеру:</br>
  > `void Camera2D_destroy(Camera2D **camera);`

  > Обновление камеры:</br>
  > `void Camera2D_update(Camera2D *self);`

  > Изменить размер камеры:</br>
  > `void Camera2D_resize(Camera2D *self, int width, int height);`

  > Изменить масштаб единицы измерения:</br>
  > `void Camera2D_set_meter(Camera2D *self, float meter);`

  > Начало отрисовки UI:</br>
  > `void Camera2D_ui_begin(Camera2D *self);`

  > Конец отрисовки UI:</br>
  > `void Camera2D_ui_end(Camera2D *self);`

  > Создать 3D камеру:</br>
  > `Camera3D* Camera3D_create( Window *window, int width, int height, Vec3d position, Vec3d rotation, Vec3d size, float fov, float z_near, float z_far, bool ortho );`

  > Уничтожить 3D камеру:</br>
  > `void Camera3D_destroy(Camera3D **camera);`

  > Обновление камеры:</br>
  > `void Camera3D_update(Camera3D *self);`

  > Изменить размер камеры:</br>
  > `void Camera3D_resize(Camera3D *self, int width, int height, bool ortho);`

  > Посмотреть на указанную точку:</br>
  > `void Camera3D_look_at(Camera3D *self, Vec3d target);`

  > Установить проверку глубины:</br>
  > `void Camera3D_set_depth_test(Camera3D *self, bool enabled);`

  > Включить или отключить запись глубины:</br>
  > `void Camera3D_set_depth_mask(Camera3D *self, bool enabled);`

  > Включить или отключить смешивание:</br>
  > `void Camera3D_set_blending(Camera3D *self, bool enabled);`

  > Установить отсечение граней:</br>
  > `void Camera3D_set_cull_faces(Camera3D *self, bool enabled);`

  > Отсекать только задние грани:</br>
  > `void Camera3D_set_back_face_culling(Camera3D *self);`

  > Отсекать только передние грани:</br>
  > `void Camera3D_set_front_face_culling(Camera3D *self);`

  > Передняя грань против часовой стрелки (CCW):</br>
  > `void Camera3D_set_front_face_onleft(Camera3D *self);`

  > Передняя грань по часовой стрелке (CW):</br>
  > `void Camera3D_set_front_face_onright(Camera3D *self);`

  > Установить ортографическую проекцию:</br>
  > `void Camera3D_set_ortho(Camera3D *self, bool enabled);`

  > Узнать включена ли ортографическая проекция:</br>
  > `bool Camera3D_get_ortho(Camera3D *self);`


<a id="api-src-cgdf-graphics-core-draw-h"></a>
- ### draw.h:
  > Описание: Создаёт общий апи для отрисовки примитивов.

  [Назад](#content)

  **Типы данных:**</br>
  typedef `SimpleDraw`:
  - Простая отрисовка примитивов.
  - Объявление: `typedef struct SimpleDraw SimpleDraw;`

  **Функции:**</br>
  > Создать простую отрисовку примитивов:</br>
  > `SimpleDraw* SimpleDraw_create(Renderer *renderer);`

  > Уничтожить простую отрисовку примитивов:</br>
  > `void SimpleDraw_destroy(SimpleDraw **draw);`

  > Нарисовать точку:</br>
  > `void SimpleDraw_point(SimpleDraw *self, Vec4f color, Vec3f point, float size);`

  > Нарисовать точки:</br>
  > `void SimpleDraw_points(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count, float size);`

  > Нарисовать линию:</br>
  > `void SimpleDraw_line(SimpleDraw *self, Vec4f color, Vec3f start, Vec3f end, float width);`

  > Нарисовать линии:</br>
  > `void SimpleDraw_lines(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count, float width);`

  > Нарисовать ломаную линию:</br>
  > `void SimpleDraw_line_strip(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count, float width);`

  > Нарисовать замкнутую ломаную линию:</br>
  > `void SimpleDraw_line_loop(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count, float width);`

  > Нарисовать треугольники:</br>
  > `void SimpleDraw_triangles(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count);`

  > Нарисовать треугольники с общей стороной:</br>
  > `void SimpleDraw_triangle_strip(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count);`

  > Нарисовать треугольники последняя вершина которой будет соединена с первой:</br>
  > `void SimpleDraw_triangle_fan(SimpleDraw *self, Vec4f color, Vec3f *points, uint32_t count);`

  > Нарисовать квадрат:</br>
  > `void SimpleDraw_quad(SimpleDraw *self, Vec4f color, Vec3f point, Vec2f size, float width);`

  > Нарисовать квадрат с заливкой:</br>
  > `void SimpleDraw_quad_fill(SimpleDraw *self, Vec4f color, Vec3f point, Vec2f size);`

  > Нарисовать круг:</br>
  > `void SimpleDraw_circle(SimpleDraw *self, Vec4f color, Vec3f center, float radius, uint32_t num_verts, float width);`

  > Нарисовать круг с заливкой:</br>
  > `void SimpleDraw_circle_fill(SimpleDraw *self, Vec4f color, Vec3f center, float radius, uint32_t num_verts);`

  > Нарисовать звезду:</br>
  > `void SimpleDraw_star( SimpleDraw *self, Vec4f color, Vec3f center, float outradius, float inradius, uint32_t num_verts, float width );`

  > Нарисовать звезду с заливкой:</br>
  > `void SimpleDraw_star_fill( SimpleDraw *self, Vec4f color, Vec3f center, float outradius, float inradius, uint32_t num_verts );`


<a id="api-src-cgdf-graphics-core-font-h"></a>
- ### font.h:
  > Описание: Функционал для рендеринга текста.

  [Назад](#content)

  **Определения:**</br>
  `FONT_ATLAS_SIZE`:
  - Размер атласа по умолчанию (в глифах).
  - Значение: `1024`.

  `FONT_ATLAS_PADDING`:
  - Отступ между глифами со всех сторон (в пикселях).
  - Значение: `1`.

  `FONT_ATLAS_SCALING`:
  - Масштабирование атласа при расширении.
  - Значение: `1.5`.

  `FONT_FALLBACK_SUMB`:
  - Замена нераспознанных символов.
  - Значение: `'?'`.

  **Перечисления:**</br>
  enum `FontAlign`:
  - Перечисление точек центрирования.
  - Значения:
    - `FONT_ALIGN_BOTTOM_LEFT`
    - `FONT_ALIGN_BOTTOM_CENTER`
    - `FONT_ALIGN_BOTTOM_RIGHT`
    - `FONT_ALIGN_CENTER_LEFT`
    - `FONT_ALIGN_CENTER_CENTER`
    - `FONT_ALIGN_CENTER_RIGHT`
    - `FONT_ALIGN_TOP_LEFT`
    - `FONT_ALIGN_TOP_CENTER`
    - `FONT_ALIGN_TOP_RIGHT`
    - `FONT_ALIGN_COUNT`

  **Структуры:**</br>
  struct `FontPixmap`:
  - Растровый шрифт.
  - `stbtt_fontinfo info;` - Информация о шрифте.
  - `Renderer       *renderer;` - Рендерер.
  - `Texture        *atlas;` - Атлас. Текстура с нашими символами.
  - `SpriteBatch    *batch;` - Пакетная отрисовка спрайтов (для нас - символов).
  - `Array          *glyphs_array;` - Массив глифов (просто коды символов). Нужен для расширения атласа.
  - `HashTable      *glyphs;` - Хэш-таблица глифов (символов). Нужен для немедленного доступа к глифам.
  - `int added_glyphs_count;` - Сколько глифов было добавлено в атлас. Нужен для авто-расширения атласа.
  - `unsigned char  *ttf_buffer;` - Буфер данных файла шрифта.
  - `Vec4f     color;` - Цвет текста.
  - `Vec4f     bg_color;` - Фоновый цвет.
  - `Vec4f     bg_padding;` - Отступы фона (left, top, right, bottom).
  - `Vec2f     scale_factor;` - Коэффициент масштабирования.
  - `bool      pixelized;` - Пикселизированный шрифт.
  - `FontAlign align;` - Выравнивание текста.
  - `int   font_size;` - [don't edit] Размер шрифта (по высоте в пикселях).
  - `float scale;` - [don't edit] Коэффициент масштабирования (только для перевода внутренних единиц в пиксели).
  - `int   ascent;` - [don't edit] Верхняя часть символа.
  - `int   descent;` - [don't edit] Нижняя часть символа.
  - `int   line_gap;` - [don't edit] Расстояние между строками.
  - `float line_advance;` - [don't edit] Базовый шаг строки из шрифта (px). Используется при '\n'.
  - `float line_height;` - [editable] Смещение межстрочного интервала. Используется при '\n'.
  - `float letter_spacing;` - [editable] Дополнительный межбуквенный интервал (px).
  - `int   tab_size;` - [editable] Сколько пробелов в '\t'.
  - `float space_advance;` - [editable] Ширина пробела (px).

  struct `FontGlyph`:
  - Глиф.
  - `float u0, v0;` - Лево-низ символа.
  - `float u1, v1;` - Право-верх символа.
  - `int   width;` - Ширина символа.
  - `int   height;` - Высота символа.
  - `float offset_x;` - Смещение символа по ширине.
  - `float offset_y;` - Смещение символа по высоте.
  - `float advance;` - Насколько сдвигать курсор при отрисовке.

  struct `FontTextBlock`:
  - Блок текста (размеры).
  - `Vec2f start;` - Левый нижний угол прямоугольника.
  - `Vec2f end;` - Правый верхний угол прямоугольника.
  - `Vec2f size;` - Размер прямоугольника (ширина и высота).
  - `int   lines;` - Количество строк в блоке.

  **Типы данных:**</br>
  typedef `FontPixmap`:
  - Растровый шрифт.
  - Объявление: `typedef struct FontPixmap FontPixmap;`

  typedef `FontGlyph`:
  - Глиф.
  - Объявление: `typedef struct FontGlyph FontGlyph;`

  typedef `FontTextBlock`:
  - Блок текста (размеры).
  - Объявление: `typedef struct FontTextBlock FontTextBlock;`

  **Функции:**</br>
  > Создать растровый шрифт:</br>
  > `FontPixmap* FontPixmap_create(Renderer *renderer, const char *file_path, int font_size);`

  > Уничтожить растровый шрифт:</br>
  > `void FontPixmap_destroy(FontPixmap **font);`

  > Получить глиф (из кэша либо создать новый):</br>
  > `FontGlyph* FontPixmap_get_glyph(FontPixmap *self, uint32_t codepoint);`

  > Установить цвет текста:</br>
  > `void FontPixmap_set_color(FontPixmap *self, Vec4f color);`

  > Получить цвет текста:</br>
  > `Vec4f FontPixmap_get_color(FontPixmap *self);`

  > Установить фоновый цвет текста:</br>
  > `void FontPixmap_set_bg_color(FontPixmap *self, Vec4f bg_color);`

  > Получить фоновый цвет текста:</br>
  > `Vec4f FontPixmap_get_bg_color(FontPixmap *self);`

  > Установить отступ фона:</br>
  > `void FontPixmap_set_bg_padding(FontPixmap *self, Vec4f bg_padding);`

  > Получить отступ фона:</br>
  > `Vec4f FontPixmap_get_bg_padding(FontPixmap *self);`

  > Установить коэффициент масштабирования:</br>
  > `void FontPixmap_set_scale_factor(FontPixmap *self, Vec2f scale_factor);`

  > Получить коэффициент масштабирования:</br>
  > `Vec2f FontPixmap_get_scale_factor(FontPixmap *self);`

  > Установить пикселизацию текста:</br>
  > `void FontPixmap_set_pixelized(FontPixmap *self, bool pixelized);`

  > Получить пикселизацию текста:</br>
  > `bool FontPixmap_get_pixelized(FontPixmap *self);`

  > Установить выравнивание блока текста:</br>
  > `void FontPixmap_set_align(FontPixmap *self, FontAlign align);`

  > Получить выравнивание блока текста:</br>
  > `FontAlign FontPixmap_get_align(FontPixmap *self);`

  > Установить смещение межстрочного интервала:</br>
  > `void FontPixmap_set_line_height(FontPixmap *self, float line_height);`

  > Получить смещение межстрочного интервала:</br>
  > `float FontPixmap_get_line_height(FontPixmap *self);`

  > Установить межбуквенный интервал:</br>
  > `void FontPixmap_set_letter_spacing(FontPixmap *self, float letter_spacing);`

  > Получить межбуквенный интервал:</br>
  > `float FontPixmap_get_letter_spacing(FontPixmap *self);`

  > Установить размер табуляции:</br>
  > `void FontPixmap_set_tab_size(FontPixmap *self, int tab_size);`

  > Получить размер табуляции:</br>
  > `int FontPixmap_get_tab_size(FontPixmap *self);`

  > Установить ширину пробела:</br>
  > `void FontPixmap_set_space_advance(FontPixmap *self, float space_advance);`

  > Получить ширину пробела:</br>
  > `float FontPixmap_get_space_advance(FontPixmap *self);`

  > Получить блок текста:</br>
  > `FontTextBlock FontPixmap_get_text_block(FontPixmap *self, const char *text, ...);`

  > Отрисовать текст:</br>
  > `void FontPixmap_render(FontPixmap *self, float x, float y, float angle, const char *text, ...);`


<a id="api-src-cgdf-graphics-core-input-h"></a>
- ### input.h:
  > Описание: Код для работы с вводом окна.

  [Назад](#content)

  **Перечисления:**</br>
  enum `Input_Scancode`:
  - Независимые сканкоды.
  - Значения:
    - `K_UNKNOWN` - unknown scancode.
    - `K_0` - 0.
    - `K_1` - 1.
    - `K_2` - 2.
    - `K_3` - 3.
    - `K_4` - 4.
    - `K_5` - 5.
    - `K_6` - 6.
    - `K_7` - 7.
    - `K_8` - 8.
    - `K_9` - 9.
    - `K_a` - A.
    - `K_b` - B.
    - `K_c` - C.
    - `K_d` - D.
    - `K_e` - E.
    - `K_f` - F.
    - `K_g` - G.
    - `K_h` - H.
    - `K_i` - I.
    - `K_j` - J.
    - `K_k` - K.
    - `K_l` - L.
    - `K_m` - M.
    - `K_n` - N.
    - `K_o` - O.
    - `K_p` - P.
    - `K_q` - Q.
    - `K_r` - R.
    - `K_s` - S.
    - `K_t` - T.
    - `K_u` - U.
    - `K_v` - V.
    - `K_w` - W.
    - `K_x` - X.
    - `K_y` - Y.
    - `K_z` - Z.
    - `K_KP_0` - Num 0.
    - `K_KP_1` - Num 1.
    - `K_KP_2` - Num 2.
    - `K_KP_3` - Num 3.
    - `K_KP_4` - Num 4.
    - `K_KP_5` - Num 5.
    - `K_KP_6` - Num 6.
    - `K_KP_7` - Num 7.
    - `K_KP_8` - Num 8.
    - `K_KP_9` - Num 9.
    - `K_KP_EXCLAIM` - !
    - `K_KP_HASH` - #.
    - `K_KP_AMPERSAND` - &.
    - `K_KP_LEFTPAREN` - (.
    - `K_KP_RIGHTPAREN` - ).
    - `K_KP_PERIOD` - Num .
    - `K_KP_DIVIDE` - Num /.
    - `K_KP_MULTIPLY` - Num *.
    - `K_KP_MINUS` - Num -.
    - `K_KP_PLUS` - Num +.
    - `K_KP_ENTER` - Num Enter.
    - `K_KP_EQUALS` - Num =.
    - `K_KP_COLON` - :.
    - `K_KP_LESS` - <.
    - `K_KP_GREATER` - >.
    - `K_KP_AT` - @.
    - `K_F1` - F1.
    - `K_F2` - F2.
    - `K_F3` - F3.
    - `K_F4` - F4.
    - `K_F5` - F5.
    - `K_F6` - F6.
    - `K_F7` - F7.
    - `K_F8` - F8.
    - `K_F9` - F9.
    - `K_F10` - F10.
    - `K_F11` - F11.
    - `K_F12` - F12.
    - `K_F13` - F13.
    - `K_F14` - F14.
    - `K_F15` - F15.
    - `K_BACKSPACE` - \b.
    - `K_TAB` - \t.
    - `K_CLEAR` - clear.
    - `K_RETURN` - \r.
    - `K_PAUSE` - pause.
    - `K_ESCAPE` - ^[.
    - `K_SPACE` - space.
    - `K_COMMA` - ,.
    - `K_MINUS` - -.
    - `K_PERIOD` - .
    - `K_SLASH` - /.
    - `K_SEMICOLON` - ;.
    - `K_EQUALS` - =.
    - `K_LEFTBRACKET` - [.
    - `K_BACKSLASH` - \.
    - `K_RIGHTBRACKET` - ].
    - `K_DELETE` - delete.
    - `K_UP` - ↑.
    - `K_DOWN` - ↓.
    - `K_RIGHT` - →.
    - `K_LEFT` - ←.
    - `K_INSERT` - insert.
    - `K_HOME` - home.
    - `K_END` - end.
    - `K_PAGEUP` - page up.
    - `K_PAGEDOWN` - page down.
    - `K_NUMLOCK` - numlock.
    - `K_CAPSLOCK` - capslock.
    - `K_SCROLLLOCK` - scroll lock.
    - `K_RSHIFT` - right shift.
    - `K_LSHIFT` - left shift.
    - `K_RCTRL` - right ctrl.
    - `K_LCTRL` - left ctrl.
    - `K_RALT` - right alt.
    - `K_LALT` - left alt.
    - `K_RGUI` - right Win/Cmd/Meta.
    - `K_LGUI` - left Win/Cmd/Meta.
    - `K_MODE` - mode shift.
    - `K_HELP` - help.
    - `K_PRINTSCREEN` - PrtSc.
    - `K_SYSREQ` - sysrq.
    - `K_MENU` - menu.
    - `K_POWER` - power.
    - `INPUT_SCANCODE_RESERVED` - 400-500 reserved for dynamic keycodes.
    - `INPUT_SCANCODE_COUNT` - Not a key, just a count of the number of scancodes.

  **Структуры:**</br>
  struct `Input_MouseState`:
  - Состояние мыши.
  - `int max_keys;` - Количество кнопок (обычно равно 8 на каждый из полей состояний).
  - `bool *pressed;` - Нажатые кнопки.
  - `bool *down;` - Нажатия в этом кадре.
  - `bool *up;` - Отпускания в этом кадре.
  - `bool focused;` - Находится ли курсор над окном.
  - `bool visible;` - Видимость курсора.
  - `Vec2i pos;` - Позиция курсора.
  - `Vec2i rel;` - Смещение за кадр.
  - `Vec2i wheel;` - Прокрутка колёсика мыши.

  struct `Input_KeyboardState`:
  - Состояние клавиатуры.
  - `int max_keys;` - Количество клавиш (обычно равно 512 на каждый из полей состояний).
  - `bool *pressed;` - Нажатые клавиши.
  - `bool *down;` - Нажатия в этом кадре.
  - `bool *up;` - Отпускания в этом кадре.

  struct `Input`:
  - Структура ввода.
  - `Input_MouseState    *mouse;` - Состояние мыши (параметры мыши и кнопок).
  - `Input_KeyboardState *keyboard;` - Состояние клавиатуры (параметры клавиш).
  - `void (*set_mouse_pos)     (Window *self, int x, int y);` - Установить позицию мыши.
  - `void (*set_mouse_visible) (Window *self, bool visible);` - Установить видимость мыши.

  **Типы данных:**</br>
  typedef `Input_MouseState`:
  - Ввод мыши.
  - Объявление: `typedef struct Input_MouseState Input_MouseState;`

  typedef `Input_KeyboardState`:
  - Ввод клавиатуры.
  - Объявление: `typedef struct Input_KeyboardState Input_KeyboardState;`

  typedef `Input`:
  - Система ввода.
  - Объявление: `typedef struct Input Input;`

  typedef `Window`:
  - Объявление: `typedef struct Window Window;`

  **Функции:**</br>
  > Создать структуру мыши:</br>
  > `Input_MouseState* Input_MouseState_create(int max_keys);`

  > Уничтожить структуру мыши:</br>
  > `void Input_MouseState_destroy(Input_MouseState **mouse_state);`

  > Создать структуру клавиатуры:</br>
  > `Input_KeyboardState* Input_KeyboardState_create(int max_keys);`

  > Уничтожить структуру клавиатуры:</br>
  > `void Input_KeyboardState_destroy(Input_KeyboardState **keyboard_state);`

  > Создать структуру ввода:</br>
  > `Input* Input_create( void (*set_mouse_pos) (Window *self, int x, int y), void (*set_mouse_visible) (Window *self, bool visible) );`

  > Уничтожить структуру ввода:</br>
  > `void Input_destroy(Input **input);`

  > Получить нажатые кнопки мыши:</br>
  > `bool* Input_get_mouse_pressed(Window *self);`

  > Получить нажатие кнопки мыши:</br>
  > `bool* Input_get_mouse_down(Window *self);`

  > Получить отжатие кнопки мыши:</br>
  > `bool* Input_get_mouse_up(Window *self);`

  > Получить смещение мыши за кадр:</br>
  > `Vec2i Input_get_mouse_rel(Window *self);`

  > Получить нахождение мыши над окном:</br>
  > `bool Input_get_mouse_focused(Window *self);`

  > Получить вращение колёсика мыши:</br>
  > `Vec2i Input_get_mouse_wheel(Window *self);`

  > Установить позицию мыши:</br>
  > `void Input_set_mouse_pos(Window *self, int x, int y);`

  > Получить позицию мыши:</br>
  > `Vec2i Input_get_mouse_pos(Window *self);`

  > Установить видимость мыши:</br>
  > `void Input_set_mouse_visible(Window *self, bool visible);`

  > Получить видимость мыши:</br>
  > `bool Input_get_mouse_visible(Window *self);`

  > Проверить, зажата ли какая-либо кнопка мыши:</br>
  > `bool Input_any_mouse_pressed(Window *self);`

  > Проверить, нажата ли какая-либо кнопка мыши:</br>
  > `bool Input_any_mouse_down(Window *self);`

  > Проверить, отжата ли какая-либо кнопка мыши:</br>
  > `bool Input_any_mouse_up(Window *self);`

  > Получить зажатые клавиши клавиатуры:</br>
  > `bool* Input_get_key_pressed(Window *self);`

  > Получить нажатие клавиши клавиатуры:</br>
  > `bool* Input_get_key_down(Window *self);`

  > Получить отжатие клавиши клавиатуры:</br>
  > `bool* Input_get_key_up(Window *self);`

  > Проверить, зажата ли какая-либо клавиша клавиатуры:</br>
  > `bool Input_any_key_pressed(Window *self);`

  > Проверить, нажата ли какая-либо клавиша клавиатуры:</br>
  > `bool Input_any_key_down(Window *self);`

  > Проверить, отжата ли какая-либо клавиша клавиатуры:</br>
  > `bool Input_any_key_up(Window *self);`


<a id="api-src-cgdf-graphics-core-light-h"></a>
- ### light.h:
  > Описание: Создаёт общий апи для работы с простым освещением.

  [Назад](#content)

  **Типы данных:**</br>
  typedef `Light2D`:
  - Простое освещение в 2D.
  - Объявление: `typedef struct Light2D Light2D;`

  **Функции:**</br>
  > Создать простое 2D освещение:</br>
  > `Light2D* Light2D_create(Renderer *renderer, Vec3f ambient, float intensity);`

  > Уничтожить простое 2D освещение:</br>
  > `void Light2D_destroy(Light2D **light);`

  > Начать захватывать отрисовку сцены:</br>
  > `void Light2D_scene_begin(Light2D *self);`

  > Закончить захватывать отрисовку сцены:</br>
  > `void Light2D_scene_end(Light2D *self);`

  > Начать захватывать отрисовку света:</br>
  > `void Light2D_light_begin(Light2D *self);`

  > Закончить захватывать отрисовку света:</br>
  > `void Light2D_light_end(Light2D *self);`

  > Отрисовать освещение (композит двух проходов отрисовки):</br>
  > `void Light2D_render(Light2D *self);`

  > Установить фоновый цвет 2D освещения:</br>
  > `void Light2D_set_ambient(Light2D *self, Vec3f ambient);`

  > Установить интенсивность 2D освещения:</br>
  > `void Light2D_set_intensity(Light2D *self, float intensity);`

  > Изменить размер 2D освещения:</br>
  > `void Light2D_resize(Light2D *self, int width, int height);`


<a id="api-src-cgdf-graphics-core-mesh-h"></a>
- ### mesh.h:
  > Описание: Определение общего функционала сетки.

  [Назад](#content)

  **Типы данных:**</br>
  typedef `Mesh`:
  - Структура сетки.
  - Объявление: `typedef struct Mesh Mesh;`

  **Функции:**</br>
  > Создать сетку:</br>
  > `Mesh* Mesh_create( const Vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, bool is_dynamic );`

  > Уничтожить сетку:</br>
  > `void Mesh_destroy(Mesh **mesh);`

  > Простой способ отрисовать сетку через forward rendering:</br>
  > `void Mesh_render(Mesh *self, bool wireframe);`


<a id="api-src-cgdf-graphics-core-renderer-h"></a>
- ### renderer.h:
  > Описание: Создаёт общий апи для работы с рендерером.

  [Назад](#content)

  **Перечисления:**</br>
  enum `RendererCameraType`:
  - Тип используемой камеры.
  - Значения:
    - `RENDERER_CAMERA_2D`
    - `RENDERER_CAMERA_3D`

  **Структуры:**</br>
  struct `RendererDebugConfig`:
  - Настройка дебага рендеринга.
  - `bool debug_enabled;` - Включить дебаг.
  - `bool sync;` - Синхронизировать поступление сообщений с вызовом API.
  - `bool level_notify;` - Уровень поступления сообщений: Уведомление.
  - `bool level_low;` - Уровень поступления сообщений: Низкий.
  - `bool level_medium;` - Уровень поступления сообщений: Средний.
  - `bool level_high;` - Уровень поступления сообщений: Высокий.

  struct `RendererInfo`:
  - Информация рендерера.
  - `char *vendor;` - Производитель видеокарты.
  - `char *renderer;` - Название видеокарты.
  - `char *version;` - Версия драйвера.
  - `char *glsl;` - Версия шейдерного языка.
  - `int max_texture_size;` - Максимальный размер текстуры.

  struct `Renderer`:
  - Рендерер.
  - `bool initialized;` - Флаг инициализации контекста OpenGL.
  - `RendererInfo info;` - Информация рендерера.
  - `Shader *shader;` - Дефолтная шейдерная программа.
  - `Shader *shader_spritebatch;` - Шейдер пакетной отрисовки спрайтов.
  - `Shader *shader_light2d;` - Шейдер 2D освещения.
  - `void *camera;` - Текущая активная камера.
  - `RendererCameraType camera_type;` - Тип камеры который используется (для корректировок).
  - `Mesh *sprite_mesh;` - Сетка спрайта.
  - `Texture *fallback_texture;` - Пустая текстура как заглушка для шейдеров.

  **Типы данных:**</br>
  typedef `Renderer`:
  - Рендерер.
  - Объявление: `typedef struct Renderer Renderer;`

  typedef `RendererInfo`:
  - Информация рендерера.
  - Объявление: `typedef struct RendererInfo RendererInfo;`

  typedef `RendererDebugConfig`:
  - Настройка дебага рендеринга.
  - Объявление: `typedef struct RendererDebugConfig RendererDebugConfig;`

  **Функции:**</br>
  > Создать рендерер:</br>
  > `Renderer* Renderer_create(void);`

  > Уничтожить рендерер:</br>
  > `void Renderer_destroy(Renderer **rnd);`

  > Инициализация рендерера:</br>
  > `void Renderer_init(Renderer *self);`

  > Освобождение буферов:</br>
  > `void Renderer_buffers_flush(Renderer *self);`

  > Освобождаем кэши:</br>
  > `void Renderer_clear_caches(Renderer *self);`

  > Получить матрицу вида камеры:</br>
  > `void Renderer_get_view(Renderer *self, mat4 view);`

  > Получить матрицу проекции камеры:</br>
  > `void Renderer_get_proj(Renderer *self, mat4 proj);`

  > Получить матрицу вида и проекции камеры:</br>
  > `void Renderer_get_view_proj(Renderer *self, mat4 view, mat4 proj);`

  > Получить ширину камеры:</br>
  > `int Renderer_get_width(Renderer *self);`

  > Получить высоту камеры:</br>
  > `int Renderer_get_height(Renderer *self);`

  > Получить производителя видеокарты:</br>
  > `const char* Renderer_get_vendor(Renderer *self);`

  > Получить название видеокарты:</br>
  > `const char* Renderer_get_renderer(Renderer *self);`

  > Получить версию драйвера:</br>
  > `const char* Renderer_get_version(Renderer *self);`

  > Получить версию шейдерного языка:</br>
  > `const char* Renderer_get_glsl(Renderer *self);`

  > Получить максимальный размер текстуры:</br>
  > `int Renderer_get_max_texture_size(Renderer *self);`

  > Получить сколько всего видеопамяти есть (в килобайтах):</br>
  > `int Renderer_get_total_memory(Renderer *self);`

  > Сколько используется видеопамяти (в килобайтах):</br>
  > `int Renderer_get_used_memory(Renderer *self);`

  > Сколько свободно видеопамяти (в килобайтах):</br>
  > `int Renderer_get_free_memory(Renderer *self);`

  **Глобальные переменные:**</br>
  `Renderer_debug_config`:
  - Глобальная конфигурация дебага рендеринга.
  - Объявление: `extern RendererDebugConfig Renderer_debug_config;`.


<a id="api-src-cgdf-graphics-core-scene-h"></a>
- ### scene.h:
  > Описание: Определение функционала для работы со сценами.

  [Назад](#content)

  **Структуры:**</br>
  struct `WindowScene`:
  - Сцена окна.
  - `void (*start)   (Window *self);` - Вызывается после создания окна.
  - `void (*destroy) (Window *self);` - Вызывается при закрытии окна.
  - `void (*update)  (Window *self, float dtime);` - Вызывается каждый кадр (цикл окна).
  - `void (*render)  (Window *self, float dtime);` - Вызывается каждый кадр (отрисовка окна).
  - `void (*resize)  (Window *self, int width, int height);` - Вызывается при изменении размера окна.
  - `void (*show)    (Window *self);` - Вызывается при разворачивании окна.
  - `void (*hide)    (Window *self);` - Вызывается при сворачивании окна.

  **Типы данных:**</br>
  typedef `Window`:
  - Объявление: `typedef struct Window Window;`

  typedef `WindowScene`:
  - Сцена окна.
  - Объявление: `typedef struct WindowScene WindowScene;`


<a id="api-src-cgdf-graphics-core-shader-h"></a>
- ### shader.h:
  > Описание: Определяет общий функционал для работы с шейдерами.

  [Назад](#content)

  **Перечисления:**</br>
  enum `ShaderCacheUniformType`:
  - Виды значений юниформов для кэша.
  - Значения:
    - `SHADERCACHE_UNIFORM_BOOL`
    - `SHADERCACHE_UNIFORM_INT`
    - `SHADERCACHE_UNIFORM_FLOAT`
    - `SHADERCACHE_UNIFORM_VEC2`
    - `SHADERCACHE_UNIFORM_VEC3`
    - `SHADERCACHE_UNIFORM_VEC4`

  **Структуры:**</br>
  struct `ShaderCacheUniformLocation`:
  - Единица кэша локаций юниформов.
  - `char    *name;` - Имя юниформа.
  - `int32_t location;` - Позиция в шейдере.

  struct `ShaderCacheUniformValue`:
  - Единица кэша значений юниформов.
  - `ShaderCacheUniformType type;` - Тип значения.
  - `int32_t location;` - Позиция в шейдере.

  struct `ShaderCacheSampler`:
  - Единица кэша сэмплеров.
  - `int32_t  location;`.
  - `uint32_t tex_id;`.
  - `uint32_t unit_id;`.

  struct `Shader`:
  - Структура шейдера.
  - `const char* vertex;`.
  - `const char* fragment;`.
  - `const char* geometry;`.
  - `char* error;`.
  - `uint32_t id;`.
  - `Renderer *renderer;`.
  - `bool _is_begin_;`.
  - `int32_t _id_before_begin_;`.
  - `Array *uniform_locations;` - Кэш позиций uniform.
  - `Array *uniform_values;` - Кэш значений uniform (всё кроме массивов и матриц).
  - `Array *sampler_units;` - Кэш привязки текстурных юнитов к названиям униформов.

  **Типы данных:**</br>
  typedef `Shader`:
  - Шейдерная программа.
  - Объявление: `typedef struct Shader Shader;`

  typedef `ShaderCacheUniformLocation`:
  - Кэш локаций юниформов.
  - Объявление: `typedef struct ShaderCacheUniformLocation ShaderCacheUniformLocation;`

  typedef `ShaderCacheUniformValue`:
  - Кэш значений юниформов.
  - Объявление: `typedef struct ShaderCacheUniformValue ShaderCacheUniformValue;`

  typedef `ShaderCacheSampler`:
  - Объявление: `typedef struct ShaderCacheSampler ShaderCacheSampler;`

  typedef `Renderer`:
  - Повторное локальное определение.
  - Объявление: `typedef struct Renderer Renderer;`

  **Функции:**</br>
  > Создать шейдерную программу:</br>
  > `Shader* Shader_create(Renderer *renderer, const char *vert, const char *frag, const char *geom);`

  > Уничтожить шейдерную программу:</br>
  > `void Shader_destroy(Shader **shader);`

  > Компиляция шейдеров в программу:</br>
  > `void Shader_compile(Shader *self);`

  > Получить ошибку компиляции или линковки:</br>
  > `const char* Shader_get_error(Shader *self);`

  > Активация программы:</br>
  > `void Shader_begin(Shader *self);`

  > Деактивация программы:</br>
  > `void Shader_end(Shader *self);`

  > Получить локацию переменной:</br>
  > `int32_t Shader_get_location(Shader *self, const char* name);`

  > Установить значение bool:</br>
  > `void Shader_set_bool(Shader *self, const char* name, bool value);`

  > Установить значение int:</br>
  > `void Shader_set_int(Shader *self, const char* name, int value);`

  > Установить значение float:</br>
  > `void Shader_set_float(Shader *self, const char* name, float value);`

  > Установить значение vec2:</br>
  > `void Shader_set_vec2(Shader *self, const char* name, Vec2f value);`

  > Установить значение vec3:</br>
  > `void Shader_set_vec3(Shader *self, const char* name, Vec3f value);`

  > Установить значение vec4:</br>
  > `void Shader_set_vec4(Shader *self, const char* name, Vec4f value);`

  > Установить значение mat2:</br>
  > `void Shader_set_mat2(Shader *self, const char* name, mat2 value);`

  > Установить значение mat3:</br>
  > `void Shader_set_mat3(Shader *self, const char* name, mat3 value);`

  > Установить значение mat4:</br>
  > `void Shader_set_mat4(Shader *self, const char* name, mat4 value);`

  > Установить значение mat2x3:</br>
  > `void Shader_set_mat2x3(Shader *self, const char* name, mat2x3 value);`

  > Установить значение mat3x2:</br>
  > `void Shader_set_mat3x2(Shader *self, const char* name, mat3x2 value);`

  > Установить значение mat2x4:</br>
  > `void Shader_set_mat2x4(Shader *self, const char* name, mat2x4 value);`

  > Установить значение mat4x2:</br>
  > `void Shader_set_mat4x2(Shader *self, const char* name, mat4x2 value);`

  > Установить значение mat3x4:</br>
  > `void Shader_set_mat3x4(Shader *self, const char* name, mat3x4 value);`

  > Установить значение mat4x3:</br>
  > `void Shader_set_mat4x3(Shader *self, const char* name, mat4x3 value);`

  > Установить 2D текстуру:</br>
  > `void Shader_set_tex2d(Shader *self, const char* name, uint32_t tex_id);`

  > Установить 3D текстуру:</br>
  > `void Shader_set_tex3d(Shader *self, const char* name, uint32_t tex_id);`

  > Очистить кэши шейдера:</br>
  > `void Shader_clear_caches(Shader *self);`


<a id="api-src-cgdf-graphics-core-sprite-h"></a>
- ### sprite.h:
  > Описание: Общий функционал для отображения спрайтов.

  [Назад](#content)

  **Структуры:**</br>
  struct `Sprite2D`:
  - Двумерный спрайт.
  - `Renderer *renderer;` - Рендерер (для доступа к графическому апи и шейдеру).
  - `Texture *texture;` - Текстура спрайта.
  - `float x, y;` - Координаты.
  - `float width, height;` - Размеры.
  - `float angle;` - Угол вращения (против часовой стрелки).
  - `Vec4f color;` - Вектор цвета (значения от 0.0 до 1.0).
  - `bool custom_shader;` - Использовать пользовательский шейдер (true) или встроенный (false).
  - `void (*render) (Sprite2D *self);` - Отрисовать спрайт, на основе данных в структуре.

  struct `Sprite3D`:
  - Трёхмерный спрайт.
  - `Renderer *renderer;` - Рендерер (для доступа к графическому апи и шейдеру).
  - `Texture *texture;` - Текстура спрайта.
  - `Vec3f position;` - Координаты.
  - `Vec3f rotation;` - Углы вращения.
  - `float width, height;` - Размеры.
  - `Vec4f color;` - Вектор цвета (значения от 0.0 до 1.0).
  - `bool custom_shader;` - Использовать пользовательский шейдер (true) или встроенный (false).
  - `void (*render) (Sprite3D *self);` - Отрисовать спрайт, на основе данных в структуре.

  **Типы данных:**</br>
  typedef `Sprite2D`:
  - Двумерный спрайт.
  - Объявление: `typedef struct Sprite2D Sprite2D;`

  typedef `Sprite3D`:
  - Трёхмерный спрайт.
  - Объявление: `typedef struct Sprite3D Sprite3D;`

  **Функции:**</br>
  > Создать спрайт:</br>
  > `Sprite2D* Sprite2D_create( Renderer *renderer, Texture *texture, float x, float y, float width, float height, float angle, Vec4f color, bool custom_shader );`

  > Уничтожить спрайт:</br>
  > `void Sprite2D_destroy(Sprite2D **sprite);`

  > Отрисовать 2D спрайт (без создания экземпляра):</br>
  > `void Sprite2D_render( Renderer *renderer, Texture *texture, float x, float y, float width, float height, float angle, Vec4f color, bool custom_shader );`

  > Создать спрайт:</br>
  > `Sprite3D* Sprite3D_create( Renderer *renderer, Texture *texture, Vec3f position, Vec3f rotation, float width, float height, Vec4f color, bool custom_shader );`

  > Уничтожить спрайт:</br>
  > `void Sprite3D_destroy(Sprite3D **sprite);`

  > Отрисовать 3D спрайт (без создания экземпляра):</br>
  > `void Sprite3D_render( Renderer *renderer, Texture *texture, Vec3f position, Vec3f rotation, float width, float height, Vec4f color, bool custom_shader );`


<a id="api-src-cgdf-graphics-core-spritebatch-h"></a>
- ### spritebatch.h:
  > Описание: Определяет API для пакетной отрисовки спрайтов.

  [Назад](#content)

  **Определения:**</br>
  `BATCH_MAX_SPRITES`:
  - Объём буфера пакета в спрайтах (константа).
  - Значение: `2048`.

  `BATCH_VERTS_PER_SPRITE`:
  - Квадрат из 4 вершин.
  - Значение: `4`.

  `BATCH_INDCS_PER_SPRITE`:
  - 6 индексов для создания спрайта.
  - Значение: `6`.

  **Структуры:**</br>
  struct `SpriteVertex`:
  - Вершина спрайта.
  - `float x, y, z;` - Позиция вершины       (loc=0, vec3 a_position).
  - `float u, v;` - Текстурные координаты (loc=1, vec2 a_texcoord).

  **Типы данных:**</br>
  typedef `SpriteBatch`:
  - Пакетная отрисовка спрайтов.
  - Объявление: `typedef struct SpriteBatch SpriteBatch;`

  typedef `SpriteVertex`:
  - Вершина спрайта.
  - Объявление: `typedef struct SpriteVertex SpriteVertex;`

  **Функции:**</br>
  > Создать пакетную отрисовку спрайтов:</br>
  > `SpriteBatch* SpriteBatch_create(Renderer *renderer);`

  > Уничтожить пакетную отрисовку спрайтов:</br>
  > `void SpriteBatch_destroy(SpriteBatch **batch);`

  > Начать отрисовку:</br>
  > `void SpriteBatch_begin(SpriteBatch *self);`

  > Установить цвет следующим спрайтам:</br>
  > `void SpriteBatch_set_color(SpriteBatch *self, Vec4f color);`

  > Получить установленный цвет:</br>
  > `Vec4f SpriteBatch_get_color(SpriteBatch *self);`

  > Установить текстурные координаты следующим спрайтам:</br>
  > `void SpriteBatch_set_texcoord(SpriteBatch *self, Vec4f texcoord);`

  > Сбросить текстурные координаты:</br>
  > `void SpriteBatch_reset_texcoord(SpriteBatch *self);`

  > Получить текстурные координаты:</br>
  > `Vec4f SpriteBatch_get_texcoord(SpriteBatch *self);`

  > Добавить 2D спрайт в пакет данных:</br>
  > `void SpriteBatch_draw( SpriteBatch *self, Texture *texture, float x, float y, float width, float height, float angle );`

  > Добавить 3D спрайт в пакет данных:</br>
  > `void SpriteBatch_draw3d( SpriteBatch *self, Texture *texture, Vec3f position, Vec3f rotation, float width, float height );`

  > Закончить отрисовку:</br>
  > `void SpriteBatch_end(SpriteBatch *self);`

  **Глобальные переменные:**</br>
  `BATCH_SPRITES_SIZE`:
  - BATCH_MAX_SPRITES Малое количество спрайтов на пакет, уменьшит производительность из-за частых отрисовок Среднее количество спрайтов на пакет, увеличит производительность, но главное не переборщить (от 1024 до 2048/4096) Большое количество спрайтов на пакет, уменьшит производительность из-за больших объёмов данных.
  - Объявление: `extern uint32_t BATCH_SPRITES_SIZE;`.


<a id="api-src-cgdf-graphics-core-texture-h"></a>
- ### texture.h:
  > Описание: Определяет функционал текстуры.

  [Назад](#content)

  **Перечисления:**</br>
  enum `TextureFormat`:
  - Типы формата данных текстуры и исходников.

  enum `TextureDataType`:
  - Типы данных используемой в текстуре.
  - Значения:
    - `TEX_DATA_FLOAT`

  enum `TextureType`:
  - Типы текстур.
  - Значения:
    - `TEX_TYPE_2D`
    - `TEX_TYPE_3D`

  **Структуры:**</br>
  struct `Texture`:
  - Структура текстуры.
  - `Renderer *renderer;` - Указатель на рендерер.
  - `uint32_t id;` - Айди текстуры.
  - `int width;` - Ширина текстуры.
  - `int height;` - Высота текстуры.
  - `int channels;` - Количество каналов текстуры.
  - `bool has_mipmap;` - Наличие мипмапов.
  - `bool _is_begin_;` - Признак активности текстуры (внутренняя логика).
  - `int32_t _id_before_begin_;` - Прошлые айди состояния (внутренняя логика).
  - `int32_t _active_id_before_begin_;` - Прошлые айди состояния (внутренняя логика).

  **Типы данных:**</br>
  typedef `Renderer`:
  - Объявление: `typedef struct Renderer Renderer;`

  typedef `Texture`:
  - Текстура 2D.
  - Объявление: `typedef struct Texture Texture;`

  **Функции:**</br>
  > Создать текстуру:</br>
  > `Texture* Texture_create(Renderer *renderer);`

  > Уничтожить текстуру:</br>
  > `void Texture_destroy(Texture **texture);`

  > Сделать пустую текстуру нужного размера:</br>
  > `void Texture_empty(Texture *self, int width, int height, bool use_mipmap, TextureFormat format, TextureDataType dtype);`

  > Загрузить текстуру (из файла):</br>
  > `void Texture_load(Texture *self, const char *filepath, bool use_mipmap);`

  > Активация текстуры:</br>
  > `void Texture_begin(Texture *self);`

  > Деактивация текстуры:</br>
  > `void Texture_end(Texture *self);`

  > Загрузить текстуру (из картинки):</br>
  > `void Texture_load_pixmap(Texture *self, Pixmap *pixmap, bool use_mipmap);`

  > Установить данные текстуры:</br>
  > `void Texture_set_data( Texture *self, const int width, const int height, const void *data, bool use_mipmap, TextureFormat tex_format, TextureFormat data_format, TextureDataType data_type );`

  > Установить данные текстуры (подмассив):</br>
  > `void Texture_set_subdata( Texture *self, int miplevel, int offset_x, int offset_y, int width, int height, TextureFormat data_format, TextureDataType data_type, const void *data );`

  > Получить картинку из текстуры:</br>
  > `Pixmap* Texture_get_pixmap(Texture *self, int channels);`

  > Установить фильтрацию текстуры:</br>
  > `void Texture_set_filter(Texture *self, int name, int param);`

  > Установить повторение текстуры:</br>
  > `void Texture_set_wrap(Texture *self, int axis, int param);`

  > Установить линейную фильтрацию текстуры:</br>
  > `void Texture_set_linear(Texture *self);`

  > Установить пикселизацию текстуры:</br>
  > `void Texture_set_pixelized(Texture *self);`


<a id="api-src-cgdf-graphics-core-utils-h"></a>
- ### utils.h:
  > Описание: Вспомогательные функции в графике.

  [Назад](#content)

  **Функции:**</br>
  > Переводит координаты точки на экране, в мировые координаты в 2D пространстве:</br>
  > `static inline Vec2d local_to_global_2d(Camera2D *camera, Vec2d point);`

  > Переводит координаты точки на экране, в мировые координаты на плоскости:</br>
  > `static inline bool camera_screen_to_plane( Window *window, mat4 view, mat4 proj, Vec2i mouse_pos, Vec3f plane_point, Vec3f plane_normal, Vec3f *out_pos );`


<a id="api-src-cgdf-graphics-core-vertex-h"></a>
- ### vertex.h:
  > Описание: Просто объявление структуры вершины.

  [Назад](#content)

  **Структуры:**</br>
  struct `Vertex`:
  - Структура вершины.
  - `float px, py, pz;` - Позиция вершины       (loc=0, vec3 a_position).
  - `float nx, ny, nz;` - Нормаль вершины       (loc=1, vec3 a_normal).
  - `float r, g, b, a;` - Цвет вершины          (loc=2, vec3 a_color).
  - `float u, v;` - Текстурные координаты (loc=3, vec2 a_texcoord).

  **Типы данных:**</br>
  typedef `Vertex`:
  - Структура вершины.
  - Объявление: `typedef struct Vertex Vertex;`


<a id="api-src-cgdf-graphics-core-window-h"></a>
- ### window.h:
  > Описание: Общий апи для работы с окном приложения.

  [Назад](#content)

  **Структуры:**</br>
  struct `Window`:
  - Структура окна.
  - `WindowScene scene;` - Текущая сцена окна.
  - `WinConfig   *config;` - Конфигурация окна.
  - `WinVars     *vars;` - Указатель на переменные окна.
  - `Input       *input;` - Ввод.
  - `Renderer    *renderer;` - Рендерер.

  struct `WinConfig`:
  - Конфигурация окна.
  - `const char *title;` - Заголовок окна.
  - `Pixmap *icon;` - Иконка окна.
  - `bool vsync;` - Вертикальная синхронизация.
  - `int  fps;` - Количество кадров в секунду.
  - `bool visible;` - Видимость окна (скрыт/виден).
  - `bool titlebar;` - Видимость заголовка окна.
  - `bool resizable;` - Масштабируемость окна.
  - `bool fullscreen;` - Полноэкранный режим.
  - `bool always_top;` - Всегда на переднем плане.
  - `WindowScene scene;` - Сцена окна.
  - `int gl_major;` - Старшая версия OpenGL.
  - `int gl_minor;` - Младшая версия OpenGL.

  **Типы данных:**</br>
  typedef `Window`:
  - Структура окна.
  - Объявление: `typedef struct Window Window;`

  typedef `WinConfig`:
  - Конфигурация окна.
  - Объявление: `typedef struct WinConfig WinConfig;`

  typedef `WinVars`:
  - Внутренние переменные окна (определяется в реализации).
  - Объявление: `typedef struct WinVars WinVars;`

  **Функции:**</br>
  > Создать конфигурацию окна:</br>
  > `WinConfig* Window_create_config(WindowScene scene);`

  > Уничтожить конфигурацию окна:</br>
  > `void Window_destroy_config(WinConfig **config);`

  > Создать окно:</br>
  > `Window* Window_create(WinConfig *config);`

  > Уничтожить окно:</br>
  > `void Window_destroy(Window **window);`

  > Вызовите для открытия окна:</br>
  > `bool Window_open(Window *self);`

  > Вызовите для закрытия окна:</br>
  > `bool Window_close(Window *self);`

  > Вызовите для полного завершения работы всех окон:</br>
  > `bool Window_quit(Window *self);`

  > Установить заголовок окна:</br>
  > `void Window_set_title(Window *self, const char *title, ...);`

  > Получить заголовок окна:</br>
  > `const char* Window_get_title(Window *self);`

  > Установить иконку окна:</br>
  > `void Window_set_icon(Window *self, Pixmap *icon);`

  > Получить иконку окна:</br>
  > `Pixmap* Window_get_icon(Window *self);`

  > Установить размер окна:</br>
  > `void Window_set_size(Window *self, int width, int height);`

  > Получить размер окна:</br>
  > `void Window_get_size(Window *self, int *width, int *height);`

  > Установить ширину окна:</br>
  > `void Window_set_width(Window *self, int width);`

  > Получить ширину окна:</br>
  > `int Window_get_width(Window *self);`

  > Установить высоту окна:</br>
  > `void Window_set_height(Window *self, int height);`

  > Получить высоту окна:</br>
  > `int Window_get_height(Window *self);`

  > Получить центр окна:</br>
  > `void Window_get_center(Window *self, int *x, int *y);`

  > Установить позицию окна:</br>
  > `void Window_set_position(Window *self, int x, int y);`

  > Получить позицию окна:</br>
  > `void Window_get_position(Window *self, int *x, int *y);`

  > Установить вертикальную синхронизацию:</br>
  > `void Window_set_vsync(Window *self, bool vsync);`

  > Получить вертикальную синхронизацию:</br>
  > `bool Window_get_vsync(Window *self);`

  > Установить фпс окна:</br>
  > `void Window_set_fps(Window *self, int fps);`

  > Получить установленный фпс окна:</br>
  > `int Window_get_target_fps(Window *self);`

  > Установить видимость окна:</br>
  > `void Window_set_visible(Window *self, bool visible);`

  > Получить видимость окна:</br>
  > `bool Window_get_visible(Window *self);`

  > Установить видимость заголовка окна:</br>
  > `void Window_set_titlebar(Window *self, bool titlebar);`

  > Получить видимость заголовка окна:</br>
  > `bool Window_get_titlebar(Window *self);`

  > Установить масштабируемость окна:</br>
  > `void Window_set_resizable(Window *self, bool resizable);`

  > Получить масштабируемость окна:</br>
  > `bool Window_get_resizable(Window *self);`

  > Установить полноэкранный режим:</br>
  > `void Window_set_fullscreen(Window *self, bool fullscreen);`

  > Получить полноэкранный режим:</br>
  > `bool Window_get_fullscreen(Window *self);`

  > Установить минимальный размер окна:</br>
  > `void Window_set_min_size(Window *self, int width, int height);`

  > Получить минимальный размер окна:</br>
  > `void Window_get_min_size(Window *self, int *width, int *height);`

  > Установить максимальный размер окна:</br>
  > `void Window_set_max_size(Window *self, int width, int height);`

  > Получить максимальный размер окна:</br>
  > `void Window_get_max_size(Window *self, int *width, int *height);`

  > Установить всегда на переднем плане или нет:</br>
  > `void Window_set_always_top(Window *self, bool on_top);`

  > Получить всегда на переднем плане или нет:</br>
  > `bool Window_get_always_top(Window *self);`

  > Получить фокус окна:</br>
  > `bool Window_get_is_focused(Window *self);`

  > Получить расфокус окна:</br>
  > `bool Window_get_is_defocused(Window *self);`

  > Получить айди дисплея в котором это окно:</br>
  > `uint32_t Window_get_window_display_id(Window *self);`

  > Получить размер дисплея:</br>
  > `bool Window_get_display_size(Window *self, uint32_t id, int *width, int *height);`

  > Развернуть окно на весь экран:</br>
  > `void Window_maximize(Window *self);`

  > Свернуть окно в панель задач:</br>
  > `void Window_minimize(Window *self);`

  > Восстановить обычное состояние окна:</br>
  > `void Window_restore(Window *self);`

  > Перенести окно на передний план:</br>
  > `void Window_raise(Window *self);`

  > Получить текущий фпс:</br>
  > `float Window_get_current_fps(Window *self);`

  > Получить дельту времени:</br>
  > `double Window_get_dtime(Window *self);`

  > Получить время со старта окна:</br>
  > `double Window_get_time(Window *self);`

  > Установить сцену окна:</br>
  > `void Window_set_scene(Window *self, WindowScene scene);`

  > Получить сцену окна:</br>
  > `WindowScene Window_get_scene(Window *self);`

  > Очистить окно:</br>
  > `void Window_clear(Window *self, float r, float g, float b);`

  > Отрисовка содержимого окна:</br>
  > `void Window_display(Window *self);`

#

### src/cgdf/graphics/core/controllers/

<a id="api-src-cgdf-graphics-core-controllers-controllers-h"></a>
- ### controllers.h:
  > Описание: Простые контроллеры камеры.

  [Назад](#content)

  **Структуры:**</br>
  struct `CameraController2D`:
  - Простой контроллер для 2D камеры.
  - `Window *window;` - Указатель на окно.
  - `Camera2D *camera;` - Указатель на 2D камеру.
  - `Vec2i fixed_mouse_pos;` - Прошлая позиция мыши.
  - `Vec2d target_pos;` - Целевая позиция камеры.
  - `float target_zoom;` - Целевой масштаб камеры.
  - `float offset_scale;` - Коэффициент смещения мыши для скольжения через колесико мыши.
  - `float min_zoom;` - Минимальный масштаб камеры.
  - `float max_zoom;` - Максимальный масштаб камеры.
  - `float friction;` - Коэффициент скольжения камеры.
  - `bool is_movement;` - Перемещается ли камера или нет.

  struct `CameraController3D`:
  - Простой контроллер для 3D камеры.
  - `Window *window;` - Указатель на окно.
  - `Camera3D *camera;` - Указатель на 3D камеру.
  - `float mouse_sensitivity;` - Коэффициент чувствительности мыши.
  - `float ctrl_speed;` - Скорость камеры при зажатом CTRL (ед/сек).
  - `float speed;` - Скорость камеры без клавиш модификаторов (ед/сек).
  - `float shift_speed;` - Скорость камеры при зажатом SHIFT (ед/сек).
  - `float friction;` - Коэффициент скольжения камеры.
  - `bool up_is_forward;` - Вверх - вперед.
  - `Vec3f up_dir;` - Направление вверх.
  - `Vec3d target_pos;` - Целевая позиция камеры.
  - `float target_fov;` - Целевой угол обзора.
  - `bool pressed_pass;` - Пропуск нажатия (внутренняя логика).
  - `bool is_pressed;` - Нажата клавиша (внутренняя логика).
  - `bool is_movement;` - Перемещается ли камера или нет.

  struct `CameraOrbitController3D`:
  - Орбитальный контроллер для 3D камеры.
  - `Window *window;` - Указатель на окно.
  - `Camera3D *camera;` - Указатель на 3D камеру.
  - `float mouse_sensitivity;` - Коэффициент чувствительности мыши.
  - `float distance;` - Дистанция камеры от цели.
  - `float friction;` - Коэффициент скольжения камеры.
  - `bool up_is_forward;` - Вверх - вперед.
  - `Vec3d rotation;` - Поворот камеры.
  - `Vec3d target_pos;` - Целевая позиция камеры.
  - `Vec3d target_rot;` - Целевой поворот камеры.
  - `float target_dst;` - Целевая дистанция камеры.
  - `float target_fov;` - Целевой угол обзора.
  - `bool pressed_pass;` - Пропуск нажатия (внутренняя логика).
  - `bool is_pressed;` - Нажата клавиша (внутренняя логика).
  - `bool is_movement;` - Перемещается ли камера или нет.

  **Типы данных:**</br>
  typedef `CameraController2D`:
  - Простой контроллер для 2D камеры.
  - Объявление: `typedef struct CameraController2D CameraController2D;`

  typedef `CameraController3D`:
  - Простой контроллер для 3D камеры.
  - Объявление: `typedef struct CameraController3D CameraController3D;`

  typedef `CameraOrbitController3D`:
  - Орбитальный контроллер для 3D камеры.
  - Объявление: `typedef struct CameraOrbitController3D CameraOrbitController3D;`

  **Функции:**</br>
  > Создать 2D контроллер:</br>
  > `CameraController2D* CameraController2D_create( Window *window, Camera2D *camera, float offset_scale, float min_zoom, float max_zoom, float friction );`

  > Уничтожить 2D контроллер:</br>
  > `void CameraController2D_destroy(CameraController2D **ctrl);`

  > Обновление контроллера:</br>
  > `void CameraController2D_update(CameraController2D *self, float dtime, bool pressed_pass);`

  > Создать 3D контроллер:</br>
  > `CameraController3D* CameraController3D_create( Window *window, Camera3D *camera, float mouse_sensitivity, float ctrl_speed, float speed, float shift_speed, float friction, bool up_is_forward );`

  > Уничтожить 3D контроллер:</br>
  > `void CameraController3D_destroy(CameraController3D **ctrl);`

  > Обновление контроллера:</br>
  > `void CameraController3D_update(CameraController3D *self, float dtime, bool pressed_pass);`

  > Создать орбитальный 3D контроллер:</br>
  > `CameraOrbitController3D* CameraOrbitController3D_create( Window *window, Camera3D *camera, Vec3d target_pos, float mouse_sensitivity, float distance, float friction, bool up_is_forward );`

  > Уничтожить орбитальный 3D контроллер:</br>
  > `void CameraOrbitController3D_destroy(CameraOrbitController3D **ctrl);`

  > Обновление контроллера:</br>
  > `void CameraOrbitController3D_update(CameraOrbitController3D *self, float dtime, bool pressed_pass);`

#

### src/cgdf/graphics/opengl/

<a id="api-src-cgdf-graphics-opengl-buffer-gc-h"></a>
- ### buffer_gc.h:
  [Назад](#content)

  **Определения:**</br>
  `BUFFER_GC_GL_START_CAPACITY`:
  - Значение: `1024`.

  **Перечисления:**</br>
  enum `BufferGC_GL_Type`:
  - Типы буферов на уничтожение.
  - Значения:
    - `BGC_GL_QBO`
    - `BGC_GL_SSBO`
    - `BGC_GL_FBO`
    - `BGC_GL_RBO`
    - `BGC_GL_VBO`
    - `BGC_GL_EBO`
    - `BGC_GL_VAO`
    - `BGC_GL_TBO`

  **Структуры:**</br>
  struct `BufferGC_GL`:
  - Единица кэша локаций юниформов.
  - `Array *qbo;`.
  - `Array *ssbo;`.
  - `Array *fbo;`.
  - `Array *rbo;`.
  - `Array *vbo;`.
  - `Array *ebo;`.
  - `Array *vao;`.
  - `Array *tbo;`.

  **Типы данных:**</br>
  typedef `BufferGC_GL`:
  - Объявление: `typedef struct BufferGC_GL BufferGC_GL;`

  **Функции:**</br>
  > Инициализация стеков буферов:</br>
  > `void BufferGC_GL_init();`

  > Уничтожение стеков буферов:</br>
  > `void BufferGC_GL_destroy();`

  > Добавить буфер на уничтожение:</br>
  > `void BufferGC_GL_push(BufferGC_GL_Type type, unsigned int id);`

  > Очистка всех буферов:</br>
  > `void BufferGC_GL_flush();`

  **Глобальные переменные:**</br>
  `buffer_gc_gl`:
  - Создаём единую глобальную структуру.
  - Объявление: `extern BufferGC_GL buffer_gc_gl;`.


<a id="api-src-cgdf-graphics-opengl-gl-h"></a>
- ### gl.h:
  > Описание: Просто подключаем функционал OpenGL.

  [Назад](#content)

  **Функции:**</br>
  > Инициализация OpenGL (true = ошибка, false = успех):</br>
  > `static inline bool gl_init();`


<a id="api-src-cgdf-graphics-opengl-texunit-h"></a>
- ### texunit.h:
  [Назад](#content)

  **Структуры:**</br>
  struct `TextureUnits`:
  - Текстурные юниты.
  - `Renderer *renderer;` - Указатель на рендерер.
  - `Array *stack;` - Стек юнитов и привязок.
  - `size_t total;` - Всего юнитов.
  - `size_t used;` - Использовано юнитов.

  struct `TexUnit`:
  - Один текстурный юнит.
  - `uint32_t shd_id;` - Айди шейдера.
  - `uint32_t loc_id;` - Локация юниформа.
  - `uint32_t tex_id;` - Айди текстуры.
  - `uint32_t type;` - Тип текстуры.
  - `bool     used;` - Флаг использования.

  **Типы данных:**</br>
  typedef `TextureUnits`:
  - Объявление: `typedef struct TextureUnits TextureUnits;`

  typedef `TexUnit`:
  - Объявление: `typedef struct TexUnit TexUnit;`

  typedef `Renderer`:
  - Объявление: `typedef struct Renderer Renderer;`

  **Функции:**</br>
  > Инициализировать текстурные юниты (вызывается автоматически):</br>
  > `void TextureUnits_init(Renderer *renderer);`

  > Уничтожить текстурные юниты (вызывается автоматически):</br>
  > `void TextureUnits_destroy();`

  > Получить всего возможных юнитов:</br>
  > `size_t TexUnits_get_total_units();`

  > Получить количество занятых юнитов:</br>
  > `size_t TexUnits_get_used_units();`

  > Получить количество свободных юнитов:</br>
  > `size_t TexUnits_get_free_units();`

  > Отвязать все текстуры:</br>
  > `void TexUnits_unbind_all();`

  > Деактивировать определённую текстуру во всех юнитах:</br>
  > `void TexUnits_invalidate_texture(uint32_t tex_id);`

  > Получить номер юнита по (шейдер, локация):</br>
  > `int TexUnits_find(uint32_t shd_id, int32_t loc_id);`

  > Зарезервировать юнит для шейдера:</br>
  > `int TexUnits_reserve(uint32_t shd_id, int32_t loc_id);`

  > Перепривязать текстуру к юниту, которая уже зарезервирована для шейдера:</br>
  > `int TexUnits_rebind_owned(uint32_t shd_id, int32_t loc_id, uint32_t tex_id, TextureType type);`

  > Освободить все юниты, которые зарезервированы для шейдера (используйте только при удалении шейдера!):</br>
  > `void TexUnits_release_shader(uint32_t shd_id);`

  **Глобальные переменные:**</br>
  `texunits_gl`:
  - Создаём единую глобальную структуру.
  - Объявление: `extern TextureUnits texunits_gl;`.

#

### src/cgdf/graphics/opengl/buffers/

<a id="api-src-cgdf-graphics-opengl-buffers-buffers-h"></a>
- ### buffers.h:
  > Описание: Структуры и API буферов.

  [Назад](#content)

  **Перечисления:**</br>
  enum `BufferFBO_Type`:
  - Типы привязок для кадрового буфера.
  - Значения:
    - `BUFFER_FBO_COLOR`
    - `BUFFER_FBO_DEPTH`
    - `BUFFER_FBO_DEPTH_STENCIL`

  **Структуры:**</br>
  struct `BufferQBO`:
  - Структура буфера отслеживания.
  - `uint32_t id;` - Айди буфера.
  - `bool _is_begin_;` - Флаг использования буфера (внутренняя логика).

  struct `BufferFBO`:
  - Структура буфера кадра.
  - `int width;` - Ширина кадрового буфера.
  - `int height;` - Высота кадрового буфера.
  - `uint32_t id;` - Айди кадрового буфера.
  - `uint32_t rbo_id;` - Айди рендер буфера.
  - `bool _is_begin_;` - Флаг использования буфера (внутренняя логика).
  - `int32_t _id_before_begin_;` - ID буфера перед использованием (внутренняя логика).
  - `int32_t _rbo_id_before_begin_;` - ID рендер буфера перед использованием (внутренняя логика).
  - `int32_t _id_before_read_;` - Айди перед чтением (внутренняя логика).
  - `int32_t _id_before_draw_;` - Айди перед рисованием (внутренняя логика).
  - `Array *attachments;` - Массив привязок.

  struct `BufferVBO`:
  - Структура буфера вершин.
  - `uint32_t id;` - Айди буфера.
  - `bool _is_begin_;` - Флаг использования буфера (внутренняя логика).
  - `int32_t _id_before_begin_;` - ID буфера перед использованием (внутренняя логика).

  struct `BufferEBO`:
  - Структура буфера индексов.
  - `uint32_t id;` - Айди буфера.
  - `bool _is_begin_;` - Флаг использования буфера (внутренняя логика).
  - `int32_t _id_before_begin_;` - ID буфера перед использованием (внутренняя логика).

  struct `BufferVAO`:
  - Структура буфера атрибутов.
  - `uint32_t id;` - Айди буфера.
  - `bool _is_begin_;` - Флаг использования буфера (внутренняя логика).
  - `int32_t _id_before_begin_;` - ID буфера перед использованием (внутренняя логика).

  **Типы данных:**</br>
  typedef `BufferQBO`:
  - Структура буфера отслеживания.
  - Объявление: `typedef struct BufferQBO BufferQBO;`

  typedef `BufferFBO`:
  - Структура буфера кадра.
  - Объявление: `typedef struct BufferFBO BufferFBO;`

  typedef `BufferVBO`:
  - Структура буфера вершин.
  - Объявление: `typedef struct BufferVBO BufferVBO;`

  typedef `BufferEBO`:
  - Структура буфера индексов.
  - Объявление: `typedef struct BufferEBO BufferEBO;`

  typedef `BufferVAO`:
  - Структура буфера атрибутов.
  - Объявление: `typedef struct BufferVAO BufferVAO;`

  **Функции:**</br>
  > Создать буфер отслеживания:</br>
  > `BufferQBO* BufferQBO_create();`

  > Уничтожить буфер отслеживания:</br>
  > `void BufferQBO_destroy(BufferQBO **qbo);`

  > Использовать буфер:</br>
  > `void BufferQBO_begin(BufferQBO *self);`

  > Не использовать буфер:</br>
  > `void BufferQBO_end(BufferQBO *self);`

  > Получить количество отрисованных примитивов:</br>
  > `uint32_t BufferQBO_get_primitives(BufferQBO *self);`

  > Создать буфер кадра:</br>
  > `BufferFBO* BufferFBO_create(int width, int height);`

  > Уничтожить буфер кадра:</br>
  > `void BufferFBO_destroy(BufferFBO **fbo);`

  > Использовать буфер:</br>
  > `void BufferFBO_begin(BufferFBO *self);`

  > Не использовать буфер:</br>
  > `void BufferFBO_end(BufferFBO *self);`

  > Очистить буфер:</br>
  > `void BufferFBO_clear(BufferFBO *self, float r, float g, float b, float a);`

  > Изменить размер кадрового буфера:</br>
  > `void BufferFBO_resize(BufferFBO *self, int width, int height);`

  > Активировать привязку для записи в неё данных:</br>
  > `void BufferFBO_active(BufferFBO *self, uint32_t attachment);`

  > Применить массив привязок для записи данных в них:</br>
  > `void BufferFBO_apply(BufferFBO *self);`

  > Скопировать цвет и глубину в другой кадровый буфер:</br>
  > `void BufferFBO_blit(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);`

  > Скопировать только цвет в другой кадровый буфер:</br>
  > `void BufferFBO_blit_color(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);`

  > Скопировать только глубину в другой кадровый буфер:</br>
  > `void BufferFBO_blit_depth(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);`

  > Скопировать только маску в другой кадровый буфер:</br>
  > `void BufferFBO_blit_stencil(BufferFBO *self, uint32_t dest_fbo_id, int x, int y, int width, int height);`

  > Привязать 2D текстуру:</br>
  > `void BufferFBO_attach(BufferFBO *self, BufferFBO_Type type, uint32_t attachment, uint32_t tex_id);`

  > Создать буфер вершин:</br>
  > `BufferVBO* BufferVBO_create(const void* data, const size_t size, int mode);`

  > Уничтожить буфер вершин:</br>
  > `void BufferVBO_destroy(BufferVBO **vbo);`

  > Использовать буфер:</br>
  > `void BufferVBO_begin(BufferVBO *self);`

  > Не использовать буфер:</br>
  > `void BufferVBO_end(BufferVBO *self);`

  > Получить размер буфера:</br>
  > `size_t BufferVBO_get_size(BufferVBO *self);`

  > Установить данные буфера (выделяет новую память и заново всё сохраняет):</br>
  > `void BufferVBO_set_data(BufferVBO *self, const void *data, const size_t size, int mode);`

  > Изменить данные буфера (не выделяет новую память а просто изменяет данные):</br>
  > `void BufferVBO_set_subdata(BufferVBO *self, const void *data, const size_t offset, const size_t size);`

  > Создать буфер индексов:</br>
  > `BufferEBO* BufferEBO_create(const void* data, const size_t size, int mode);`

  > Уничтожить буфер индексов:</br>
  > `void BufferEBO_destroy(BufferEBO **vbo);`

  > Использовать буфер:</br>
  > `void BufferEBO_begin(BufferEBO *self);`

  > Не использовать буфер:</br>
  > `void BufferEBO_end(BufferEBO *self);`

  > Получить размер буфера:</br>
  > `size_t BufferEBO_get_size(BufferEBO *self);`

  > Установить данные буфера (выделяет новую память и заново всё сохраняет):</br>
  > `void BufferEBO_set_data(BufferEBO *self, const void *data, const size_t size, int mode);`

  > Изменить данные буфера (не выделяет новую память а просто изменяет данные):</br>
  > `void BufferEBO_set_subdata(BufferEBO *self, const void *data, const size_t offset, const size_t size);`

  > Создать буфер атрибутов:</br>
  > `BufferVAO* BufferVAO_create();`

  > Уничтожить буфер атрибутов:</br>
  > `void BufferVAO_destroy(BufferVAO **vao);`

  > Использовать буфер:</br>
  > `void BufferVAO_begin(BufferVAO *self);`

  > Не использовать буфер:</br>
  > `void BufferVAO_end(BufferVAO *self);`

  > Установить атрибуты вершин:</br>
  > `void BufferVAO_attrib_pointer( BufferVAO *self, uint32_t loc, int count, int type, bool normalize, size_t stride, size_t offset );`

  > Установить дивизор:</br>
  > `void BufferVAO_attrib_divisor( BufferVAO *self, uint32_t loc, int count, int type, bool normalize, size_t stride, size_t offset, uint32_t divisor );`

#

### Конец генерации.
Всего файлов обработано: 37
