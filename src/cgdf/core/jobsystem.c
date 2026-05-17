//
// jobsystem.c - Реализация работы с задачами (потоками).
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "array.h"
#include "logger.h"
#include "libs.h"
#include "info.h"
#include "jobsystem.h"


// Глобальный объект работы с задачами (потоками):
JobSystem g_JobSystem;


// Внутренняя функция потока, выполняющая задачи в цикле:
static int _JobSystem_task_work_(void *args) {
    (void)args;  // Не используется.
    JobTask current_job;

    // Вечный цикл потока:
    while (true) {
        // Проверяем, есть ли задачи в стеке:
        mtx_lock(&g_JobSystem.mutex);
        if (Array_len(g_JobSystem.stack) > 0) {
            // Стек не пуст. Извлекаем самую старую задачу и удаляем из стека:
            Array_remove(g_JobSystem.stack, 0, &current_job);
            mtx_unlock(&g_JobSystem.mutex);
            int result = current_job.function(current_job.args);
            if (result != 0) log_msg("[E] JobSystem: Task returned error: %d\n", result);
        } else {
            // Стек пуст. Этот поток больше не нужен. Завершаем и самоликвидируемся:
            g_JobSystem.worker_count--;
            mtx_unlock(&g_JobSystem.mutex);
            break;
        }
    }
    return 0;
}


// Инициализация работы с задачами (потоками):
void JobSystem_init(void) {
    if (g_JobSystem.initialized) return;
    g_JobSystem.worker_count = 0;
    g_JobSystem.max_workers_count = Info_get_cpu().threads;  // Потоки процессора.
    if (g_JobSystem.max_workers_count == 0) g_JobSystem.max_workers_count = 1;  // Минимум 1.
    g_JobSystem.stack = Array_create(sizeof(JobTask), g_JobSystem.max_workers_count);
    mtx_init(&g_JobSystem.mutex, mtx_plain);
    g_JobSystem.initialized = true;
}


// Прекращение работы с задачами (потоками):
void JobSystem_destroy(void) {
    if (!g_JobSystem.initialized) return;
    Array_destroy(&g_JobSystem.stack);
    mtx_destroy(&g_JobSystem.mutex);
    g_JobSystem.worker_count = 0;
    g_JobSystem.initialized = false;
}


// Создать задачу:
void JobSystem_create_job(JobFunction func, void *args) {
    if (!g_JobSystem.initialized) return;

    // Сначала создаем задачу и кладем её в стек (в конец массива):
    JobTask new_job;
    new_job.function = func;
    new_job.args = args;
    mtx_lock(&g_JobSystem.mutex);
    Array_push(g_JobSystem.stack, &new_job);

    // Проверяем счетчик активных потоков (есть ли свободное место для нового потока):
    if (g_JobSystem.worker_count < g_JobSystem.max_workers_count) {
        g_JobSystem.worker_count++;
        mtx_unlock(&g_JobSystem.mutex);

        // Создаём поток:
        thrd_t thread;
        if (thrd_create(&thread, _JobSystem_task_work_, NULL) == thrd_success) thrd_detach(thread);
        else {
            // Откат счетчика, если операционная система не смогла выделить поток
            mtx_lock(&g_JobSystem.mutex);
            g_JobSystem.worker_count--;
            mtx_unlock(&g_JobSystem.mutex);
        }
    } else mtx_unlock(&g_JobSystem.mutex);  // Нет свободных мест. Оставим задачу в стеке.
}


// Есть ли ещё работающие задачи:
bool JobSystem_has_active_jobs(void) {
    if (!g_JobSystem.initialized) return false;
    mtx_lock(&g_JobSystem.mutex);
    bool has_active = (g_JobSystem.worker_count > 0 || Array_len(g_JobSystem.stack) > 0);
    mtx_unlock(&g_JobSystem.mutex);
    return has_active;
}


// Получить количество задач в очереди (стеке):
size_t JobSystem_get_jobs_count(void) {
    if (!g_JobSystem.initialized) return 0;
    mtx_lock(&g_JobSystem.mutex);
    size_t count = Array_len(g_JobSystem.stack);
    mtx_unlock(&g_JobSystem.mutex);
    return count;
}


// Получить количество активных потоков:
size_t JobSystem_get_active_workers_count(void) {
    if (!g_JobSystem.initialized) return 0;
    mtx_lock(&g_JobSystem.mutex);
    size_t count = g_JobSystem.worker_count;
    mtx_unlock(&g_JobSystem.mutex);
    return count;
}


// Получить максимальное количество потоков:
size_t JobSystem_get_max_workers_count(void) {
    if (!g_JobSystem.initialized) return 0;
    return g_JobSystem.max_workers_count;
}
