#include "arena/arena.h"
#include "renderer/renderer.h"
#include <stdio.h>

int main(int argc, char** argv) {
    printf("Arena Engine Client v0.1.0\n");
    
    Arena* arena = arena_create(1024 * 1024);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }
    
    printf("Arena created with capacity: %zu bytes\n", arena_capacity(arena));
    
    arena_destroy(arena);
    
    printf("Client shutdown complete.\n");
    return 0;
}
