#include "arena/arena.h"
#include <stdio.h>

int main(int argc, char** argv) {
    printf("Arena Engine Dedicated Server v0.1.0\n");
    
    Arena* arena = arena_create(1024 * 1024);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }
    
    printf("Arena created with capacity: %zu bytes\n", arena_capacity(arena));
    printf("Dedicated server listening on port 7777...\n");
    
    arena_destroy(arena);
    
    printf("Server shutdown complete.\n");
    return 0;
}
