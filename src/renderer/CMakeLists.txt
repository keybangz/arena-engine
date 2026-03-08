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

# Compile shaders
find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
    set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

    add_custom_command(
        OUTPUT ${SHADER_OUT_DIR}/triangle.vert.spv
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_DIR}/triangle.vert -o ${SHADER_OUT_DIR}/triangle.vert.spv
        DEPENDS ${SHADER_DIR}/triangle.vert
        COMMENT "Compiling triangle.vert"
    )

    add_custom_command(
        OUTPUT ${SHADER_OUT_DIR}/triangle.frag.spv
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_DIR}/triangle.frag -o ${SHADER_OUT_DIR}/triangle.frag.spv
        DEPENDS ${SHADER_DIR}/triangle.frag
        COMMENT "Compiling triangle.frag"
    )

    add_custom_target(shaders ALL
        DEPENDS
            ${SHADER_OUT_DIR}/triangle.vert.spv
            ${SHADER_OUT_DIR}/triangle.frag.spv
    )

    add_dependencies(arena-renderer shaders)
endif()
