#ifndef LIST_H
#define LIST_H

#include "../types.h"

typedef struct List List;

List* LIST_create(CompareFunc cmp, FreeFunc free_func, PrintFunc print_func);

List* LIST_create_string();

List* LIST_create_int();

int LIST_size(const List* list);

void LIST_append(List* list, void* data);

void LIST_prepend(List* list, void* data);

int LIST_pop(List* list);

int LIST_shift(List* list);

void LIST_print(const List* list);

void LIST_free(List* list);

typedef struct {
    void* data;
    int status;
} ListValue;

ListValue LIST_get(const List* list, int index);

void LIST_set(const List* list, int index, void* value);

int LIST_find(const List* list, const void* value);

int LIST_remove(List* list, int index);

int LIST_contains(const List* list, const void* value);

int LIST_insert(List* list, int index, void* data);

void LIST_reverse(List* list);

void LIST_sort(List* list);

void LIST_clear(List* list);

typedef struct ListIter ListIter;

ListIter* LIST_iter(const List* list);

int LIST_iter_has_next(const ListIter* iter);

void* LIST_iter_next(ListIter* iter);

void LIST_iter_free(ListIter* iter);


#endif
