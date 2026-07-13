//
// std.h - Заголовочный файл, импортирующий все минимальные стандартные библиотеки.
//

#pragma once

#if !defined(_WIN32) && !defined(__APPLE__)
    #define _POSIX_C_SOURCE 200809L
#endif


// Подключаем:
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <limits.h>
#include <stdalign.h>
