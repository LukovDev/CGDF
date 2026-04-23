//
// node.h - Нод. Позволяет организовывать объекты в сцене. Может иметь родительский нод и дочерние.
//
// Работает на ленивых
//

#pragma once


// Подключаем:
#include <cgdf/core/std.h>
#include <cgdf/core/math.h>
#include <cgdf/core/array.h>


// Определения:
#define NODE_DEFAULT_CHILDREN_COUNT 16  // Количество дочерних узлов по умолчанию.


// Объявление структур:
typedef struct Node Node;  // Структура нода.


// Структура нода:
struct Node {
    Node *parent;           // Родительский узел.
    Array *children;        // Дочерние узлы.
    Vec3d position;         // Позиция узла.
    versor quaternion;      // Поворот узла.
    Vec3d scale;            // Масштаб узла.
    mat4 transform;         // Матрица локальной трансформации узла.
    mat4 result_transform;  // Итоговая матрица с учетом родительской трансформации.
    bool changed;           // Флаг необходимости пересчета матрицы трансформации.
    bool parent_changed;    // Флаг указывающий на изменения в родительском узле.
};


// -------- API нода: --------


// Создать нод:
Node* Node_create(Node *parent);

// Уничтожить нод:
void Node_destroy(Node **node);

// Получить список дочерних нод:
Array* Node_get_children(Node *self);

// Возвращает необходимость пересчета матрицы трансформации:
bool Node_is_changed(Node *self);

// Установить позицию:
void Node_set_position(Node *self, Vec3d position);

// Установить поворот:
void Node_set_quaternion(Node *self, versor quaternion);

// Установить масштаб:
void Node_set_scale(Node *self, Vec3d scale);

// Получить матрицу трансформации:
void Node_get_transform(Node *self, mat4 dest);

// Копировать нод в том же месте (в родителе оригинала):
Node* Node_copy(Node *self);

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
