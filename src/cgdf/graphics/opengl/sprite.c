//
// sprite.c - Реализация работы спрайта для OpenGL.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/mm.h>
#include "../renderer.h"
#include "../texture.h"
#include "../shader.h"
#include "../mesh.h"
#include "../sprite.h"


// Объявление функций:
static void Sprite2D_Impl_render(Sprite2D *self);


// -------- API спрайта: --------


// Создать спрайт:
Sprite2D* Sprite2D_create(
    Renderer *renderer, Texture *texture,
    float x, float y, float width, float height,
    float angle, Vec4f color, bool custom_shader
) {
    if (!renderer) return NULL;
    Sprite2D *sprite = (Sprite2D*)mm_alloc(sizeof(Sprite2D));

    // Заполняем поля:
    sprite->renderer = renderer;
    sprite->texture = texture;
    sprite->x = x;
    sprite->y = y;
    sprite->width = width;
    sprite->height = height;
    sprite->angle = angle;
    sprite->color = color;
    sprite->custom_shader = custom_shader;
    sprite->render = Sprite2D_Impl_render;
    return sprite;
}

// Уничтожить спрайт:
void Sprite2D_destroy(Sprite2D **sprite) {
    if (!sprite || !*sprite) return;
    mm_free(*sprite);
    *sprite = NULL;
}

// Отрисовать 2D спрайт (без создания экземпляра):
void Sprite2D_render(
    Renderer *renderer, Texture *texture,
    float x, float y, float width, float height,
    float angle, Vec4f color, bool custom_shader
) {
    // Настраиваем матрицу модели:
    mat4 model;
    glm_mat4_identity(model);
    glm_translate(model, (vec3){x+width/2.0f, y+height/2.0f, 0.0f});
    glm_scale(model, (vec3){width/2.0f, height/2.0f, 0.0f});
    if (angle != 0.0f) glm_rotate(model, radians(angle), (vec3){0.0f, 0.0f, 1.0f});

    // Настраиваем дефолтный шейдер:
    if (!custom_shader) {
        Shader_begin(renderer->shader);

        // Настраиваем текстуру в шейдере:
        if (texture) {
            Shader_set_bool(renderer->shader, "u_use_texture", true);
            Shader_set_tex2d(renderer->shader, "u_texture", texture->id);
        } else { Shader_set_bool(renderer->shader, "u_use_texture", false); }

        // Настраиваем цвет и матрицу модели в шейдере:
        Shader_set_vec4(renderer->shader, "u_color", color);
        Shader_set_mat4(renderer->shader, "u_model", model);

        // Рисуем спрайт:
        Mesh_render(renderer->sprite_mesh, false);
        Shader_end(renderer->shader);
    } else {
        // Рисуем спрайт:
        Mesh_render(renderer->sprite_mesh, false);
    }
}

// Функция внутри структуры спрайта:
static void Sprite2D_Impl_render(Sprite2D *self) {
    if (!self) return;
    Sprite2D_render(
        self->renderer, self->texture,
        self->x, self->y, self->width, self->height,
        self->angle, self->color, self->custom_shader
    );
}
