//
// camera.c - Реализует работу камеры в OpenGL.
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../core/window.h"
#include "../core/camera.h"
#include "gl.h"


// -------- API 2D камеры: --------


// Создать 2D камеру:
Camera2D* Camera2D_create(Window *window, int width, int height, Vec2d position, float angle, float zoom) {
    Camera2D *camera = (Camera2D*)mm_alloc(sizeof(Camera2D));

    // Заполняем поля:
    camera->window = window;
    camera->position = position;
    camera->angle = angle;
    camera->zoom = zoom;
    camera->meter = 100.0f;  // По умолчанию, 1 метр = 100 пикселей.
    camera->width = width;
    camera->height = height;
    camera->_ui_begin_ = false;

    // Матрица вида:
    glm_mat4_identity(camera->view);
    glm_mat4_identity(camera->old_view);

    // Матрица проекции:
    glm_mat4_identity(camera->proj);
    glm_mat4_identity(camera->old_proj);

    // Установка проекции:
    Camera2D_resize(camera, width, height);

    // Масштабирование и перемещение:
    Camera2D_update(camera);
    return camera;
}

// Уничтожить 2D камеру:
void Camera2D_destroy(Camera2D **camera) {
    if (!camera || !*camera) return;
    mm_free(*camera);
    *camera = NULL;
}

// Обновление камеры:
void Camera2D_update(Camera2D *self) {
    if (!self || !self->window->renderer) return;
    Renderer *renderer = self->window->renderer;

    glm_mat4_identity(self->view);
    if (self->zoom != 0.0f) {
        glm_scale(self->view, (vec3){1.0f/self->zoom, 1.0f/self->zoom, 1.0f});
    } else {
        glm_scale(self->view, (vec3){0.0f, 0.0f, 1.0f});
    }
    glm_rotate(self->view, glm_rad(self->angle), (vec3){0, 0, 1.0f});
    glm_translate(self->view, (vec3){-self->position.x, -self->position.y, 0.0f});

    // Высчитываем матрицу взгляда:
    glm_mat4_mul(self->proj, self->view, self->view_proj);

    // Устанавливаем активную камеру:
    renderer->camera = (void*)self;
    renderer->camera_type = RENDERER_CAMERA_2D;

    // Обновляем данные матриц в шейдере по умолчанию:
    glDisable(GL_DEPTH_TEST);
    Shader *shader = renderer->shader;
    if (!shader) return;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", self->view);
    Shader_set_mat4(shader, "u_proj", self->proj);
    Shader_end(shader);
}

// Изменить размер камеры:
void Camera2D_resize(Camera2D *self, int width, int height) {
    if (!self) return;

    self->width = width;
    self->height = height;
    glViewport(0, 0, width, height);

    glm_mat4_identity(self->proj);
    float wdth = ((float)self->width)*0.5f * self->meter/100.0f;
    float hght = ((float)self->height)*0.5f * self->meter/100.0f;
    glm_ortho(-wdth, wdth, -hght, hght, -1.0f, 1.0f, self->proj);
}

// Изменить масштаб единицы измерения:
void Camera2D_set_meter(Camera2D *self, float meter) {
    if (!self) return;
    self->meter = meter;
    Camera2D_resize(self, self->width, self->height);
}

// Начало отрисовки UI:
void Camera2D_ui_begin(Camera2D *self) {
    if (!self || self->_ui_begin_ || !self->window->renderer) return;
    Renderer *renderer = self->window->renderer;
    self->_ui_begin_ = true;

    // Сохраняем текущую матрицу камеры и переключаемся на отдельный UI-view:
    glm_mat4_copy(self->view, self->old_view);
    glm_mat4_copy(self->proj, self->old_proj);
    glm_mat4_identity(self->view);
    glm_ortho(0.0f, (float)self->width, 0.0f, (float)self->height, -1.0f, 1.0f, self->proj);
    Shader *shader = renderer->shader;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", self->view);
    Shader_set_mat4(shader, "u_proj", self->proj);
    Shader_end(shader);
}

// Конец отрисовки UI:
void Camera2D_ui_end(Camera2D *self) {
    if (!self || !self->_ui_begin_ || !self->window->renderer) return;
    Renderer *renderer = self->window->renderer;
    self->_ui_begin_ = false;

    // Возвращаем обратно матрицу вида в шейдере по умолчанию:
    glm_mat4_copy(self->old_view, self->view);
    glm_mat4_copy(self->old_proj, self->proj);
    Shader *shader = renderer->shader;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", self->view);
    Shader_set_mat4(shader, "u_proj", self->proj);
    Shader_end(shader);
}


// -------- API 3D камеры: --------


