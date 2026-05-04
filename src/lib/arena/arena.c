#include <stdlib.h>
#include "arena.h"


struct Arena {
    void* buffer;
    size_t size;
    size_t offset;
};

Arena* ARENA_create(const size_t size) {
    Arena* arena = malloc(sizeof(Arena));
    void* buffer = malloc(size);
    arena->buffer = buffer;
    arena->size = size;
    arena->offset = 0;
    return arena;
}

void* ARENA_alloc(Arena* arena, const size_t size) {
    if (size <= arena->size - arena->offset) {
        void* ptr = (char*)arena->buffer + arena->offset;
        arena->offset += size;
        return ptr;
    }
    return NULL;
}

void ARENA_reset(Arena* arena) {
    arena->offset = 0;
}
void ARENA_free(Arena* arena) {
    free(arena->buffer);
    free(arena);
}
