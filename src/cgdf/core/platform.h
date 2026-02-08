//
// platform.h - Определяет платформу, на которой компилируется код.
//

#pragma once


// Платформа Windows:
static inline bool is_windows(void) {
    #if defined(_WIN32) || defined(_WIN64)
        return true;
    #else
        return false;
    #endif
}


// Платформа MacOS:
static inline bool is_macos(void) {
    #if defined(__APPLE__) && defined(__MACH__)
        return true;
    #else
        return false;
    #endif
}


// Платформа Linux:
static inline bool is_linux(void) {
    #ifdef __linux__
        return true;
    #else
        return false;
    #endif
}


// Получить название платформы:
static inline const char* get_platform_name(void) {
    if (is_windows()) return "Windows";
    if (is_macos())   return "MacOS";
    if (is_linux())   return "Linux";
    return "Unknown";
}
