#include <stdlib.h>
#include <string.h>

int* INT(const int a) {
    int* p = malloc(sizeof(int));
    *p = a;
    return p;
}

char* STRING(const char* string) {
    return strdup(string);
}


