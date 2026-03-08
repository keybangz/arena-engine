add_library(arena-core STATIC
    arena.h
    alloc/arena_allocator.c
    alloc/arena_allocator.h
    collections/arena.c
    collections/arena.h
    collections/array.c
    collections/array.h
    collections/hashmap.c
    collections/hashmap.h
    math/vec3.c
    math/vec3.h
    math/mat4.c
    math/mat4.h
    math/quat.c
    math/quat.h
    math/math.h
    ecs/ecs.c
    ecs/ecs.h
    input/input.c
    input/input.h
    world/world.h
    network/network.h
    server/server.h
)

target_compile_options(arena-core PRIVATE -Wall -Wextra -Wpedantic -g)
target_link_libraries(arena-core PRIVATE m)  # Math library for sqrt, etc.
