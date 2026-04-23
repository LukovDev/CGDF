//
// node.c - Граф сцены. Реализует функционал системы дерева из узлов. Работает на основе ленивого дерева.
//


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>
#include <cgdf/core/mm.h>
#include <cgdf/core/logger.h>
#include "node.h"


// Создать нод:
Node* Node_create(Node *parent) {
    Node *node = (Node*)mm_alloc(sizeof(Node));

    // Если передали родителя, записываем себя в потомки:
    if (parent) {
        Array_push(parent->children, &node);
    }

    // Заполняем поля:
    node->parent = parent;
    node->children = Array_create(sizeof(Node*), NODE_DEFAULT_CHILDREN_COUNT);
    node->position = (Vec3d){0.0, 0.0, 0.0};
    glm_quat_identity(node->quaternion);
    node->scale = (Vec3d){1.0, 1.0, 1.0};
    glm_mat4_identity(node->transform);
    glm_mat4_identity(node->result_transform);
    node->changed = false;
    node->parent_changed = parent ? true : false;  // Если есть родитель, то пересчитать итоговую матрицу.
    return node;
}

// Уничтожить нод:
void Node_destroy(Node **node) {
    if (!node || !*node) return;

    // Удаляем себя из потомков родителя:
    if ((*node)->parent) {
        Node_remove_child((*node)->parent, *node);
    }

    // Рекурсивно удаляем потомков:
    if ((*node)->children) {
        for (int i = 0; i < Array_len((*node)->children); i++) {
            Node *child = (Node*)Array_get_ptr((*node)->children, i);
            child->parent = NULL;  // Чтобы потомок не пытался удалить себя из нас.
            Node_destroy(&child);
        }
        Array_destroy(&(*node)->children);
    }

    mm_free(*node);
    *node = NULL;
}

// Получить список дочерних нод:
Array* Node_get_children(Node *self) {
    if (!self) return NULL;
    return self->children;
}

// Возвращает необходимость пересчета матрицы трансформации:
bool Node_is_changed(Node *self) {
    if (!self) return false;
    return self->changed;
}

// Установить позицию:
void Node_set_position(Node *self, Vec3d position) {
    if (!self) return;
    self->position = position;
    self->changed = true;          // Флаг о изменении нода.
    Node_invalidate_parent(self);  // Проход потомков в глубину и установка флага parent_changed.
}

// Установить поворот:
void Node_set_quaternion(Node *self, versor quaternion) {
    if (!self) return;
    glm_quat_copy(quaternion, self->quaternion);
    self->changed = true;          // Флаг о изменении нода.
    Node_invalidate_parent(self);  // Проход потомков в глубину и установка флага parent_changed.
}

// Установить масштаб:
void Node_set_scale(Node *self, Vec3d scale) {
    if (!self) return;
    self->scale = scale;
    self->changed = true;          // Флаг о изменении нода.
    Node_invalidate_parent(self);  // Проход потомков в глубину и установка флага parent_changed.
}

// Вращать узел:
void Node_rotate(Node *self, Vec3d axis, float angle) {
    if (!self) return;
    versor step;
    glm_quat(step, radians(angle), axis.x, axis.y, axis.z);
    glm_quat_mul(self->quaternion, step, self->quaternion);
    self->changed = true;          // Флаг о изменении нода.
    Node_invalidate_parent(self);  // Проход потомков в глубину и установка флага parent_changed.
}

// Получить матрицу трансформации:
void Node_get_transform(Node *self, mat4 dest) {
    if (!self) return;
    Node_recalculate_matrix(self);  // Если требуется, пересчитаем матрицу.
    glm_mat4_copy(self->result_transform, dest);
}

// Получить позицию в мире:
Vec3d Node_get_world_position(Node *self) {
    if (!self) return (Vec3d){0.0, 0.0, 0.0};
    Node_recalculate_matrix(self);  // Гарантируем актуальность.
    // В cglm матрица 4x4, позиция лежит в элементах [3][0], [3][1], [3][2]:
    Vec3d pos;
    pos.x = self->result_transform[3][0];
    pos.y = self->result_transform[3][1];
    pos.z = self->result_transform[3][2];
    return pos;
}

// Получить поворот в мире:
void Node_get_world_quaternion(Node *self, versor dest) {
    if (!self) return;
    Node_recalculate_matrix(self);  // Гарантируем актуальность.
    mat4 pure_rot;
    glm_mat4_copy(self->result_transform, pure_rot);
    // Убираем масштаб из матрицы, чтобы осталось только вращение:
    glm_vec3_normalize(pure_rot[0]);
    glm_vec3_normalize(pure_rot[1]);
    glm_vec3_normalize(pure_rot[2]);
    // Конвертируем очищенную матрицу в кватернион:
    glm_mat4_quat(pure_rot, dest);
}

// Получить масштаб в мире:
Vec3d Node_get_world_scale(Node *self) {
    if (!self) return (Vec3d){0.0, 0.0, 0.0};
    Node_recalculate_matrix(self);  // Гарантируем актуальность.
    // Извлекаем длину каждого вектора оси (X, Y, Z):
    Vec3d scale;
    scale.x = glm_vec3_norm(self->result_transform[0]);
    scale.y = glm_vec3_norm(self->result_transform[1]);
    scale.z = glm_vec3_norm(self->result_transform[2]);
    return scale;
}

