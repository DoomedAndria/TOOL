#include "file.h"

#include <stdio.h>
#include <stdlib.h>

char* FILE_read(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) return NULL;
    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    rewind(file);
    char* buffer = malloc(size+1);
    buffer[size] = '\0';
    fread(buffer, 1, size, file);
    fclose(file);
    return buffer;
}
