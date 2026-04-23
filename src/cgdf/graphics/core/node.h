//
// node.h - Граф сцены. Определяет функционал системы дерева из узлов. Работает на основе ленивого дерева.
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>


// Определения:
#define NODE_DEFAULT_CHILDREN_COUNT 32  // Количество дочерних узлов по умолчанию.


// Объявление структур:
typedef struct Node Node;  // Структура нода.


// Структура нода:
struct Node {
    Node   *parent;           // Родительский узел.
    Array  *children;         // Дочерние узлы.
    Vec3d  position;          // Позиция узла.
    versor quaternion;        // Поворот узла.
    Vec3d  scale;             // Масштаб узла.
    mat4   transform;         // Матрица локальной трансформации узла.
    mat4   result_transform;  // Итоговая матрица с учетом родительской трансформации.
    bool   changed;           // Флаг необходимости пересчета матрицы трансформации.
    bool   parent_changed;    // Флаг указывающий на изменения в родительском узле.
};


// -------- API нода: --------


// Создать нод:
Node* Node_create(Node *parent);

// Уничтожить нод:
void Node_destroy(Node **node);

// Получить список дочерних нод:
Array* Node_get_children(Node *self);

// Установить позицию:
void Node_set_position(Node *self, Vec3d position);

// Установить поворот:
void Node_set_quaternion(Node *self, versor quaternion);

// Установить масштаб:
void Node_set_scale(Node *self, Vec3d scale);

// Вращать узел:
void Node_rotate(Node *self, Vec3d axis, float angle);

// Получить матрицу трансформации:
void Node_get_transform(Node *self, mat4 dest);

// Получить позицию в мире:
Vec3d Node_get_world_position(Node *self);

// Получить поворот в мире:
void Node_get_world_quaternion(Node *self, versor dest);

// Получить масштаб в мире:
Vec3d Node_get_world_scale(Node *self);

// Копировать нод c потомками в переданный родитель:
Node* Node_copy(Node *self, Node *parent);

// Удалить дочерний узел (из списка потомков):
void Node_remove_child(Node *self, Node *child);

// Установить родителя:
void Node_set_parent(Node *self, Node *parent);

// Получить родителя:
Node* Node_get_parent(Node *self);

// Пересчитать матрицу трансформации:
void Node_recalculate_matrix(Node *self);

// Проход потомков в глубину с изменением флага parent_changed:
void Node_invalidate_parent(Node *self);

// Получить количество узлов в узле:
size_t Node_count_nodes(Node *self);

// Количество узлов во всем дереве:
size_t Node_count_all_nodes(Node *self);
