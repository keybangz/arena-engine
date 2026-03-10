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

# Animation System Unit Tests
add_executable(arena-test-animation
    test_animation.c
)

target_link_libraries(arena-test-animation
    PRIVATE arena-core
    PRIVATE m
)

target_compile_options(arena-test-animation PRIVATE -Wall -Wextra -Wpedantic -g)

# Character Model Loading Test
add_executable(arena-test-character
    test_character_load.c
)

target_link_libraries(arena-test-character
    PRIVATE arena-core
    PRIVATE m
)

target_compile_options(arena-test-character PRIVATE -Wall -Wextra -Wpedantic -g)

# Animation Playback Integration Test
add_executable(arena-test-animation-playback
    test_animation_playback.c
)

target_link_libraries(arena-test-animation-playback
    PRIVATE arena-core
    PRIVATE m
)

target_compile_options(arena-test-animation-playback PRIVATE -Wall -Wextra -Wpedantic -g)

add_executable(arena-test-character-animation
    test_character_animation.c
)

target_link_libraries(arena-test-character-animation
    PRIVATE arena-core
    PRIVATE arena-renderer
    PRIVATE m
)

add_executable(arena-test-cgltf-load
    test_cgltf_load.c
)

target_link_libraries(arena-test-cgltf-load
    PRIVATE arena-core
    PRIVATE arena-renderer
    PRIVATE m
)

target_compile_options(arena-test-cgltf-load PRIVATE -Wall -Wextra -Wpedantic -g)

