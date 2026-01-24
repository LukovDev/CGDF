//
// main.c - Основной файл программы.
//


// Подключаем:
#include <cgdf/cgdf.h>


// Точка входа в программу:
int main(int argc, char *argv[]) {
    CGDF_Init();

    const char* cgdf_version = CGDF_GetVersion();
    log_msg("CGDF version: %s\n", cgdf_version);

    return 0;
}
