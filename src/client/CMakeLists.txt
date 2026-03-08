add_executable(arena-client
    main.c
)

target_link_libraries(arena-client
    PRIVATE arena-renderer
    PRIVATE arena-core
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
)

target_compile_options(arena-client PRIVATE -Wall -Wextra -Wpedantic -g)

# 3D Test Scene executable
add_executable(arena-test3d
    test_3d.c
)

target_link_libraries(arena-test3d
    PRIVATE arena-renderer
    PRIVATE arena-core
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
    PRIVATE m
)

target_compile_options(arena-test3d PRIVATE -Wall -Wextra -Wpedantic -g)