// Создать 3D камеру:
Camera3D* Camera3D_create(
    Window *window, int width, int height, Vec3d position, Vec3d rotation,
    Vec3d size, float fov, float z_near, float z_far, bool ortho
) {
    Camera3D *camera = (Camera3D*)mm_alloc(sizeof(Camera3D));

    // Заполняем поля:
    camera->window = window;
    camera->position = position;
    camera->size = size;
    camera->fov = fov;
    camera->z_far = z_far;
    camera->z_near = z_near;
    camera->is_ortho = ortho;
    camera->width = width;
    camera->height = height;

    camera->_oldfov_ = fov;
    camera->_oldfar_ = z_far;
    camera->_oldnear_ = z_near;

    // Матрица вида:
    glm_mat4_identity(camera->view);
    glm_quat_identity(camera->quaternion);
    Camera3D_set_euler(camera, rotation);

    // Матрица проекции:
    glm_mat4_identity(camera->proj);

    // Установка проекции:
    Camera3D_resize(camera, width, height, ortho);

    // Установка настроек отображения геометрии:
    Camera3D_set_depth_test(camera, true);  // Включаем тест глубины.
    Camera3D_set_depth_mask(camera, true);  // Включаем запись в буфер глубины.
    Camera3D_set_blending(camera, true);    // Включаем смешивание.
    Camera3D_set_cull_faces(camera, true);  // Включаем отсечение граней.

    Camera3D_set_back_face_culling(camera);
    Camera3D_set_front_face_onleft(camera);

    // Масштабирование и перемещение:
    Camera3D_update(camera);
    return camera;
}

// Уничтожить 3D камеру:
void Camera3D_destroy(Camera3D **camera) {
    if (!camera || !*camera) return;
    mm_free(*camera);
    *camera = NULL;
}

// Обновление камеры:
void Camera3D_update(Camera3D *self) {
    if (!self || !self->window->renderer) return;
    Renderer *renderer = self->window->renderer;

    // Ограничиваем диапазон значений:
    self->fov = glm_clamp(self->fov, 0.0f, 180.0f);  // Устанавливаем границы угла обзора (от 0 до 180 градусов).
    self->z_far = glm_max(self->z_far, 0.00002f);    // Минимальное расстояние дальнего отсечения.
    self->z_near = glm_max(self->z_near, 0.00001f);  // Минимальное расстояние ближнего отсечения.

    // Обновляем матрицу проекции в случае изменения параметров камеры:
    if (!cmp_float(self->fov, self->_oldfov_) ||
        !cmp_float(self->z_far, self->_oldfar_) ||
        !cmp_float(self->z_near, self->_oldnear_))
    {
        Camera3D_resize(self, self->width, self->height, self->is_ortho);
        self->_oldfov_ = self->fov;
        self->_oldfar_ = self->z_far;
        self->_oldnear_ = self->z_near;
    }

    // Сбрасываем матрицу вида:
    glm_mat4_identity(self->view);

    // Инвертируем ориентацию камеры:
    versor inv;
    glm_quat_conjugate(self->quaternion, inv);

    // Превращаем в матрицу вращения:
    mat4 rot;
    glm_quat_mat4(inv, rot);

    // Применяем rotation:
    glm_mat4_mul(rot, self->view, self->view);

    // Масштаб для ortho:
    if (self->is_ortho) {
        glm_scale(self->view, (vec3){
            1.0f/self->size.x,
            1.0f/self->size.y,
            1.0f/self->size.z
        });
    }

    // Перенос (обратный позиции камеры):
    glm_translate(self->view, (vec3){
        -self->position.x,
        -self->position.y,
        -self->position.z
    });

    // Высчитываем матрицу взгляда:
    glm_mat4_mul(self->proj, self->view, self->view_proj);

    // Устанавливаем активную камеру:
    renderer->camera = (void*)self;
    renderer->camera_type = RENDERER_CAMERA_3D;

    // Обновляем данные матриц в шейдере по умолчанию:
    Shader *shader = renderer->shader;
    if (!shader) return;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", self->view);
    Shader_set_mat4(shader, "u_proj", self->proj);
    Shader_end(shader);
}

// Изменить размер камеры:
void Camera3D_resize(Camera3D *self, int width, int height, bool ortho) {
    if (!self) return;

    self->width = width;
    self->height = height;
    glViewport(0, 0, width, height);
    glm_mat4_identity(self->proj);
    float aspect = (float)self->width / (float)self->height;

    if (ortho) {
        float l = -(self->size.x * 0.5f + 4.0f);
        float r =  (self->size.x * 0.5f + 4.0f);
        float b = -(self->size.y * 0.5f + 4.0f);
        float t =  (self->size.y * 0.5f + 4.0f);
        glm_ortho(l*aspect, r*aspect, b, t, self->z_near, self->z_far, self->proj);
    } else {
        glm_perspective(radians(self->fov), aspect, self->z_near, self->z_far, self->proj);
    }
}

// Установить поворот камеры:
void Camera3D_set_euler(Camera3D *self, Vec3d rotation) {
    if (!self) return;

    versor qx, qy, qz, tmp;
    glm_quatv(qx, radians(rotation.x), (vec3){1, 0, 0});
    glm_quatv(qy, radians(rotation.y), (vec3){0, 1, 0});
    glm_quatv(qz, radians(rotation.z), (vec3){0, 0, 1});
    glm_quat_mul(qy, qx, tmp);
    glm_quat_mul(tmp, qz, self->quaternion);
    glm_quat_normalize(self->quaternion);
}

