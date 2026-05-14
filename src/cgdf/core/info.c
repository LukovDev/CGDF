//
// info.c - Реализует функции для получения информации о CPU и RAM.
//


// Подключаем:
#include "std.h"
#if defined(_WIN32)
    #include <windows.h>
    #include <intrin.h>
#elif defined(__APPLE__)
    #include <unistd.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/sysinfo.h>
#endif
#if (defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)) && !defined(_WIN32)
    #include <cpuid.h>
#endif
#include "info.h"


// Глобальные переменные:
bool g_Info_cpu_cached_ = false;
CpuInfo g_Info_cpu_info_cache_ = {0};


// Получить архитектуру процессора в виде строки:
const char* Info_get_cpu_arch_name(Info_cpu_arch arch) {
    switch (arch) {
        case INFO_X86_64: return "x86_64";
        case INFO_X86:    return "x86";
        case INFO_ARM64:  return "ARM64";
        case INFO_ARM:    return "ARM";
        default:          return "Unknown";
    }
}


// Функция для получения информации о процессоре:
CpuInfo Info_get_cpu(void) {
    if (g_Info_cpu_cached_) return g_Info_cpu_info_cache_;  // Используем кэш.

    CpuInfo info = {.threads = 1, .arch = INFO_UNKNOWN};
    memset(info.model, 0, sizeof(info.model));  // Обнуляем строку модели процессора.

    // Определяем архитектуру процессора:
    #if defined(__x86_64__) || defined(_M_X64)
        info.arch = INFO_X86_64;
    #elif defined(__i386__) || defined(_M_IX86)
        info.arch = INFO_X86;
    #elif defined(__aarch64__) || defined(_M_ARM64)
        info.arch = INFO_ARM64;
    #elif defined(__arm__) || defined(_M_ARM)
        info.arch = INFO_ARM;
    #else
        info.arch = INFO_UNKNOWN;
    #endif

    // Получаем модель процессора:
    #if defined(_WIN32)
        int regs[4];
        char name[49] = {0};
        for (int i = 0; i < 3; i++) {
            __cpuid(regs, 0x80000002 + i);
            memcpy(name + i * 16, regs, 16);
        }
        strncpy(info.model, name, sizeof(info.model) - 1);
    #elif defined(__APPLE__)
        size_t len = sizeof(info.model);
        if (sysctlbyname("machdep.cpu.brand_string", info.model, &len, NULL, 0) != 0) {
            strncpy(info.model, "Apple Silicon", sizeof(info.model) - 1);
        }
    #elif defined(__linux__)
        if (info.arch == INFO_X86_64 || info.arch == INFO_X86) {
            #if defined(__cpuid_available) || defined(__GNUC__)
                uint32_t regs[4];
                char name[49] = {0};
                for (int i = 0; i < 3; i++) {
                    __get_cpuid(0x80000002 + i, &regs[0], &regs[1], &regs[2], &regs[3]);
                    memcpy(name + i * 16, regs, 16);
                }
                strncpy(info.model, name, sizeof(info.model) - 1);
            #endif
        } else {
            FILE* f = fopen("/proc/cpuinfo", "r");
            if (f) {
                char line[256];
                while (fgets(line, sizeof(line), f)) {
                    if (strstr(line, "model name") ||
                        strstr(line, "Model name") ||
                        strstr(line, "Hardware") ||
                        strstr(line, "Model"))
                    {
                        char* col = strchr(line, ':');
                        if (col && *(col + 1) != '\0') {
                            const char* val = col + 1;
                            while (*val == ' ') val++;
                            strncpy(info.model, val, sizeof(info.model) - 1);
                            break;
                        }
                    }
                }
                fclose(f);
            }
        }
    #endif

    // Если модель всё ещё пуста:
    if (info.model[0] == '\0') strncpy(info.model, "Generic", 63);

    // Очистка пробелов:
    char *p = info.model;
    while (*p == ' ') p++;
    if (p != info.model) memmove(info.model, p, strlen(p) + 1);
    info.model[sizeof(info.model) - 1] = '\0';
    for (int i = (int)strlen(info.model) - 1; i >= 0; i--) {
        if (info.model[i] == ' ' || info.model[i] == '\n' || info.model[i] == '\r') info.model[i] = '\0';
        else break;
    }

    // Получаем количество потоков:
    #if defined(_WIN32)
        SYSTEM_INFO si; GetSystemInfo(&si);
        info.threads = si.dwNumberOfProcessors;
    #elif defined(__APPLE__)
        int count; size_t c_len = sizeof(count);
        sysctlbyname("hw.ncpu", &count, &c_len, NULL, 0);
        info.threads = count;
    #elif defined(__linux__)
        info.threads = sysconf(_SC_NPROCESSORS_ONLN);
    #endif

    // Кэширование результата:
    if (!g_Info_cpu_cached_) {
        g_Info_cpu_cached_ = true;
        g_Info_cpu_info_cache_ = info;
    }
    return info;
}


// Функция для получения ОЗУ (в байтах):
MemInfo Info_get_mem(void) {
    MemInfo info = {.total = 0, .free = 0, .used = 0};
    #if defined(_WIN32)
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        info.total = status.ullTotalPhys;
        info.free = status.ullAvailPhys;
    #elif defined(__APPLE__)
        int64_t mem;
        size_t len = sizeof(mem);
        sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
        info.total = mem;
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics64_data_t vm_stats;
        if (host_statistics64(mach_host_self(), HOST_VM_INFO, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
            info.free = (size_t)vm_stats.free_count * sysconf(_SC_PAGESIZE);
        }
    #elif defined(__linux__)
        struct sysinfo si;
        sysinfo(&si);
        info.total = (size_t)si.totalram * si.mem_unit;
        info.free = (size_t)si.freeram * si.mem_unit;
    #endif
    info.used = info.total - info.free;
    return info;
}
