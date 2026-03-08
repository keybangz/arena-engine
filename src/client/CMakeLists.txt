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