// Копировать нод в то же место (в родителе оригинала):
Node* Node_copy(Node *self) {
    if (!self) return NULL;
    Node *node = (Node*)mm_alloc(sizeof(Node));

    // Заполняем поля:
    node->parent = self->parent;
    node->children = Array_create(sizeof(Node*), Array_capacity(self->children));
    node->position = self->position;
    glm_quat_copy(self->quaternion, node->quaternion);
    node->scale = self->scale;
    node->changed = true;
    node->parent_changed = true;

    // Если есть родитель, записываем себя в потомки:
    if (self->parent) {
        Array_push(self->parent->children, &node);
    }

    // Если у оригинала не было изменений, то копируем матрицу трансформации:
    if (!self->changed) {
        glm_mat4_copy(self->transform, node->transform);
    } else {
        glm_mat4_identity(node->transform);
    }

    // Если у родителя не было изменений для оригинала, то копируем итоговую матрицу трансформации:
    if (!self->parent_changed) {
        glm_mat4_copy(self->result_transform, node->result_transform);
    } else {
        glm_mat4_identity(node->result_transform);
    }

    // Копируем потомков:
    for (int i = 0; i < Array_len(self->children); i++) {
        Node *child_original = (Node*)Array_get_ptr(self->children, i);
        Node *child_copy = Node_copy(child_original);  // Рекурсия.
        Node_set_parent(child_copy, node);  // Привязываем копию ребенка к копии родителя.
    }

    return node;
}

// Удалить дочерний узел (из списка потомков):
void Node_remove_child(Node *self, Node *child) {
    if (!self || !child) return;

    // Ищем, под каким индексом лежит ребенок:
    int index = -1;
    for (int i = 0; i < Array_len(self->children); i++) {
        if ((Node*)Array_get_ptr(self->children, i) == child) {
            index = i; break;
        }
    }

    // Если нашли, удаляем его:
    if (index != -1) {
        child->parent = NULL;          // Удаляем родителя у потомка.
        child->parent_changed = true;  // Теперь он сам по себе, матрицы надо пересчитать.
        Array_remove(self->children, index, NULL);
    }
}

// Установить родителя:
void Node_set_parent(Node *self, Node *parent) {
    if (!self || self->parent == parent) return;

    // Проверка на зацикливание (чтобы не стать родителем самому себе или предкам):
    Node *tmp = parent;
    while (tmp) {
        if (tmp == self) {  // Ошибка! Попытка создать петлю в иерархии:
            log_msg("[E] Node_set_parent: Cycle detected!\n");
            return;
        }
        tmp = tmp->parent;
    }

    // Если у узла уже был родитель, удаляемся из его списка детей:
    if (self->parent) {
        Node_remove_child(self->parent, self);
    }

    // Устанавливаем нового родителя:
    self->parent = parent;

    // Если новый родитель существует, добавляемся к нему в список:
    if (parent) {
        Array_push(parent->children, &self);
    }

    // Инвалидация (сброс) матриц для всей ветки вниз:
    self->parent_changed = true;
    Node_invalidate_parent(self);
}

// Получить родителя:
Node* Node_get_parent(Node *self) {
    if (!self) return NULL;
    return self->parent;
}

// Пересчитать матрицу трансформации:
void Node_recalculate_matrix(Node *self) {
    if (!self) return;

    // Если было изменение по векторам позиции, поворота и масштаба, то пересчитываем матрицу трансформации:
    if (self->changed) {
        glm_mat4_identity(self->transform);  // Сбрасываем трансформацию.
        glm_translate(self->transform, (vec3){self->position.x, self->position.y, self->position.z});
        glm_quat_rotate(self->transform, self->quaternion, self->transform);
        glm_scale(self->transform, (vec3){self->scale.x, self->scale.y, self->scale.z});
    }

    // Если собственная или родительская матрицы менялись, необходимо пересчитать итоговую матрицу:
    if (self->changed || self->parent_changed) {
        if (self->parent) {  // Если есть родитель:
            mat4 matrix;
            Node_get_transform(self->parent, matrix);  // Матрица трансформации родителя.
            glm_mat4_mul(matrix, self->transform, self->result_transform);
        } else {  // Если нет родителя:
            glm_mat4_copy(self->transform, self->result_transform);
        }
        self->changed = self->parent_changed = false;  // Изменения применены.
    }
}

// Проход потомков в глубину с изменением флага parent_changed:
void Node_invalidate_parent(Node *self) {
    if (!self) return;

    // Цикл по потомкам:
    for (int i = 0; i < Array_len(self->children); i++) {
        Node *child = (Node*)Array_get_ptr(self->children, i);
        if (!child->parent_changed) {       // Если еще не помечен.
            child->parent_changed = true;   // Флаг о изменении родителя.
            Node_invalidate_parent(child);  // Рекурсивный вызов для потомков выбранного потомка.
        }
    }
}
