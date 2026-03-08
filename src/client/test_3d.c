/**
 * test_3d.c - Simple 3D Test Scene for Arena Engine
 * 
 * Tests the 3D rendering foundation:
 * - Creates camera, lights, and mesh entities
 * - Renders spinning cubes with the mesh system
 * - Uses the camera system for isometric view
 * 
 * Compile with: make arena-test3d
 * Run with: ./build/arena-test3d
 */

#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/ecs/components_3d.h"
#include "arena/game/camera_system.h"
#include "renderer/renderer.h"
#include "renderer/mesh.h"
#include "renderer/render_3d.h"
#include "renderer/gltf_loader.h"
#include "renderer/texture.h"
#include "renderer/material.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// ============================================================================
// Configuration
// ============================================================================

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Arena Engine - 3D Test Scene"
#define TEST_MAX_ENTITIES  1000

// ============================================================================
// Test Scene State
// ============================================================================

typedef struct TestScene {
    GLFWwindow* window;
    Arena* arena;
    World* world;
    Renderer* renderer;  // Vulkan renderer
    Render3D render3d;   // 3D rendering system

    // Entities
    Entity camera_entity;
    Entity light_entity;
    Entity cube_entities[10];
    int cube_count;

    // Managers
    MeshManager mesh_manager;
    TextureManager texture_manager;
    MaterialManager material_manager;
    MeshHandle cube_mesh;
    MeshHandle plane_mesh;
    MaterialHandle cube_materials[10];
    bool meshes_initialized;

    // glTF loaded model
    GltfLoadResult gltf_box;
    Entity gltf_entity;

    // Time
    float time;
    bool running;
} TestScene;

static TestScene scene = {0};

// ============================================================================
// GLFW Callbacks
// ============================================================================

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window; (void)width; (void)height;
    // TODO: Handle resize
}

// ============================================================================
// Initialization
// ============================================================================

static bool init_window(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    scene.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!scene.window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return false;
    }
    
    glfwSetKeyCallback(scene.window, key_callback);
    glfwSetFramebufferSizeCallback(scene.window, framebuffer_size_callback);

    // Explicitly show window (required on some platforms like Wayland/KDE)
    glfwShowWindow(scene.window);

    printf("Window created: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    return true;
}

