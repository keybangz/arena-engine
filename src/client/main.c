#include "arena/arena.h"
#include "arena/math/math.h"
#include "renderer/renderer.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// Configuration
// ============================================================================

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Arena Engine v0.2.0"

#define TARGET_FPS       60
#define FIXED_TIMESTEP   (1.0 / 30.0)  // 30 Hz simulation
#define MAX_FRAME_TIME   0.25          // Prevent spiral of death

// ============================================================================
// Time Utilities
// ============================================================================

static double get_time(void) {
    return glfwGetTime();
}

// ============================================================================
// Game State
// ============================================================================

typedef struct GameState {
    Arena* persistent_arena;
    Arena* frame_arena;

    // Timing
    double current_time;
    double accumulator;
    uint64_t tick_count;
    uint64_t frame_count;

    // Window
    GLFWwindow* window;
    int window_width;
    int window_height;
    bool should_quit;

    // Renderer
    Renderer* renderer;
} GameState;

static GameState g_state = {0};

// ============================================================================
// Callbacks
// ============================================================================

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        g_state.should_quit = true;
    }
}

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    g_state.window_width = width;
    g_state.window_height = height;
    renderer_on_resize(g_state.renderer, width, height);
}

// ============================================================================
// Initialization
// ============================================================================

static bool init_window(void) {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Vulkan requires no OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    g_state.window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        WINDOW_TITLE,
        NULL, NULL
    );

    if (!g_state.window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwSetKeyCallback(g_state.window, glfw_key_callback);
    glfwSetFramebufferSizeCallback(g_state.window, glfw_framebuffer_size_callback);

    glfwGetFramebufferSize(g_state.window, &g_state.window_width, &g_state.window_height);

    printf("Window created: %dx%d\n", g_state.window_width, g_state.window_height);
    return true;
}

static bool init_game(void) {
    // Create memory arenas
    g_state.persistent_arena = arena_create(256 * 1024 * 1024);  // 256 MB
    if (!g_state.persistent_arena) {
        fprintf(stderr, "Failed to create persistent arena\n");
        return false;
    }

    g_state.frame_arena = arena_create(16 * 1024 * 1024);  // 16 MB
    if (!g_state.frame_arena) {
        fprintf(stderr, "Failed to create frame arena\n");
        return false;
    }

    printf("Memory arenas created: persistent=%zu MB, frame=%zu MB\n",
           arena_capacity(g_state.persistent_arena) / (1024 * 1024),
           arena_capacity(g_state.frame_arena) / (1024 * 1024));

    // Initialize renderer
    g_state.renderer = renderer_create(g_state.window, g_state.persistent_arena);
    if (!g_state.renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return false;
    }

    return true;
}

// ============================================================================
// Game Loop
// ============================================================================

static void fixed_update(double dt) {
    (void)dt;  // Will be used when physics is implemented

    // Physics and game logic at fixed 30 Hz
    g_state.tick_count++;

    // TODO: Update ECS systems
    // TODO: Process inputs
    // TODO: Run physics
}

static void render(double alpha) {
    (void)alpha;  // Will be used for interpolation when ECS is implemented

    // Interpolate between states using alpha for smooth rendering
    g_state.frame_count++;

    // Reset frame arena each frame
    arena_reset(g_state.frame_arena);

    // Render
    if (renderer_begin_frame(g_state.renderer)) {
        // TODO: Submit draw calls

        renderer_end_frame(g_state.renderer);
    }
}

static void run_game_loop(void) {
    g_state.current_time = get_time();
    g_state.accumulator = 0.0;

    printf("Entering game loop (fixed timestep: %.1f Hz, target FPS: %d)\n",
           1.0 / FIXED_TIMESTEP, TARGET_FPS);

    while (!g_state.should_quit && !glfwWindowShouldClose(g_state.window)) {
        double new_time = get_time();
        double frame_time = new_time - g_state.current_time;
        g_state.current_time = new_time;

        // Clamp frame time to prevent spiral of death
        if (frame_time > MAX_FRAME_TIME) {
            frame_time = MAX_FRAME_TIME;
        }

        g_state.accumulator += frame_time;

        // Process input
        glfwPollEvents();

        // Fixed timestep updates
        while (g_state.accumulator >= FIXED_TIMESTEP) {
            fixed_update(FIXED_TIMESTEP);
            g_state.accumulator -= FIXED_TIMESTEP;
        }

        // Render with interpolation alpha
        double alpha = g_state.accumulator / FIXED_TIMESTEP;
        render(alpha);
    }

    printf("Game loop ended. Ticks: %lu, Frames: %lu\n",
           (unsigned long)g_state.tick_count,
           (unsigned long)g_state.frame_count);
}

// ============================================================================
// Shutdown
// ============================================================================

static void shutdown_game(void) {
    if (g_state.renderer) {
        renderer_destroy(g_state.renderer);
        g_state.renderer = NULL;
    }

    if (g_state.frame_arena) {
        arena_destroy(g_state.frame_arena);
        g_state.frame_arena = NULL;
    }

    if (g_state.persistent_arena) {
        arena_destroy(g_state.persistent_arena);
        g_state.persistent_arena = NULL;
    }

    if (g_state.window) {
        glfwDestroyWindow(g_state.window);
        g_state.window = NULL;
    }

    glfwTerminate();
}

// ============================================================================
// Entry Point
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("Arena Engine Client v0.2.0\n");
    printf("==========================\n");

    if (!init_window()) {
        return 1;
    }

    if (!init_game()) {
        shutdown_game();
        return 1;
    }

    run_game_loop();

    shutdown_game();

    printf("Client shutdown complete.\n");
    return 0;
}
