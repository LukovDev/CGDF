//
// files.c - Реализует работу с файлами.
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "logger.h"
#include "files.h"
#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif


// Код для исправления путей для OS X:
#ifdef __APPLE__
#include <mach-o/dyld.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static bool set_cwd_to_prog_dir() {
    uint32_t size = PATH_MAX;
    char path[PATH_MAX];
    char *buf = path;

    if (_NSGetExecutablePath(buf, &size) != 0) {
        buf = (char*)mm_alloc(size);
        if (!buf) return false;
        if (_NSGetExecutablePath(buf, &size) != 0) {
            mm_free(buf);
            return false;
        }
    }

    char resolved[PATH_MAX];
    const char *use_path = buf;
    if (realpath(buf, resolved)) {
        use_path = resolved;
    }

    char *slash = strrchr((char*)use_path, '/');
    if (!slash) {
        if (buf != path) mm_free(buf);
        return false;
    }

    if (slash == use_path) {
        slash[1] = '\0';
    } else {
        *slash = '\0';
    }

    bool ok = (chdir(use_path) == 0);
    if (buf != path) mm_free(buf);
    return ok;
}

void Files_fix_apple_path() {
    char cwd[PATH_MAX];
    const char *home = Files_get_home();
    if (Files_get_cwd(cwd, sizeof(cwd))) {
        bool is_home = (home && strcmp(cwd, home) == 0);
        bool is_root = (strcmp(cwd, "/") == 0);
        if (is_home || is_root) {
            if (!set_cwd_to_prog_dir()) {
                log_msg("[W] Failed to set CWD to executable directory.\n");
            }
        }
    }
}
#endif


// Получить текущую директорию:
char *Files_get_cwd(char *buf, size_t size) {
    #ifdef _WIN32
        return _getcwd(buf, size);
    #else
        return getcwd(buf, size);
    #endif
}


// Переход к каталогу:
bool Files_chdir(const char *path) {
    #ifdef _WIN32
        return _chdir(path) == 0;
    #else
        return chdir(path) == 0;
    #endif
}


// Получить путь домашнего каталога:
char *Files_get_home() {
    char *dir = getenv("HOME");

    // Если HOME не задан (Windows), пробуем USERPROFILE:
    if (dir == NULL) {
        dir = getenv("USERPROFILE");
    }
    return dir;
}


// Загружаем файл в строку:
char* Files_load(const char* file_path, const char* mode) {
    FILE* f = fopen(file_path, mode);
    if (!f) return NULL;

    // Определяем размер файла:
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    // Выделяем память и +1 для '\0':
    char* buffer = (char*)mm_alloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    // Читаем файл:
    size_t read_size = fread(buffer, 1, size, f);
    buffer[read_size] = '\0';
    fclose(f);

    return buffer;
}


// Сохраняем строку в файл:
bool Files_save(const char* file_path, const char* data, const char* mode) {
    FILE* f = fopen(file_path, mode);
    if (!f) return false;

    fwrite(data, 1, strlen(data), f);
    fclose(f);
    return true;
}


// Загружаем файл в буфер бинарно:
unsigned char* Files_load_bin(const char* file_path, const char* mode, size_t* out_size) {
    FILE* f = fopen(file_path, mode);
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char* buffer = (unsigned char*)mm_alloc(size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, f);
    fclose(f);

    if (out_size) *out_size = read_size;
    return buffer;
}


// Сохраняем буфер в файл бинарно:
bool Files_save_bin(const char* file_path, const void* data, size_t size, const char* mode) {
    FILE* f = fopen(file_path, mode);
    if (!f) return false;

    fwrite(data, 1, size, f);
    fclose(f);
    return true;
}
