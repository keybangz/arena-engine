#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Arena Arena;

Arena* arena_create(size_t capacity);
void arena_destroy(Arena* arena);

void* arena_alloc(Arena* arena, size_t size, size_t alignment);
void* arena_realloc(Arena* arena, void* ptr, size_t old_size, size_t new_size, size_t alignment);
void arena_free(Arena* arena, void* ptr);

void arena_reset(Arena* arena);
size_t arena_used(Arena* arena);
size_t arena_capacity(Arena* arena);
bool arena_owns(Arena* arena, void* ptr);

typedef struct {
    void* (*alloc)(size_t size, void* user_data);
    void* (*realloc)(void* ptr, size_t old_size, size_t new_size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* user_data;
} ArenaAllocatorFunctions;

Arena* arena_create_custom(ArenaAllocatorFunctions funcs, size_t capacity);

#endif