// Посмотреть на указанную точку:
void Camera3D_look_at(Camera3D *self, Vec3d target, Vec3d up_dir) {
    if (!self) return;

    // Вектор направления камеры:
    vec3 dir = {
        target.x - self->position.x,
        target.y - self->position.y,
        target.z - self->position.z
    };

    if (glm_vec3_norm(dir) < 1e-6f) return;  // Проверка на нулевой вектор.
    glm_vec3_normalize(dir);  // Нормализуем.

    vec3 up = {up_dir.x, up_dir.y, up_dir.z};
    glm_vec3_normalize(up);
    if (glm_vec3_norm(up) < 1e-6f || fabs(glm_vec3_dot(dir, up)) > 0.999999f) {
        up[0] = 0; up[1] = 1; up[2] = 0;
    }

    // Смотрим в указанную точку:
    glm_quat_for(dir, up, self->quaternion);
}

// Повернуть камеру по оси X:
void Camera3D_rotate_pitch(Camera3D *self, float angle) {
    if (!self) return;

    versor q;
    glm_quatv(q, radians(angle), (vec3){1, 0, 0});
    glm_quat_mul(self->quaternion, q, self->quaternion);
    glm_quat_normalize(self->quaternion);
}

// Повернуть камеру по оси Y:
void Camera3D_rotate_yaw(Camera3D *self, float angle) {
    if (!self) return;

    versor q;
    glm_quatv(q, radians(angle), (vec3){0, 1, 0});
    glm_quat_mul(q, self->quaternion, self->quaternion);
    glm_quat_normalize(self->quaternion);
}

// Повернуть камеру по оси Z:
void Camera3D_rotate_roll(Camera3D *self, float angle) {
    if (!self) return;

    versor q;
    glm_quatv(q, radians(angle), (vec3){0, 0, 1});
    glm_quat_mul(self->quaternion, q, self->quaternion);
    glm_quat_normalize(self->quaternion);
}

// Получить поворот камеры (только для UI/debug!):
Vec3d Camera3D_get_euler(Camera3D *self) {
    if (!self) return (Vec3d){0.0f, 0.0f, 0.0f};

    mat4 m;
    glm_quat_mat4(self->quaternion, m);
    float pitch = degrees(asinf(-m[2][1]));
    float yaw   = degrees(atan2f(m[2][0], m[2][2]));
    float roll  = degrees(atan2f(m[0][1], m[1][1]));
    return (Vec3d){pitch, yaw, roll};
}

// Получить вектор вперед:
Vec3d Camera3D_get_forward(Camera3D *self) {
    if (!self) return (Vec3d){0.0f, 0.0f, -1.0f};

    vec3 f = {0, 0, -1};
    glm_quat_rotatev(self->quaternion, f, f);
    return (Vec3d){f[0], f[1], f[2]};
}

// Получить вектор вправо:
Vec3d Camera3D_get_right(Camera3D *self) {
    if (!self) return (Vec3d){1.0f, 0.0f, 0.0f};

    vec3 r = {1, 0, 0};
    glm_quat_rotatev(self->quaternion, r, r);
    return (Vec3d){r[0], r[1], r[2]};
}

// Получить вектор вверх:
Vec3d Camera3D_get_up(Camera3D *self) {
    if (!self) return (Vec3d){0.0f, 1.0f, 0.0f};

    vec3 u = {0, 1, 0};
    glm_quat_rotatev(self->quaternion, u, u);
    return (Vec3d){u[0], u[1], u[2]};
}

// Установить проверку глубины:
void Camera3D_set_depth_test(Camera3D *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

// Включить или отключить запись глубины:
void Camera3D_set_depth_mask(Camera3D *self, bool enabled) {
    if (!self) return;
    enabled ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
}

// Включить или отключить смешивание:
void Camera3D_set_blending(Camera3D *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

// Установить отсечение граней:
void Camera3D_set_cull_faces(Camera3D *self, bool enabled) {
    if (!self) return;
    enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
}

// Отсекать только задние грани:
void Camera3D_set_back_face_culling(Camera3D *self) {
    if (!self) return;
    glCullFace(GL_BACK);
}

// Отсекать только передние грани:
void Camera3D_set_front_face_culling(Camera3D *self) {
    if (!self) return;
    glCullFace(GL_FRONT);
}

// Передняя грань против часовой стрелки (CCW):
void Camera3D_set_front_face_onleft(Camera3D *self) {
    if (!self) return;
    glFrontFace(GL_CCW);  // Против часовой стрелки.
}

// Передняя грань по часовой стрелке (CW):
void Camera3D_set_front_face_onright(Camera3D *self) {
    if (!self) return;
    glFrontFace(GL_CW);  // По часовой стрелке.
}

// Установить ортографическую проекцию:
void Camera3D_set_ortho(Camera3D *self, bool enabled) {
    if (!self) return;
    Camera3D_resize(self, self->width, self->height, enabled);
    self->is_ortho = enabled;
}

// Узнать включена ли ортографическая проекция:
bool Camera3D_get_ortho(Camera3D *self) {
    if (!self) return false;
    return self->is_ortho;
}
