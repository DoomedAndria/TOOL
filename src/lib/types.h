#ifndef TYPES_H
#define TYPES_H

int* INT(int a);

char* STRING(const char* string);

typedef int (*CompareFunc)(const void* a, const void* b);

typedef void (*FreeFunc)(void* data);

typedef void (*PrintFunc)(const void* data);

#endif
