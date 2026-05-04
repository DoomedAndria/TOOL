#ifndef ARENA_H
#define ARENA_H
#include <stddef.h>

typedef struct Arena Arena;

Arena* ARENA_create(size_t size);

void* ARENA_alloc(Arena* arena, size_t size);

void ARENA_reset(Arena* arena);

void ARENA_free(Arena* arena);

#endif
