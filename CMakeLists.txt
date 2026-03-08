cmake_minimum_required(VERSION 3.20)
project(ArenaEngine VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_CLIENT "Build game client" ON)
option(BUILD_SERVER "Build dedicated server" ON)
option(BUILD_TOOLS "Build editor tools" ON)
option(BUILD_TESTS "Build tests" OFF)

# Find Vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK: ${Vulkan_INCLUDE_DIR}")
endif()

# Find GLFW via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
message(STATUS "GLFW found: ${GLFW_INCLUDE_DIRS}")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
)

# Subdirectories
add_subdirectory(src/arena)
add_subdirectory(src/renderer)

if(BUILD_CLIENT)
    add_subdirectory(src/client)
endif()

if(BUILD_SERVER)
    add_subdirectory(src/server)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools/arena-editor)
    add_subdirectory(tools/asset-packer)
    add_subdirectory(tools/entity-editor)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== Arena Engine Configuration ===")
message(STATUS "  Client:  ${BUILD_CLIENT}")
message(STATUS "  Server:  ${BUILD_SERVER}")
message(STATUS "  Tools:   ${BUILD_TOOLS}")
message(STATUS "  Tests:   ${BUILD_TESTS}")
message(STATUS "===================================")
message(STATUS "")
