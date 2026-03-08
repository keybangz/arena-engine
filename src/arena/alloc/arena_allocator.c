#include "arena_allocator.h"
#include <stdlib.h>
#include <string.h>

struct Arena {
    void* memory;
    size_t capacity;
    size_t offset;
    size_t used;
    ArenaAllocatorFunctions funcs;
    bool is_custom;
};

static void* default_alloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

static void* default_realloc(void* ptr, size_t old_size, size_t new_size, void* user_data) {
    (void)user_data;
    (void)old_size;
    return realloc(ptr, new_size);
}

static void default_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

Arena* arena_create_custom(ArenaAllocatorFunctions funcs, size_t capacity) {
    if (!funcs.alloc) return NULL;
    
    Arena* arena = (Arena*)funcs.alloc(sizeof(Arena), funcs.user_data);
    if (!arena) return NULL;
    
    void* memory = NULL;
    if (capacity > 0) {
        memory = funcs.alloc(capacity, funcs.user_data);
        if (!memory) {
            funcs.free(arena, funcs.user_data);
            return NULL;
        }
    }
    
    arena->memory = memory;
    arena->capacity = capacity;
    arena->offset = 0;
    arena->used = 0;
    arena->funcs = funcs;
    arena->is_custom = true;
    
    return arena;
}

Arena* arena_create(size_t capacity) {
    ArenaAllocatorFunctions funcs = {
        .alloc = default_alloc,
        .realloc = default_realloc,
        .free = default_free,
        .user_data = NULL
    };
    return arena_create_custom(funcs, capacity);
}

void arena_destroy(Arena* arena) {
    if (!arena) return;
    if (arena->memory) {
        arena->funcs.free(arena->memory, arena->funcs.user_data);
    }
    arena->funcs.free(arena, arena->funcs.user_data);
}

void* arena_alloc(Arena* arena, size_t size, size_t alignment) {
    if (!arena || size == 0) return NULL;
    
    size_t aligned_offset = (arena->offset + alignment - 1) & ~(alignment - 1);
    size_t new_offset = aligned_offset + size;
    
    if (new_offset > arena->capacity) {
        return NULL;
    }
    
    void* ptr = (char*)arena->memory + aligned_offset;
    arena->offset = new_offset;
    arena->used += size;
    
    return ptr;
}

void* arena_realloc(Arena* arena, void* ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (!arena || !ptr) return NULL;
    
    void* new_ptr = arena_alloc(arena, new_size, alignment);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    
    return new_ptr;
}

void arena_free(Arena* arena, void* ptr) {
    (void)arena;
    (void)ptr;
}

void arena_reset(Arena* arena) {
    if (!arena) return;
    arena->offset = 0;
    arena->used = 0;
}

size_t arena_used(Arena* arena) {
    return arena ? arena->offset : 0;
}

size_t arena_capacity(Arena* arena) {
    return arena ? arena->capacity : 0;
}

bool arena_owns(Arena* arena, void* ptr) {
    if (!arena || !ptr || !arena->memory) return false;
    char* p = (char*)ptr;
    char* start = (char*)arena->memory;
    char* end = start + arena->offset;
    return p >= start && p < end;
}
