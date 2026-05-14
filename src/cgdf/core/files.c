//
// files.c - Реализует работу с файлами.
//


// Подключаем:
#include "std.h"
#include "mm.h"
#include "logger.h"
#include "files.h"
#if defined(_WIN32)
    #include <direct.h>
    #define getcwd_os _getcwd
#else
    #include <unistd.h>
    #define getcwd_os getcwd
#endif


// Код для исправления путей для OS X:
#ifdef __APPLE__
#include <mach-o/dyld.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static bool set_cwd_to_prog_dir(void) {
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

void Files_fix_apple_path(void) {
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
char* Files_get_cwd(char *buf, size_t size) {
    if (buf != NULL && size == 0) return NULL;
    return getcwd_os(buf, size);
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
char* Files_get_home(void) {
    char *dir = getenv("HOME");

    // Если HOME не задан (Windows), пробуем USERPROFILE:
    if (dir == NULL) {
        dir = getenv("USERPROFILE");
    }
    return dir;
}


// Получить директорию файла (требуется освободить память):
char* Files_dirname_dup(const char *filepath) {
    const char *slash = strrchr(filepath, '/');
    const char *backslash = strrchr(filepath, '\\');
    const char *sep = slash > backslash ? slash : backslash;
    if (!sep) return mm_strdup(".");

    size_t len = (size_t)(sep - filepath);
    char *dir = (char*)mm_alloc(len + 1);
    memcpy(dir, filepath, len);
    dir[len] = '\0';
    return dir;
}


// Склеить пути (требуется освободить память):
char* Files_path_join(const char *dir, const char *path) {
    if (!path || !path[0]) return NULL;
    if (path[0] == '/' || path[0] == '\\' || (path[1] == ':' &&
            ((path[0] >= 'A' && path[0] <= 'Z') ||
            (path[0] >= 'a' && path[0] <= 'z')))) {
        return mm_strdup(path);
    }

    size_t dir_len = strlen(dir);
    size_t path_len = strlen(path);
    bool needs_sep = dir_len > 0 && dir[dir_len - 1] != '/' && dir[dir_len - 1] != '\\';
    char *out = (char*)mm_alloc(dir_len + (needs_sep ? 1 : 0) + path_len + 1);
    memcpy(out, dir, dir_len);
    if (needs_sep) out[dir_len++] = '/';
    memcpy(out + dir_len, path, path_len + 1);
    return out;
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
