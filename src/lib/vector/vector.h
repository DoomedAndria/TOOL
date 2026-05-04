#ifndef VECTOR_H
#define VECTOR_H
#include "../types.h"

typedef struct Vector Vector;

Vector* VEC_create(CompareFunc compare, FreeFunc free_func, PrintFunc print);

void VEC_push(Vector* vec, void* data);

void* VEC_pop(Vector* vec);

void* VEC_get(const Vector* vec, int index);

int VEC_set(Vector* vec, int index, void* data);

int VEC_size(const Vector* vec);

void VEC_free(Vector* vec);

int VEC_insert(Vector* vec, int index, void* data);

int VEC_remove(Vector* vec, int index);
#endif
