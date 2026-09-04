//
// jobsystem.c - Реализация работы с задачами (потоками).
//
// Создаёт отдельные системные потоки (воркеры) по всем ядрам процессора,
// принимает другие задачи в пул потоков, и раздаёт их воркерам на выполнение.
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "array.h"
#include "logger.h"
#include "libs.h"
#include "info.h"
#include "time.h"
#include "jobsystem.h"


// Глобальный объект работы с задачами (потоками):
JobSystem g_JobSystem;


// Внутренняя функция потока, выполняющая задачи в цикле:
static int _JobSystem_task_work_(void *args) {
    (void)args;  // Не используется.
    JobTask current_job;

    // Увеличиваем счетчик реальных потоков:
    mtx_lock(&g_JobSystem.mutex);
    g_JobSystem.real_workers_count++;
    mtx_unlock(&g_JobSystem.mutex);

    // Вечный цикл потока:
    while (true) {
        mtx_lock(&g_JobSystem.mutex);

        // Пока задач нет, поток засыпает здесь и ждет сигнала:
        while (Array_len(g_JobSystem.stack) == 0 && g_JobSystem.initialized) {
            cnd_wait(&g_JobSystem.cond_var, &g_JobSystem.mutex);  // Уснул, отдав мьютекс.
        }

        // Если мы больше не инициализированы, то выходим:
        if (!g_JobSystem.initialized) {
            g_JobSystem.real_workers_count--;  // Поток умирает, уменьшаем счетчик живых.
            mtx_unlock(&g_JobSystem.mutex);
            break; 
        }

        // Проснулся, забрал задачу:
        Array_remove(g_JobSystem.stack, 0, &current_job);
        g_JobSystem.workers_count++;  // Поток занят работой.
        mtx_unlock(&g_JobSystem.mutex);

        // Выполняем задачу:
        int result = current_job.function(current_job.args);
        if (result != 0) log_msg("[E] JobSystem: Task error: %d\n", result);

        mtx_lock(&g_JobSystem.mutex);
        g_JobSystem.workers_count--;  // Поток освободился.
        mtx_unlock(&g_JobSystem.mutex);
    }
    return 0;
}


// Инициализация работы с задачами (потоками):
void JobSystem_init(void) {
    if (g_JobSystem.initialized) return;
    g_JobSystem.initialized = true;
    g_JobSystem.workers_count = 0;
    g_JobSystem.real_workers_count = 0;
    g_JobSystem.max_workers_count = Info_get_cpu().threads;  // Потоки процессора.
    if (g_JobSystem.max_workers_count == 0) g_JobSystem.max_workers_count = 1;  // Минимум 1.
    g_JobSystem.stack = Array_create(sizeof(JobTask), g_JobSystem.max_workers_count);

    // Инициализируем мьютекс и переменную ожидания:
    mtx_init(&g_JobSystem.mutex, mtx_plain);
    cnd_init(&g_JobSystem.cond_var);

    // Создаём потоки-воркеры под все логические потоки проца для выполнения задач из виртуального пула потоков:
    mtx_lock(&g_JobSystem.mutex);
    for (size_t i = 0; i < g_JobSystem.max_workers_count; i++) {
        thrd_t thread;
        if (thrd_create(&thread, _JobSystem_task_work_, NULL) == thrd_success) {
            thrd_detach(thread);
        } else {
            log_msg("[E] JobSystem: Init failed to create thread %zu!\n", i);
        }
    }
    mtx_unlock(&g_JobSystem.mutex);
}


// Прекращение работы с задачами (потоками):
void JobSystem_destroy(void) {
    if (!g_JobSystem.initialized) return;

    // Останавливаем воркеры:
    mtx_lock(&g_JobSystem.mutex);
    g_JobSystem.initialized = false;
    cnd_broadcast(&g_JobSystem.cond_var);  // Будим все запущенные воркеры.
    mtx_unlock(&g_JobSystem.mutex);

    // Ждем завершения всех воркеров:
    while (true) {
        mtx_lock(&g_JobSystem.mutex);
        if (g_JobSystem.real_workers_count == 0) {
            mtx_unlock(&g_JobSystem.mutex);
            break;
        }
        mtx_unlock(&g_JobSystem.mutex);
        Time_delay(0.001);  
    }

    Array_destroy(&g_JobSystem.stack);
    mtx_destroy(&g_JobSystem.mutex);
    cnd_destroy(&g_JobSystem.cond_var);
    g_JobSystem.workers_count = 0;
    g_JobSystem.real_workers_count = 0;
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

    // Проверяем целостность запущенных реальных воркеров, и восстанавливаем недостающие:
    if (g_JobSystem.real_workers_count < g_JobSystem.max_workers_count) {
        size_t missing_workers = g_JobSystem.max_workers_count - g_JobSystem.real_workers_count;
        log_msg("[W] JobSystem: Detected pool corruption! Missing %zu workers. Reviving...\n", missing_workers);

        for (size_t i = 0; i < missing_workers; i++) {
            thrd_t thread;
            if (thrd_create(&thread, _JobSystem_task_work_, NULL) == thrd_success) {
                thrd_detach(thread);
            } else {
                log_msg("[E] JobSystem: Integrity check failed to revive thread!\n");
                break;
            }
        }
    }

    cnd_signal(&g_JobSystem.cond_var);  // Сообщаем одному случайному потоку, что есть задача.
    mtx_unlock(&g_JobSystem.mutex);
}


// Есть ли ещё работающие задачи:
bool JobSystem_has_active_jobs(void) {
    if (!g_JobSystem.initialized) return false;
    mtx_lock(&g_JobSystem.mutex);
    bool has_active = (g_JobSystem.workers_count > 0 || Array_len(g_JobSystem.stack) > 0);
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
    size_t count = g_JobSystem.workers_count;
    mtx_unlock(&g_JobSystem.mutex);
    return count;
}


// Получить количество реальных потоков:
size_t JobSystem_get_real_workers_count(void) {
    if (!g_JobSystem.initialized) return 0;
    return g_JobSystem.real_workers_count;
}


// Получить максимальное количество потоков:
size_t JobSystem_get_max_workers_count(void) {
    if (!g_JobSystem.initialized) return 0;
    return g_JobSystem.max_workers_count;
}
