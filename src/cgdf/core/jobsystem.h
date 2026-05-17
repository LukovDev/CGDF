//
// jobsystem.h - Работа с задачами (потоками).
//
// Не подходит под мелкие задачи.
// JobSystem сам определяет количество потоков процессора.
// При создании задачи, он попадает в стек задач, и если можно создать поток, то он создаётся.
// Каждый поток отслеживает стек, и вытягивает самую старую задачу и выполняет её,
// до тех пор пока стек не опустеет. Как только стек пуст, поток завершается.
// В этом алгоритме нет пула потоков. Зато архитектура чиста и проста, а процессор не нагружается в простое.
//

#pragma once


// Подключаем:
#include "std.h"
#include "array.h"
#include "libs.h"


// Определения:
typedef int (*JobFunction)(void *args);  // Указатель на функцию, которую мы будем выполнять.


// Объявление структур:
typedef struct JobSystem JobSystem;  // Структура работы с задачами (потоками).
typedef struct JobTask JobTask;      // Задача которую мы будем выполнять.


// Структура работы с задачами (потоками):
struct JobSystem {
    bool initialized;          // Инициализирована ли работа с задачами.
    size_t worker_count;       // Текущее количество потоков.
    size_t max_workers_count;  // Максимальное количество задач.
    Array *stack;              // Стек задач.
    mtx_t mutex;               // Мьютекс для защиты работы стека и счетчиков.
};


// Структура задачи:
struct JobTask {
    JobFunction function;  // Функция, которую мы будем выполнять.
    void *args;             // Аргумент задачи.
};


// Глобальный объект работы с задачами (потоками):
extern JobSystem g_JobSystem;


// Инициализация работы с задачами (потоками):
void JobSystem_init(void);

// Прекращение работы с задачами (потоками):
void JobSystem_destroy(void);

// Создать задачу:
void JobSystem_create_job(JobFunction func, void *args);

// Есть ли ещё работающие задачи:
bool JobSystem_has_active_jobs(void);

// Получить количество задач в очереди (стеке):
size_t JobSystem_get_jobs_count(void);

// Получить количество активных потоков:
size_t JobSystem_get_active_workers_count(void);

// Получить максимальное количество потоков:
size_t JobSystem_get_max_workers_count(void);
