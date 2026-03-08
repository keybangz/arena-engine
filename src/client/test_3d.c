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

    // Entities
    Entity camera_entity;
    Entity light_entity;
    Entity cube_entities[10];
    int cube_count;

    // Mesh manager (holds GPU mesh data)
    MeshManager mesh_manager;
    MeshHandle cube_mesh;
    MeshHandle plane_mesh;

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
    scene.cube_count = 5;
    
    for (int i = 0; i < scene.cube_count; i++) {
        scene.cube_entities[i] = world_spawn(scene.world);
        
        Transform3D* t = world_add_transform3d(scene.world, scene.cube_entities[i]);
        transform3d_init(t);
        
        // Arrange cubes in a row
        t->position = vec3((i - 2) * 3.0f, 0.5f, 0);
        t->scale = vec3(1.0f, 1.0f, 1.0f);
        
        MeshRenderer* mr = world_add_mesh_renderer(scene.world, scene.cube_entities[i]);
        mesh_renderer_init(mr);
        mr->mesh_id = 1;  // Will be set to actual cube mesh handle
        mr->material_id = 0;
        mr->cast_shadows = true;
    }
    
    printf("Created %d cube entities\n", scene.cube_count);

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

    // Draw cubes as colored quads (placeholder until 3D pipeline is complete)
    // Each cube is represented as a quad on screen for now
    for (int i = 0; i < scene.cube_count; i++) {
        Transform3D* t = world_get_transform3d(scene.world, scene.cube_entities[i]);
        if (t) {
            // Map 3D position to 2D screen space (simple orthographic projection)
            float screen_x = 640 + t->position.x * 50;  // Center + offset
            float screen_y = 360 - t->position.y * 50;  // Center - offset (Y flipped)
            float size = 40.0f;

            // Different colors for each cube
            float hue = (float)i / scene.cube_count;
            float r = 0.5f + 0.5f * sinf(hue * 6.28f);
            float g = 0.5f + 0.5f * sinf(hue * 6.28f + 2.09f);
            float b = 0.5f + 0.5f * sinf(hue * 6.28f + 4.18f);

            renderer_draw_quad(scene.renderer,
                screen_x - size/2, screen_y - size/2,
                size, size,
                r, g, b, 1.0f);
        }
    }

    // End Vulkan frame (submit and present)
    renderer_end_frame(scene.renderer);

    // Print status every second
    static float print_timer = 0;
    print_timer += 0.016f;

    if (print_timer > 1.0f) {
        print_timer = 0;

        // Get camera info
        Mat4 view, projection;
        Vec3 cam_pos;
        if (camera_system_get_matrices(scene.world, &view, &projection, &cam_pos)) {
            printf("Frame | Camera: (%.1f, %.1f, %.1f) | Cubes: %d | Time: %.1fs\n",
                   cam_pos.x, cam_pos.y, cam_pos.z, scene.cube_count, scene.time);
        }
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

    // Destroy renderer first (needs valid window)
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

