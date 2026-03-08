add_library(arena-renderer STATIC
    renderer.h
    renderer.c
)

target_link_libraries(arena-renderer 
    PUBLIC arena-core
    PRIVATE Vulkan::Vulkan
)

target_include_directories(arena-renderer PUBLIC
    ${GLFW_INCLUDE_DIRS}
)

target_compile_options(arena-renderer PRIVATE -Wall -Wextra -Wpedantic -g)