static bool init_scene(void) {
    // Create memory arena
    scene.arena = arena_create(64 * 1024 * 1024);  // 64 MB
    if (!scene.arena) {
        fprintf(stderr, "Failed to create memory arena\n");
        return false;
    }

    // Create Vulkan renderer
    scene.renderer = renderer_create(scene.window, scene.arena);
    if (!scene.renderer) {
        fprintf(stderr, "Failed to create Vulkan renderer\n");
        return false;
    }
    printf("Vulkan renderer created\n");

    // Get Vulkan handles
    VkDevice device = (VkDevice)renderer_get_device(scene.renderer);
    VkPhysicalDevice phys_device = (VkPhysicalDevice)renderer_get_physical_device(scene.renderer);
    VkRenderPass render_pass = (VkRenderPass)renderer_get_render_pass(scene.renderer);
    VkCommandPool command_pool = (VkCommandPool)renderer_get_command_pool(scene.renderer);
    VkQueue graphics_queue = (VkQueue)renderer_get_graphics_queue(scene.renderer);
    uint32_t width, height;
    renderer_get_extent(scene.renderer, &width, &height);

    // Initialize managers
    mesh_manager_init(&scene.mesh_manager, device, phys_device);
    if (!texture_manager_init(&scene.texture_manager, device, phys_device, command_pool, graphics_queue)) {
        fprintf(stderr, "Failed to initialize texture manager\n");
        return false;
    }
    if (!material_manager_init(&scene.material_manager, &scene.texture_manager)) {
        fprintf(stderr, "Failed to initialize material manager\n");
        return false;
    }
    printf("Managers initialized (mesh, texture, material)\n");

    // Initialize 3D renderer
    if (!render3d_init(&scene.render3d, device, phys_device, render_pass,
                       command_pool, graphics_queue, width, height,
                       &scene.mesh_manager, &scene.texture_manager, &scene.material_manager)) {
        fprintf(stderr, "Failed to initialize 3D renderer\n");
        return false;
    }
    printf("3D renderer initialized\n");

    // Create ECS world
    scene.world = world_create(scene.arena, TEST_MAX_ENTITIES);
    if (!scene.world) {
        fprintf(stderr, "Failed to create ECS world\n");
        return false;
    }

    printf("ECS world created\n");
    
    // =========================================================================
    // Create Camera Entity
    // =========================================================================
    scene.camera_entity = world_spawn(scene.world);
    
    Transform3D* cam_transform = world_add_transform3d(scene.world, scene.camera_entity);
    transform3d_init(cam_transform);
    cam_transform->position = vec3(0, 10, 15);  // Behind and above origin
    
    Camera* cam = world_add_camera(scene.world, scene.camera_entity);
    camera_init_perspective(cam, 60.0f * ARENA_DEG2RAD, 0.1f, 1000.0f);
    cam->is_active = true;
    
    // Set up isometric-style view looking at origin
    camera_set_isometric(cam, cam_transform, vec3(0, 0, 0), 20.0f, ISOMETRIC_PITCH, 0);
    
    printf("Camera entity created at (%.1f, %.1f, %.1f)\n",
           cam_transform->position.x, cam_transform->position.y, cam_transform->position.z);
    
    // =========================================================================
    // Create Light Entity
    // =========================================================================
    scene.light_entity = world_spawn(scene.world);
    
    Transform3D* light_transform = world_add_transform3d(scene.world, scene.light_entity);
    transform3d_init(light_transform);
    light_transform->position = vec3(10, 20, 10);
    
    Light* light = world_add_light(scene.world, scene.light_entity);
    light_init_directional(light, vec3(1.0f, 0.95f, 0.9f), 1.0f);
    
    printf("Directional light created\n");
    
    // =========================================================================
    // Create Cube Entities
    // =========================================================================
    // Create cube mesh on GPU
    scene.cube_mesh = mesh_create_cube(&scene.mesh_manager);
    if (scene.cube_mesh != MESH_HANDLE_INVALID) {
        scene.meshes_initialized = true;
        printf("Cube mesh created (handle: %u)\n", scene.cube_mesh);
    } else {
        fprintf(stderr, "Warning: Failed to create cube mesh\n");
    }

    scene.cube_count = 5;

    // Create colored materials for each cube
    Vec4 colors[] = {
        vec4(1.0f, 0.2f, 0.2f, 1.0f),  // Red
        vec4(0.2f, 1.0f, 0.2f, 1.0f),  // Green
        vec4(0.2f, 0.4f, 1.0f, 1.0f),  // Blue
        vec4(1.0f, 1.0f, 0.2f, 1.0f),  // Yellow
        vec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange
    };

    for (int i = 0; i < scene.cube_count; i++) {
        // Create material with unique color
        scene.cube_materials[i] = material_create_pbr(&scene.material_manager, colors[i],
            0.0f + (i * 0.2f),   // Increasing metallic
            0.3f + (i * 0.1f),   // Increasing roughness
            texture_get_white(&scene.texture_manager));

        scene.cube_entities[i] = world_spawn(scene.world);

        Transform3D* t = world_add_transform3d(scene.world, scene.cube_entities[i]);
        transform3d_init(t);
        t->position = vec3((i - 2) * 3.0f, 0.5f, 0);
        t->scale = vec3(1.0f, 1.0f, 1.0f);

        MeshRenderer* mr = world_add_mesh_renderer(scene.world, scene.cube_entities[i]);
        mesh_renderer_init(mr);
        mr->mesh_id = scene.cube_mesh;
        mr->material_id = scene.cube_materials[i];  // Use unique colored material
        mr->cast_shadows = true;
    }

    printf("Created %d cube entities with colored materials\n", scene.cube_count);

    // =========================================================================
    // Test glTF Loading
    // =========================================================================
    printf("\n--- Testing glTF Loader ---\n");

    // Try to load the Box.glb model (path is relative to build directory)
    if (gltf_load_file(&scene.mesh_manager, "../assets/models/Box.glb", &scene.gltf_box)) {
        printf("glTF Load Success!\n");
        printf("  Meshes: %u\n", scene.gltf_box.mesh_count);
        printf("  Bounds: min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
               scene.gltf_box.bounds_min.x, scene.gltf_box.bounds_min.y, scene.gltf_box.bounds_min.z,
               scene.gltf_box.bounds_max.x, scene.gltf_box.bounds_max.y, scene.gltf_box.bounds_max.z);

        for (uint32_t i = 0; i < scene.gltf_box.mesh_count; i++) {
            printf("  [%u] %s: %u verts, %u indices%s\n",
                   i, scene.gltf_box.meshes[i].name,
                   scene.gltf_box.meshes[i].vertex_count,
                   scene.gltf_box.meshes[i].index_count,
                   scene.gltf_box.meshes[i].is_skinned ? " (skinned)" : "");
        }
    } else {
        printf("glTF Load Failed: %s\n", scene.gltf_box.error_message);
        printf("  (This is expected - we need a MeshManager with valid Vulkan device)\n");
    }

    printf("--- glTF Test Complete ---\n\n");

    scene.running = true;
    return true;
}

// ============================================================================
// Update Loop
// ============================================================================

static void update(float dt) {
    scene.time += dt;

    // Rotate cubes
    for (int i = 0; i < scene.cube_count; i++) {
        Transform3D* t = world_get_transform3d(scene.world, scene.cube_entities[i]);
        if (t) {
            // Rotate around Y axis at different speeds
            float angle = scene.time * (0.5f + i * 0.2f);
            t->rotation = quat_from_axis_angle(vec3(0, 1, 0), angle);

            // Bob up and down
            t->position.y = 0.5f + sinf(scene.time * 2.0f + i) * 0.3f;

            t->dirty = true;
        }
    }

    // Update all transforms
    for (int i = 0; i < scene.cube_count; i++) {
        Transform3D* t = world_get_transform3d(scene.world, scene.cube_entities[i]);
        if (t) {
            transform3d_update_local(t);
        }
    }

    // Update camera matrices
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    camera_system_update(scene.world, dt, aspect);
}

