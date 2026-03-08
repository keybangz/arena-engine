#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/input/input.h"
#include "arena/game/ability.h"
#include "arena/game/combat.h"
#include "arena/network/network.h"
#include "arena/network/client.h"
#include "renderer/renderer.h"
#include "renderer/sprite_batch.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// Configuration
// ============================================================================

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Arena Engine v0.6.0"

#define GAME_MAX_ENTITIES 10000

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
    SpriteBatch* sprite_batch;

    // ECS
    World* world;
    Entity player;

    // Input
    InputState input;

    // Networking
    NetClient* net_client;
    bool is_multiplayer;
} GameState;

static GameState g_state = {0};

// Player movement speed
#define PLAYER_SPEED 200.0f  // pixels per second

// ============================================================================
// Callbacks
// ============================================================================

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Forward to input system
    input_glfw_key_callback(window, key, scancode, action, mods);

    // Handle escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        g_state.should_quit = true;
    }
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    input_glfw_mouse_button_callback(window, button, action, mods);
}

static void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
    input_glfw_cursor_pos_callback(window, x, y);
}

static void glfw_scroll_callback(GLFWwindow* window, double x, double y) {
    input_glfw_scroll_callback(window, x, y);
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
    glfwSetMouseButtonCallback(g_state.window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(g_state.window, glfw_cursor_pos_callback);
    glfwSetScrollCallback(g_state.window, glfw_scroll_callback);
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

    // Initialize input
    input_init(&g_state.input);
    input_set_callback_target(&g_state.input);

    // Initialize ability registry
    ability_registry_init();

    // Initialize ECS world
    g_state.world = world_create(g_state.persistent_arena, GAME_MAX_ENTITIES);
    if (!g_state.world) {
        fprintf(stderr, "Failed to create ECS world\n");
        return false;
    }
    printf("ECS world created (max entities: %d)\n", GAME_MAX_ENTITIES);

    // Create player entity
    g_state.player = world_spawn(g_state.world);
    Transform* player_transform = world_add_transform(g_state.world, g_state.player);
    player_transform->x = WINDOW_WIDTH / 2.0f;
    player_transform->y = WINDOW_HEIGHT / 2.0f;
    player_transform->scale_x = 1.0f;
    player_transform->scale_y = 1.0f;

    Velocity* player_vel = world_add_velocity(g_state.world, g_state.player);
    player_vel->x = 0.0f;
    player_vel->y = 0.0f;

    Player* player_comp = world_add_player(g_state.world, g_state.player);
    player_comp->player_id = 0;
    player_comp->team = 0;

    // Add sprite component for rendering
    Sprite* player_sprite = world_add_sprite(g_state.world, g_state.player);
    player_sprite->width = 32.0f;
    player_sprite->height = 32.0f;
    player_sprite->color = sprite_color(0, 200, 255, 255);  // Cyan color
    player_sprite->layer = 10;

    printf("Player entity created at (%.0f, %.0f)\n", player_transform->x, player_transform->y);

    // Create some test entities (enemies)
    for (int i = 0; i < 5; i++) {
        Entity enemy = world_spawn(g_state.world);
        Transform* t = world_add_transform(g_state.world, enemy);
        t->x = 200.0f + i * 180.0f;
        t->y = 200.0f;
        t->scale_x = 1.0f;
        t->scale_y = 1.0f;

        Sprite* s = world_add_sprite(g_state.world, enemy);
        s->width = 28.0f;
        s->height = 28.0f;
        s->color = sprite_color(255, 50, 50, 255);  // Red enemies
        s->layer = 5;

        // Add health component
        Health* h = world_add_health(g_state.world, enemy);
        h->current = 100.0f;
        h->max = 100.0f;
    }
    printf("Created 5 test enemies\n");

    // Initialize renderer
    g_state.renderer = renderer_create(g_state.window, g_state.persistent_arena);
    if (!g_state.renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return false;
    }

    // Initialize sprite batch
    g_state.sprite_batch = sprite_batch_create(g_state.renderer);
    if (!g_state.sprite_batch) {
        fprintf(stderr, "Failed to create sprite batch\n");
        return false;
    }

    return true;
}

// ============================================================================
// Game Loop
// ============================================================================

static void fixed_update(double dt) {
    // Physics and game logic at fixed 30 Hz
    g_state.tick_count++;

    // Get player input direction
    float move_x, move_y;
    input_get_movement(&g_state.input, &move_x, &move_y);

    // Update player velocity
    Velocity* vel = world_get_velocity(g_state.world, g_state.player);
    if (vel) {
        vel->x = move_x * PLAYER_SPEED;
        vel->y = move_y * PLAYER_SPEED;
    }

    // Fire projectile on left mouse click
    if (input_mouse_pressed(&g_state.input, MOUSE_BUTTON_LEFT)) {
        double mx, my;
        input_mouse_position(&g_state.input, &mx, &my);
        ability_try_cast(g_state.world, g_state.player, 0, (float)mx, (float)my, ENTITY_NULL);
    }

    // Movement system: apply velocity to all entities with Transform + Velocity
    Query move_query = world_query(g_state.world,
        component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_VELOCITY));

    Entity entity;
    while (query_next(&move_query, &entity)) {
        Transform* t = world_get_transform(g_state.world, entity);
        Velocity* v = world_get_velocity(g_state.world, entity);

        if (t && v) {
            t->x += v->x * (float)dt;
            t->y += v->y * (float)dt;

            // Keep entities in bounds (despawn if out of screen)
            bool in_bounds = t->x >= -100 && t->x <= g_state.window_width + 100 &&
                            t->y >= -100 && t->y <= g_state.window_height + 100;

            // Only clamp player, let projectiles fly off
            if (world_has_component(g_state.world, entity, COMPONENT_PLAYER)) {
                if (t->x < 0) t->x = 0;
                if (t->y < 0) t->y = 0;
                if (t->x > g_state.window_width) t->x = (float)g_state.window_width;
                if (t->y > g_state.window_height) t->y = (float)g_state.window_height;
            } else if (!in_bounds) {
                // Despawn projectiles that go off screen
                world_despawn(g_state.world, entity);
            }
        }
    }

    // Run combat systems
    combat_projectile_system(g_state.world, (float)dt);
    combat_death_system(g_state.world, (float)dt);
}

