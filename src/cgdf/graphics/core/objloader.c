//
// objloader.c - Реализует загрузчик моделей в формате OBJ/MTL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include <cgdf/core/files.h>
#include <cgdf/core/logger.h>
#include "material.h"
#include "mesh.h"
#include "model.h"
#include "renderer.h"
#include "objloader.h"


// -------- Вспомогательные структуры: --------


// Индекс вершины полигона (определение параметров вершины из формата "f p/t/n p/t/n ..."):
typedef struct {
    int p, t, n;  // p - индекс позиции, t - индекс текстурных координат, n - индекс нормали.
} ObjIndex;


// -------- Вспомогательные функции: --------


// Пропускаем пробелы и другие символы в строке. Сдвигаем pointer на первый нормальный символ:
static char* skip_ws(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    return s;
}

// Убираем пробелы и другие символы в конце строки:
static void trim_end(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

// Конвертируем индекс из obj файла в индекс массива:
static int convert_obj_index(int index, Array *arr) {
    int len = (int)Array_len(arr);
    if (index > 0) return index - 1;
    if (index < 0) return len + index;
    return -1;
}

// Получить индексы из строки:
static bool parse_face_token(const char *token, ObjIndex *idx) {
    if (!token || !idx) return false;
    *idx = (ObjIndex){0, 0, 0};

    int slash_count = 0;
    for (const char *c = token; *c; c++) {
        if (*c == '/') slash_count++;
    }

    if (slash_count == 0) return sscanf(token, "%d", &idx->p) == 1;
    if (slash_count == 1) return sscanf(token, "%d/%d", &idx->p, &idx->t) >= 1;
    if (strstr(token, "//")) return sscanf(token, "%d//%d", &idx->p, &idx->n) >= 1;
    return sscanf(token, "%d/%d/%d", &idx->p, &idx->t, &idx->n) >= 1;
}

// Собрать полноценный Vertex из OBJ индексов:
static bool make_vertex(ObjIndex idx, Array *positions, Array *normals, Array *texcoords, Vertex *out) {
    // Конвертируем индексы:
    int p_index = convert_obj_index(idx.p, positions);
    int n_index = convert_obj_index(idx.n, normals);
    int t_index = convert_obj_index(idx.t, texcoords);

    Vec3d *p_ptr = (Vec3d*)Array_get(positions, p_index);
    if (!p_ptr) return false;

    // Получаем нормали и текстурные координаты:
    Vec3d *n_ptr = (Vec3d*)Array_get(normals, n_index);
    Vec2d *t_ptr = (Vec2d*)Array_get(texcoords, t_index);
    Vec3d p = *p_ptr;
    Vec3d n = n_ptr ? *n_ptr : (Vec3d){0.0, 0.0, 0.0};
    Vec2d t = t_ptr ? *t_ptr : (Vec2d){0.0, 0.0};

    // Создаём вершину:
    *out = (Vertex){p.x, p.y, p.z, n.x, n.y, n.z, 1.0f, 1.0f, 1.0f, 1.0f, t.x, -t.y};
    return true;
}

// Закончить текущий меш и добавить в модель:
static void flush_mesh(Model *model, Array *vertices, Array *indices, Material *material) {
    if (!model || Array_len(vertices) == 0 || Array_len(indices) == 0) return;

    Mesh *mesh = Mesh_create(
        vertices->data, (uint32_t)Array_len(vertices),
        indices->data, (uint32_t)Array_len(indices), 
        false, material
    );
    Model_add_mesh(model, mesh);
    Array_clear(vertices, false);
    Array_clear(indices, false);
}

// Закончить текущую модель и добавить в массив моделей:
static void flush_model(OBJFile *objfile, Model **model) {
    if (!objfile || !model || !*model) return;
    // Если сеток в модели больше нуля (хотя бы одна) то добавляем в модели, иначе удаляем:
    if (Array_len((*model)->meshes) > 0) {
        Array_push(objfile->models, model);
    } else {
        Model_destroy(model);
    }
    *model = NULL;
}


// -------- Код для парсинга MTL-файла: --------


// Найти материал в массиве материалов по имени:
static Material* find_material(Array *materials, const char *name) {
    if (!materials || !name) return NULL;
    for (size_t i = 0; i < Array_len(materials); i++) {
        Material *mat = (Material*)Array_get_ptr(materials, i);
        if (mat && mat->name && strcmp(mat->name, name) == 0) return mat;
    }
    return NULL;
}

// Загрузить текстуру из MTL-файла:
static Texture* load_texture(Renderer *renderer, const char *mtl_dir, const char *raw_path) {
    char *path = Files_path_join(mtl_dir, raw_path);
    if (!path) return NULL;

    Texture *texture = Texture_create(renderer);
    Texture_load(texture, path, true);
    if (!texture || texture->id == 0) {
        Texture_destroy(&texture);
        log_msg("[W] ObjLoader_load: texture not loaded: %s\n", path);
    }

    mm_free(path);
    return texture;
}

// Парсить путь к текстуре:
static char* parse_texture_path(char *args) {
    char *last = NULL;
    char *token = strtok(args, " \t\r\n");
    while (token) {
        if (token[0] == '-') {
            token = strtok(NULL, " \t\r\n");
            while (token && token[0] != '-') {
                last = token;
                token = strtok(NULL, " \t\r\n");
            }
            continue;
        }
        last = token;
        token = strtok(NULL, " \t\r\n");
    }
    return last;
}

// Парсить MTL-файл:
static void parse_mtl_file(Renderer *renderer, const char *filepath, Array *materials) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        log_msg("[W] ObjLoader_load: MTL file not found: %s\n", filepath);
        return;
    }

    // Получаем папку MTL-файла:
    char *mtl_dir = Files_dirname_dup(filepath);
    char line[2048];
    Material *mat = NULL;

    // Читаем файл построчно:
    while (fgets(line, sizeof(line), f)) {
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';

        // Чистим строку от мусора:
        char *s = skip_ws(line);
        trim_end(s);
        if (!*s) continue;

        // Создаём новый материал:
        if (strncmp(s, "newmtl", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *name = skip_ws(s + 6);
            // Если не нашли материал в массиве материалов, то создаем новый и добавляем его:
            if (!find_material(materials, name)) {
                mat = Material_create_default(name);
                Array_push(materials, &mat);
            // Иначе если материал с таким именем уже есть, то берём его:
            } else {
                mat = find_material(materials, name);
            }
        }

        // Фоновый цвет:
        else if (mat && strncmp(s, "Ka", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f %f %f", &mat->ambient.x, &mat->ambient.y, &mat->ambient.z);
        }

        // Цвет материала:
        else if (mat && strncmp(s, "Kd", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f %f %f", &mat->albedo.x, &mat->albedo.y, &mat->albedo.z);
        }

        // Зеркальный цвет отражения:
        else if (mat && strncmp(s, "Ks", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f %f %f", &mat->specular.x, &mat->specular.y, &mat->specular.z);
            mat->specular_strength = (mat->specular.x + mat->specular.y + mat->specular.z) / 3.0f;
        }

        // Прозрачность:
        else if (mat && strncmp(s, "d", 1) == 0 && (s[1] == ' ' || s[1] == '\t')) {
            sscanf(s + 1, "%f", &mat->albedo.w);
            mat->transparent = mat->albedo.w < 1.0f;  // Если альфа цвета ниже 1.0, значит прозрачный.
        }

        // Обратная прозрачность:
        else if (mat && strncmp(s, "Tr", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            float tr = 0.0f;
            sscanf(s + 2, "%f", &tr);
            mat->albedo.w = 1.0f - tr;
            mat->transparent = mat->albedo.w < 1.0f;  // Если альфа цвета ниже 1.0, значит прозрачный.
        }

        // Цвет свечения:
        else if (mat && strncmp(s, "Ke", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f %f %f", &mat->emissive_color.x, &mat->emissive_color.y, &mat->emissive_color.z);
            mat->emissive_strength = (mat->emissive_color.x+mat->emissive_color.y+mat->emissive_color.z) > 0 ? 1 : 0;
        }

        // Спекулярность (блеск) Blinn-Phong:
        else if (mat && strncmp(s, "Ns", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            float ns = 0.0f;
            sscanf(s + 2, "%f", &ns);
            mat->roughness = 1.0f - (ns / 1000.0f);
            if (mat->roughness < 0.0f) mat->roughness = 0.0f;
            if (mat->roughness > 1.0f) mat->roughness = 1.0f;
        }

        // PBR: Металличность:
        else if (mat && strncmp(s, "Pm", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f", &mat->metallic);
        }

        // PBR: Шероховатость:
        else if (mat && strncmp(s, "Pr", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            sscanf(s + 2, "%f", &mat->roughness);
        }

        // Текстура альбедо:
        else if (mat && strncmp(s, "map_Kd", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *path = parse_texture_path(skip_ws(s + 6));
            // Загружает только если текстура не указана:
            if (path && !mat->albedo_map) {
                mat->albedo_map = load_texture(renderer, mtl_dir, path);
                mat->owns_albedo_map = mat->albedo_map != NULL;
            }
        }

        // Карта прозрачности (влияет на альфа-канал):
        else if (mat && (strncmp(s, "map_d", 5) == 0 && (s[5] == ' ' || s[5] == '\t'))) {
            char *path = parse_texture_path(skip_ws(s + 5));
            if (path) {
                mat->transparent = true; 
                // Обычно объединяется с albedo_map в рантайме или шейдере.
            }
        }

        // Карты нормалей/бампа:
        else if (mat && (strncmp(s, "map_Bump", 8) == 0 || strncmp(s, "map_bump", 8) == 0
                || strncmp(s, "bump", 4) == 0 || strncmp(s, "norm", 4) == 0)) {
            char *args = s + (s[0] == 'b' ? 4 : (s[4] == '_' ? 8 : 4));
            char *path = parse_texture_path(skip_ws(args));
            // Загружает только если текстура не указана:
            if (path && !mat->normal_map) {
                mat->normal_map = load_texture(renderer, mtl_dir, path);
                mat->owns_normal_map = mat->normal_map != NULL;
            }
        }

        // Текстура свечения:
        else if (mat && strncmp(s, "map_Ke", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *path = parse_texture_path(skip_ws(s + 6));
            // Загружает только если текстура не указана:
            if (path && !mat->emissive_map) {
                mat->emissive_map = load_texture(renderer, mtl_dir, path);
                mat->owns_emissive_map = mat->emissive_map != NULL;
                if (mat->emissive_map && mat->emissive_strength == 0.0f) mat->emissive_strength = 1.0f;
            }
        }

        // Текстура высоты (параллакс):
        else if (mat && (strncmp(s, "disp", 4) == 0 || strncmp(s, "map_disp", 8) == 0)) {
            char *args = s + (s[0] == 'd' ? 4 : 8);
            char *path = parse_texture_path(skip_ws(args));
            // Загружает только если текстура не указана:
            if (path && !mat->height_map) {
                mat->height_map = load_texture(renderer, mtl_dir, path);
                mat->owns_height_map = mat->height_map != NULL;
            }
        }

        // PBR: Карта окклюзии (Ambient Occlusion):
        else if (mat && (strncmp(s, "map_Ao", 6) == 0 || strncmp(s, "map_ao", 6) == 0) && (s[6] == ' ' || s[6] == '\t')) {
            char *path = parse_texture_path(skip_ws(s + 6));
            // Загружает только если текстура не указана:
            if (path && !mat->occlusion_map) {
                mat->occlusion_map = load_texture(renderer, mtl_dir, path);
                mat->owns_occlusion_map = mat->occlusion_map != NULL;
            }
        }

        // PBR: Карта шероховатости (Roughness):
        else if (mat && strncmp(s, "map_Pr", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *path = parse_texture_path(skip_ws(s + 6));
            // Загружает только если текстура не указана:
            if (path && !mat->roughness_map) {
                mat->roughness_map = load_texture(renderer, mtl_dir, path);
                mat->owns_roughness_map = mat->roughness_map != NULL;
            }
        }

        // PBR: Карта металличности (Metallic):
        else if (mat && strncmp(s, "map_Pm", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *path = parse_texture_path(skip_ws(s + 6));
            // Загружает только если текстура не указана:
            if (path && !mat->metallic_map) {
                mat->metallic_map = load_texture(renderer, mtl_dir, path);
                mat->owns_metallic_map = mat->metallic_map != NULL;
            }
        }
    }

    mm_free(mtl_dir);
    fclose(f);
}


// -------- API загрузчика: --------


// Загрузить модели из OBJ-файла с материалами:
OBJFile ObjLoader_load(Renderer *renderer, const char *filepath) {
    if (!renderer || !filepath) return (OBJFile){NULL, NULL};

    // Создаём структуру:
    OBJFile objfile = {
        .models = Array_create(sizeof(void*), ARRAY_DEFAULT_CAPACITY),
        .materials = Array_create(sizeof(void*), ARRAY_DEFAULT_CAPACITY),
    };

    // Открываем файл:
    FILE *f = fopen(filepath, "r");
    if (!f) {
        Array_destroy(&objfile.models);
        Array_destroy(&objfile.materials);
        return (OBJFile){NULL, NULL};
    }

    // Создаём временные массивы:
    char *obj_dir = Files_dirname_dup(filepath);
    Array *positions = Array_create(sizeof(Vec3d), ARRAY_DEFAULT_CAPACITY);
    Array *normals = Array_create(sizeof(Vec3d), ARRAY_DEFAULT_CAPACITY);
    Array *texcoords = Array_create(sizeof(Vec2d), ARRAY_DEFAULT_CAPACITY);
    Array *vertices = Array_create(sizeof(Vertex), ARRAY_DEFAULT_CAPACITY);
    Array *indices = Array_create(sizeof(uint32_t), ARRAY_DEFAULT_CAPACITY);

    // Временные переменные для парсинга:
    Material *default_mat = Material_create_default(NULL);
    Array_push(objfile.materials, &default_mat);
    Material *current_mat = default_mat;
    Model *current_model = Model_create(renderer);

    // Парсим файл:
    char line[2048];  // Ограничение строки файла в 2048 символов.
    while (fgets(line, sizeof(line), f)) {
        // Чистим строку:
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        char *s = skip_ws(line);
        trim_end(s);
        if (!*s) continue;

        // Загружаем материалы:
        if (strncmp(s, "mtllib", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            char *libs = skip_ws(s + 6);
            // Проходимся по всем перечисленным файлам материалов:
            while (*libs) {
                // Чистим строку:
                char *start = skip_ws(libs);
                char *end = start;
                while (*end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n') end++;

                // Загружаем материалы из файла:
                char old = *end;
                *end = '\0';
                char *mtl_path = Files_path_join(obj_dir, start);
                parse_mtl_file(renderer, mtl_path, objfile.materials);
                mm_free(mtl_path);

                if (!old) break;
                *end = old;
                libs = end + 1;
            }
        }

        // Начинаем новую модель:
        else if ((s[0] == 'o' || s[0] == 'g') && (s[1] == ' ' || s[1] == '\t')) {
            flush_mesh(current_model, vertices, indices, current_mat);
            flush_model(&objfile, &current_model);
            current_model = Model_create(renderer);
        }

        // Если используем другой материал, то ищем или создаём новый материал и начинаем новый меш:
        else if (strncmp(s, "usemtl", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            flush_mesh(current_model, vertices, indices, current_mat);
            char *name = skip_ws(s + 6);
            Material *mat = find_material(objfile.materials, name);
            if (!mat) {
                mat = Material_create_default(name);
                Array_push(objfile.materials, &mat);
            }
            current_mat = mat;
        }

        // Сохраняем позиции, нормали и текстурные координаты:
        else if (s[0] == 'v' && s[1] == ' ') {
            Vec3d p;
            if (sscanf(s, "v %lf %lf %lf", &p.x, &p.y, &p.z) == 3) Array_push(positions, &p);
        } else if (s[0] == 'v' && s[1] == 'n') {
            Vec3d n;
            if (sscanf(s, "vn %lf %lf %lf", &n.x, &n.y, &n.z) == 3) Array_push(normals, &n);
        } else if (s[0] == 'v' && s[1] == 't') {
            Vec2d t;
            if (sscanf(s, "vt %lf %lf", &t.x, &t.y) >= 1) Array_push(texcoords, &t);
        }

        // Обрабатываем поверхности геометрии:
        else if (s[0] == 'f' && (s[1] == ' ' || s[1] == '\t')) {
            ObjIndex face[64];
            int face_count = 0;
            char *token = strtok(s, " \t\r\n");
            while ((token = strtok(NULL, " \t\r\n"))) {
                if (face_count >= (int)(sizeof(face) / sizeof(face[0]))) break;
                ObjIndex idx;
                if (parse_face_token(token, &idx)) face[face_count++] = idx;
            }

            // Триангулируем поверхность:
            for (int i = 1; i < face_count - 1; i++) {
                Vertex v0, v1, v2;
                if (!make_vertex(face[0], positions, normals, texcoords, &v0) ||
                    !make_vertex(face[i], positions, normals, texcoords, &v1) ||
                    !make_vertex(face[i + 1], positions, normals, texcoords, &v2)) {
                    continue;
                }

                uint32_t base = (uint32_t)Array_len(vertices);
                Array_push(vertices, &v0);
                Array_push(vertices, &v1);
                Array_push(vertices, &v2);
                Array_push(indices, &(uint32_t){base + 0});
                Array_push(indices, &(uint32_t){base + 1});
                Array_push(indices, &(uint32_t){base + 2});
            }
        }
    }

    // Закрываем последний меш и модель:
    flush_mesh(current_model, vertices, indices, current_mat);
    flush_model(&objfile, &current_model);

    // Освобождаем временные массивы и закрываем файл:
    Array_destroy(&positions);
    Array_destroy(&normals);
    Array_destroy(&texcoords);
    Array_destroy(&vertices);
    Array_destroy(&indices);
    mm_free(obj_dir);
    fclose(f);

    return objfile;
}
