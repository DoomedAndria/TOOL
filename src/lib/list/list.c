#include <stdlib.h>
#include <stdio.h>
#include "list.h"

typedef struct Node {
    void* data;
    struct Node* next;
    struct Node* prev;
} Node;


Node* NODE_create(void* data) {
    Node* node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    return node;
}


typedef struct List {
    Node* head;
    Node* tail;
    int size;
    CompareFunc compare;
    FreeFunc free_func;
    PrintFunc print_func;
} List;


List* LIST_create(const CompareFunc cmp, const FreeFunc free_func, PrintFunc print_func) {
    List* list = malloc(sizeof(List));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->compare = cmp;
    list->free_func = free_func;
    list->print_func = print_func;
    return list;
}

int LIST_size(const List* list) {
    return list->size;
}

void LIST_append(List* list, void* data) {
    Node* node = NODE_create(data);
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size++;
}

void LIST_prepend(List* list, void* data) {
    Node* node = NODE_create(data);
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->size++;
}

int LIST_pop(List* list) {
    if (list->size <= 0) return 0;
    Node* temp = list->tail;

    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->tail = temp->prev;
        list->tail->next = NULL;
    }
    if (list->free_func != NULL)
        list->free_func(temp->data);
    free(temp);
    list->size--;
    return 1;
}

int LIST_shift(List* list) {
    if (list->size <= 0) return 0;

    Node* temp = list->head;

    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->head = temp->next;
        list->head->prev = NULL;
    }
    if (list->free_func != NULL)
        list->free_func(temp->data);
    free(temp);
    list->size--;
    return 1;
}


void LIST_print(const List* list) {
    const Node* current = list->head;
    printf("[");
    while (current != NULL) {
        list->print_func(current->data);
        if (current->next != NULL)
            printf(", ");
        current = current->next;
    }

    printf("]\n");
}

void LIST_free(List* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        if (list->free_func != NULL)
            list->free_func(temp->data);
        free(temp);
    }
    free(list);
}

Node* LIST_get_node(const List* list, int index) {
    if (index < 0)
        index = index + list->size;
    if (index >= 0 && index < list->size) {
        Node* current;
        if (index <= list->size / 2) {
            current = list->head;
            while (index > 0) {
                current = current->next;
                index--;
            }
        } else {
            current = list->tail;
            while (index < list->size - 1) {
                current = current->prev;
                index++;
            }
        }
        return current;
    }
    return NULL;
}

ListValue LIST_get(const List* list, const int index) {
    ListValue val = {NULL, 0};
    const Node* current = LIST_get_node(list, index);
    if (current != NULL) {
        val.data = current->data;
        val.status = 1;
    }
    return val;
}

void LIST_set(const List* list, const int index, void* value) {
    Node* current = LIST_get_node(list, index);
    if (current != NULL) {
        if (list->free_func) list->free_func(current->data);
        current->data = value;
    }
}

int LIST_find(const List* list, const void* value) {
    int index = -1;
    int count = 0;
    const Node* current = list->head;
    while (current != NULL) {
        if (list->compare(current->data, value) == 0) {
            index = count;
            break;
        }
        count++;
        current = current->next;
    }
    return index;
}

int LIST_remove(List* list, int index) {
    Node* current = LIST_get_node(list, index);

    if (current == NULL)
        return 0;

    if (current == list->head)
        return LIST_shift(list);
    if (current == list->tail)
        return LIST_pop(list);

    current->prev->next = current->next;
    current->next->prev = current->prev;
    if (list->free_func != NULL)
        list->free_func(current->data);
    free(current);
    list->size--;
    return 1;
}


int LIST_contains(const List* list, const void* value) {
    const int idx = LIST_find(list, value);
    if (idx >= 0) return 1;
    return 0;
}


int LIST_insert(List* list, const int index, void* data) {
    if (index == list->size) {
        LIST_append(list, data);
        return 1;
    }
    Node* current = LIST_get_node(list, index);
    if (current == NULL) return 0;
    if (current == list->head) {
        LIST_prepend(list, data);
        return 1;
    }
    Node* new_node = NODE_create(data);
    Node* left = current->prev;
    left->next = new_node;
    new_node->next = current;
    current->prev = new_node;
    new_node->prev = left;
    list->size++;
    return 1;
}

void LIST_reverse(List* list) {
    Node* current = list->head;
    while (current != NULL) {
        Node* next = current->next;
        current->next = current->prev;
        current->prev = next;
        current = next;
    }

    Node* temp = list->head;
    list->head = list->tail;
    list->tail = temp;
}

void LIST_clear(List* list) {
    Node* current = list->head;
    list->head = NULL;
    list->tail = NULL;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        if (list->free_func != NULL)
            list->free_func(temp->data);
        free(temp);
    }
    list->size = 0;
}

Node* LIST_split(Node* node) {
    if (node == NULL || node->next == NULL)
        return NULL;
    Node* slow = node;
    const Node* fast = node;
    while (fast != NULL && fast->next != NULL) {
        fast = fast->next->next;
        slow = slow->next;
    }
    Node* right = slow;
    if (slow->prev != NULL)
        slow->prev->next = NULL;

    right->prev = NULL;
    return right;
}

typedef struct {
    Node* head;
    Node* tail;
} NodePair;

static NodePair LIST_merge(Node* a, Node* b, CompareFunc compare) {
    NodePair result = {NULL, NULL};

    while (a && b) {
        Node* temp;

        if (compare(a->data, b->data) < 0) {
            temp = a;
            a = a->next;
        } else {
            temp = b;
            b = b->next;
        }

        temp->next = NULL;
        temp->prev = result.tail;

        if (result.tail)
            result.tail->next = temp;
        else
            result.head = temp;

        result.tail = temp;
    }

    Node* rest = a != NULL ? a : b;

    while (rest) {
        Node* temp = rest;
        rest = rest->next;

        temp->next = NULL;
        temp->prev = result.tail;

        if (result.tail)
            result.tail->next = temp;
        else
            result.head = temp;

        result.tail = temp;
    }

    return result;
}


static NodePair LIST_sort_helper(Node* node, const CompareFunc compare) {
    if (node == NULL)
        return (NodePair){NULL, NULL};
    if (node->next == NULL)
        return (NodePair){node, node};
    Node* right = LIST_split(node);
    Node* left_sorted = LIST_sort_helper(node, compare).head;
    Node* right_sorted = LIST_sort_helper(right, compare).head;
    return LIST_merge(left_sorted, right_sorted, compare);
}

void LIST_sort(List* list) {
    Node* node = list->head;
    const NodePair pair = LIST_sort_helper(node, list->compare);
    list->head = pair.head;
    list->tail = pair.tail;
}


struct ListIter {
    Node* current;
};

ListIter* LIST_iter(const List* list) {
    ListIter* iter = malloc(sizeof(ListIter));
    iter->current = list->head;
    return iter;
}

int LIST_iter_has_next(const ListIter* iter) {
    return iter->current != NULL;
}

void* LIST_iter_next(ListIter* iter) {
    void* data = iter->current->data;
    iter->current = iter->current->next;
    return data;
}

void LIST_iter_free(ListIter* iter) {
    free(iter);
}
