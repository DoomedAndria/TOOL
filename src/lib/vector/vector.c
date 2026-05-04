#include <stdlib.h>
#include "vector.h"
#include "../types.h"

struct Vector {
    void** data;
    int size;
    int capacity;
    CompareFunc compare;
    FreeFunc free_func;
    PrintFunc print;
};

Vector* VEC_create(const CompareFunc compare,
                   const FreeFunc free_func,
                   const PrintFunc print) {
    Vector* ret = malloc(sizeof(Vector));
    ret->compare = compare;
    ret->free_func = free_func;
    ret->print = print;

    ret->capacity = 8;
    ret->data = malloc(sizeof(void *) * ret->capacity);
    ret->size = 0;
    return ret;
}

static void VEC_resize(Vector* vec) {
    const int old_capacity = vec->capacity;
    const int new_capacity = old_capacity * 2;
    void** new_data = realloc(vec->data, sizeof(void *) * new_capacity);
    if (new_data == NULL) return;
    vec->data = new_data;
    vec->capacity = new_capacity;
}

void VEC_push(Vector* vec, void* data) {
    if (vec->size >= vec->capacity) {
        VEC_resize(vec);
    }
    vec->data[vec->size] = data;
    vec->size++;
}

void* VEC_pop(Vector* vec) {
    if (vec->size == 0) return NULL;
    void* data = vec->data[vec->size - 1];
    vec->size--;
    return data;
}

void* VEC_get(const Vector* vec, int index) {
    if (index < 0) index = vec->size + index;
    if (index < 0 || index >= vec->size || vec->size == 0) return NULL;
    void* data = vec->data[index];
    return data;
}

int VEC_set(Vector* vec, int index, void* data) {
    if (index < 0) index = vec->size + index;
    if (index < 0 || index >= vec->size || vec->size == 0) return 0;
    if (vec->free_func != NULL) vec->free_func(vec->data[index]);
    vec->data[index] = data;
    return 1;
}

int VEC_size(const Vector* vec) {
    return vec->size;
}

void VEC_free(Vector* vec) {
    for (int i = 0; i < vec->size; i++) {
        void* data = vec->data[i];
        if (vec->free_func != NULL) vec->free_func(data);
    }
    free(vec->data);
    free(vec);
}

int VEC_insert(Vector* vec, int index, void* data) {
    if (vec->size >= vec->capacity)
        VEC_resize(vec);
    if (index < 0) index = vec->size + index;
    if (index < 0 || index > vec->size || vec->size == 0) return 0;

    void* temp = vec->data[index];
    vec->data[index] = data;
    for (int i = index; i <= vec->size; i++) {
        void* temp2 = vec->data[i];
        vec->data[i] = temp;
        temp = temp2;
    }
    vec->size++;
    return 1;
}


int VEC_remove(Vector* vec, int index) {
    if (index < 0) index = vec->size + index;
    if (index < 0 || index >= vec->size || vec->size == 0) return 0;
    if (vec->free_func != NULL) vec->free_func(vec->data[index]);
    for (int i = index; i < vec->size-1; i++) {
        vec->data[i] = vec->data[i+1];
    }
    vec->size--;
    return 1;
}