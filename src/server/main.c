#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/network/network.h"
#include "arena/network/server.h"
#include "arena/game/ability.h"
#include "arena/game/combat.h"
#include "arena/game/ai.h"
#include "arena/game/spawner.h"
#include "arena/map/map.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

// ============================================================================
// Configuration
// ============================================================================

#define SERVER_PORT      7777
#define TICK_RATE        30          // 30 Hz
#define TICK_INTERVAL    (1.0 / TICK_RATE)
#define SERVER_MAX_ENTITIES  10000

// ============================================================================
// Server State
// ============================================================================

typedef struct ServerState {
    Arena* persistent_arena;
    Arena* tick_arena;
    World* world;
    GameServer* game_server;
    Map* map;
    Spawner spawner;

    // Timing
    uint64_t tick_count;
    double current_time;
    double accumulator;

    // Control
    bool running;
} ServerState;

static ServerState g_server = {0};

// ============================================================================
// Time Utilities
// ============================================================================

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

// ============================================================================
// Signal Handling
// ============================================================================

static void signal_handler(int sig) {
    (void)sig;
    printf("\nShutdown signal received.\n");
    g_server.running = false;
}

// ============================================================================
// Game Logic
// ============================================================================

static void do_server_tick(double dt) {
    g_server.tick_count++;

    // Reset per-tick allocator
    arena_reset(g_server.tick_arena);

    // Network: receive packets and send state
    server_update(g_server.game_server, dt);

    // Movement system
    Query move_query = world_query(g_server.world,
        component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_VELOCITY));

    Entity entity;
    while (query_next(&move_query, &entity)) {
        Transform* t = world_get_transform(g_server.world, entity);
        Velocity* v = world_get_velocity(g_server.world, entity);

        if (t && v) {
            t->x += v->x * (float)dt;
            t->y += v->y * (float)dt;

            // Boundary check for non-player entities (projectiles)
            if (!world_has_component(g_server.world, entity, COMPONENT_PLAYER)) {
                if (t->x < -100 || t->x > 1380 || t->y < -100 || t->y > 820) {
                    world_despawn(g_server.world, entity);
                }
            }
        }
    }

    // AI system
    ai_system_update(g_server.world, g_server.map, (float)dt);

    // Spawner system
    spawner_update(&g_server.spawner, g_server.world, g_server.map, (float)dt);

    // Combat systems
    combat_projectile_system(g_server.world, (float)dt);
    combat_death_system(g_server.world, (float)dt);

    // Print stats every 5 seconds
    if (g_server.tick_count % (TICK_RATE * 5) == 0) {
        printf("Server tick %lu | Entities: %u | Clients: %u | Wave: %u\n",
               (unsigned long)g_server.tick_count,
               world_entity_count(g_server.world),
               g_server.game_server->client_count,
               g_server.spawner.wave_count);
    }
}

// ============================================================================
// Main Loop
// ============================================================================

static void run_server_loop(void) {
    g_server.current_time = get_time();
    g_server.accumulator = 0.0;

    printf("Server loop started (tick rate: %d Hz)\n", TICK_RATE);

    while (g_server.running) {
        double new_time = get_time();
        double frame_time = new_time - g_server.current_time;
        g_server.current_time = new_time;

        // Clamp frame time
        if (frame_time > 0.25) {
            frame_time = 0.25;
        }

        g_server.accumulator += frame_time;

        // Fixed timestep ticks
        while (g_server.accumulator >= TICK_INTERVAL) {
            do_server_tick(TICK_INTERVAL);
            g_server.accumulator -= TICK_INTERVAL;
        }

        // Sleep to reduce CPU usage (rough timing, accumulator handles precision)
        double sleep_time = TICK_INTERVAL - g_server.accumulator;
        if (sleep_time > 0.001) {
            sleep_ms((int)(sleep_time * 1000 * 0.9));  // Sleep 90% of remaining time
        }
    }

    printf("Server loop ended. Total ticks: %lu\n", (unsigned long)g_server.tick_count);
}

// ============================================================================
// Entry Point
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("Arena Engine Server v0.6.0\n");
    printf("==========================\n");

    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize networking
    if (!net_init()) {
        fprintf(stderr, "Failed to initialize networking\n");
        return 1;
    }

    // Initialize ability registry
    ability_registry_init();

    // Create memory arenas
    g_server.persistent_arena = arena_create(64 * 1024 * 1024);  // 64 MB
    if (!g_server.persistent_arena) {
        fprintf(stderr, "Failed to create persistent arena\n");
        return 1;
    }

    g_server.tick_arena = arena_create(4 * 1024 * 1024);  // 4 MB
    if (!g_server.tick_arena) {
        fprintf(stderr, "Failed to create tick arena\n");
        arena_destroy(g_server.persistent_arena);
        return 1;
    }

    printf("Memory arenas created: persistent=%zu MB, tick=%zu MB\n",
           arena_capacity(g_server.persistent_arena) / (1024 * 1024),
           arena_capacity(g_server.tick_arena) / (1024 * 1024));

    // Create ECS world
    g_server.world = world_create(g_server.persistent_arena, SERVER_MAX_ENTITIES);
    if (!g_server.world) {
        fprintf(stderr, "Failed to create ECS world\n");
        return 1;
    }
    printf("ECS world created (max entities: %d)\n", SERVER_MAX_ENTITIES);

    // Create map
    g_server.map = map_create_default();
    if (!g_server.map) {
        fprintf(stderr, "Failed to create map\n");
        return 1;
    }
    map_print(g_server.map);

    // Initialize spawner
    spawner_init(&g_server.spawner);

    // Spawn initial objects (towers)
    spawn_initial_objects(g_server.world, g_server.map);

    // Create game server
    g_server.game_server = server_create(SERVER_PORT, g_server.world);
    if (!g_server.game_server) {
        fprintf(stderr, "Failed to create game server\n");
        return 1;
    }

    printf("Server ready on port %d\n", SERVER_PORT);
    printf("Press Ctrl+C to shutdown.\n\n");

    g_server.running = true;
    run_server_loop();

    // Cleanup
    server_destroy(g_server.game_server);
    map_destroy(g_server.map);
    arena_destroy(g_server.tick_arena);
    arena_destroy(g_server.persistent_arena);
    net_shutdown();

    printf("Server shutdown complete.\n");
    return 0;
}
