//
// files.h
//

#pragma once


// Подключаем:
#include "std.h"


// Загружаем файл в строку:
char* Files_load(const char* file_path, const char* mode);

// Сохраняем строку в файл:
bool Files_save(const char* file_path, const char* data, const char* mode);

// Загружаем файл в буфер бинарно:
unsigned char* Files_load_bin(const char* file_path, const char* mode, size_t* out_size);

// Сохраняем буфер в файл бинарно:
bool Files_save_bin(const char* file_path, const void* data, size_t size, const char* mode);
