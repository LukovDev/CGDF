//
// camera.c - Реализует работу камеры в OpenGL.
//


// Подключаем:
#include <cgdf/core/math.h>
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../window.h"
#include "../camera.h"
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

    // Матрица проекции:
    glm_mat4_identity(camera->proj);

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
    float wdth = ((float)self->width)/2.0f * self->meter/100.0f;
    float hght = ((float)self->height)/2.0f * self->meter/100.0f;
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

    // Обнуляем матрицу вида в шейдере по умолчанию:
    mat4 view;
    glm_mat4_identity(view);
    glm_translate(view, (vec3){-self->width/2, -self->height/2, 0});
    Shader *shader = renderer->shader;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", view);
    Shader_end(shader);
}

// Конец отрисовки UI:
void Camera2D_ui_end(Camera2D *self) {
    if (!self || !self->_ui_begin_ || !self->window->renderer) return;
    Renderer *renderer = self->window->renderer;
    self->_ui_begin_ = false;

    // Возвращаем обратно матрицу вида в шейдере по умолчанию:
    Shader *shader = renderer->shader;
    Shader_begin(shader);
    Shader_set_mat4(shader, "u_view", self->view);
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
    camera->rotation = rotation;
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

    // Обновляем матрицу вида:
    glm_mat4_identity(self->view);
    if (self->is_ortho) glm_scale(self->view, (vec3){1.0f/self->size.x, 1.0f/self->size.y, 1.0f/self->size.z});
    glm_rotate(self->view, radians(self->rotation.z), (vec3){false, false, true});
    glm_rotate(self->view, radians(self->rotation.x), (vec3){true, false, false});
    glm_rotate(self->view, radians(self->rotation.y), (vec3){false, true, false});
    glm_translate(self->view, (vec3){-self->position.x, -self->position.y, -self->position.z});

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
        float l = -(self->size.x / 2.0f + 4.0f);
        float r =  (self->size.x / 2.0f + 4.0f);
        float b = -(self->size.y / 2.0f + 4.0f);
        float t =  (self->size.y / 2.0f + 4.0f);
        glm_ortho(l*aspect, r*aspect, b, t, self->z_near, self->z_far, self->proj);
    } else {
        glm_perspective(radians(self->fov), aspect, self->z_near, self->z_far, self->proj);
    }
}

// Посмотреть на указанную точку:
void Camera3D_look_at(Camera3D *self, Vec3d target) {
    if (!self) return;
    double dx = self->position.x - target.x;
    double dy = self->position.y - target.y;
    double dz = self->position.z - target.z;
    self->rotation.x = degrees(atan2(dy, sqrt(dx*dx + dz*dz)));      // Pitch.
    self->rotation.y = normalize_deg(degrees(atan2(dz, dx))-90.0f);  // Yaw.
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