static void render(void) {
    // Begin Vulkan frame
    if (!renderer_begin_frame(scene.renderer)) {
        return;  // Swapchain out of date, skip frame
    }

    // Get camera matrices
    Mat4 view, projection;
    Vec3 cam_pos;
    camera_system_get_matrices(scene.world, &view, &projection, &cam_pos);

    // Submit 3D draw calls for each cube entity
    if (scene.meshes_initialized) {
        // Begin 3D frame with camera/lighting
        render3d_begin_frame(&scene.render3d, view, projection, cam_pos,
                            scene.render3d.light_direction, scene.render3d.light_color,
                            scene.render3d.light_intensity, scene.render3d.ambient_intensity);

        for (int i = 0; i < scene.cube_count; i++) {
            Transform3D* t = world_get_transform3d(scene.world, scene.cube_entities[i]);
            MeshRenderer* mr = world_get_mesh_renderer(scene.world, scene.cube_entities[i]);

            if (t && mr && mr->mesh_id != MESH_HANDLE_INVALID) {
                // Update local matrix and use it
                transform3d_update_local(t);
                render3d_draw_mesh(&scene.render3d, mr->mesh_id, t->local_matrix, mr->material_id);
            }
        }

        // Execute 3D draw calls
        VkCommandBuffer cmd = (VkCommandBuffer)renderer_get_command_buffer(scene.renderer);
        render3d_end_frame(&scene.render3d, cmd, 0);
    } else {
        // Fallback: draw 2D quads if meshes not available
        for (int i = 0; i < scene.cube_count; i++) {
            Transform3D* t = world_get_transform3d(scene.world, scene.cube_entities[i]);
            if (t) {
                float screen_x = 640 + t->position.x * 50;
                float screen_y = 360 - t->position.y * 50;
                float size = 40.0f;
                float hue = (float)i / scene.cube_count;
                float r = 0.5f + 0.5f * sinf(hue * 6.28f);
                float g = 0.5f + 0.5f * sinf(hue * 6.28f + 2.09f);
                float b = 0.5f + 0.5f * sinf(hue * 6.28f + 4.18f);
                renderer_draw_quad(scene.renderer, screen_x - size/2, screen_y - size/2, size, size, r, g, b, 1.0f);
            }
        }
    }

    // End Vulkan frame (submit and present)
    renderer_end_frame(scene.renderer);

    // Print status every second
    static float print_timer = 0;
    print_timer += 0.016f;

    if (print_timer > 1.0f) {
        print_timer = 0;
        printf("Frame | Cubes: %d | 3D: %s | Time: %.1fs\n",
               scene.cube_count, scene.meshes_initialized ? "ON" : "OFF", scene.time);
    }
}

// ============================================================================
// Main Loop
// ============================================================================

static void run(void) {
    printf("Starting 3D test scene...\n");
    printf("Press ESC to exit\n\n");

    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(scene.window)) {
        double current_time = glfwGetTime();
        float dt = (float)(current_time - last_time);
        last_time = current_time;

        // Clamp delta time
        if (dt > 0.1f) dt = 0.1f;

        glfwPollEvents();

        update(dt);
        render();

        // Simple frame limiter (~60 FPS)
        double frame_end = glfwGetTime();
        double frame_time = frame_end - current_time;
        if (frame_time < 0.016) {
            // Sleep would be ideal here, but for simplicity just busy wait
        }
    }
}

// ============================================================================
// Cleanup
// ============================================================================

static void cleanup(void) {
    printf("\nCleaning up...\n");

    // Cleanup 3D renderer and managers (order matters: render3d first, then materials, textures, meshes)
    render3d_cleanup(&scene.render3d);
    material_manager_cleanup(&scene.material_manager);
    texture_manager_cleanup(&scene.texture_manager);
    mesh_manager_cleanup(&scene.mesh_manager);

    // Destroy renderer (needs valid window)
    if (scene.renderer) {
        renderer_destroy(scene.renderer);
        scene.renderer = NULL;
    }

    if (scene.arena) {
        arena_destroy(scene.arena);
    }

    if (scene.window) {
        glfwDestroyWindow(scene.window);
    }

    glfwTerminate();
    printf("Cleanup complete.\n");
}

// ============================================================================
// Entry Point
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("========================================\n");
    printf("Arena Engine - 3D Test Scene\n");
    printf("========================================\n\n");

    if (!init_window()) {
        return 1;
    }

    if (!init_scene()) {
        cleanup();
        return 1;
    }

    run();

    cleanup();
    return 0;
}