static void render(double alpha) {
    (void)alpha;  // Will be used for interpolation
    g_state.frame_count++;

    // Reset frame arena each frame
    arena_reset(g_state.frame_arena);

    // Print player position every 120 frames (~2 seconds)
    if (g_state.frame_count % 120 == 0) {
        Transform* t = world_get_transform(g_state.world, g_state.player);
        if (t) {
            printf("Player: (%.1f, %.1f) | Entities: %u | Frame: %lu\n",
                   t->x, t->y, world_entity_count(g_state.world),
                   (unsigned long)g_state.frame_count);
        }
    }

    // Render
    if (renderer_begin_frame(g_state.renderer)) {
        // Draw all entities with Transform + Sprite components
        Query sprite_query = world_query(g_state.world,
            component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_SPRITE));

        Entity entity;
        while (query_next(&sprite_query, &entity)) {
            Transform* t = world_get_transform(g_state.world, entity);
            Sprite* s = world_get_sprite(g_state.world, entity);

            if (t && s) {
                // Draw sprite centered at transform position
                float x = t->x - s->width * 0.5f;
                float y = t->y - s->height * 0.5f;

                // Extract color components
                float r = (s->color & 0xFF) / 255.0f;
                float g = ((s->color >> 8) & 0xFF) / 255.0f;
                float b = ((s->color >> 16) & 0xFF) / 255.0f;
                float a = ((s->color >> 24) & 0xFF) / 255.0f;

                renderer_draw_quad(g_state.renderer, x, y, s->width, s->height, r, g, b, a);
            }
        }

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

        // Update input state for next frame (after all fixed updates)
        input_update(&g_state.input);

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
    if (g_state.sprite_batch) {
        sprite_batch_destroy(g_state.sprite_batch);
        g_state.sprite_batch = NULL;
    }

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

    printf("Arena Engine Client v0.6.0\n");
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
