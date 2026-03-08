#include "arena/arena.h"
#include "arena/math/math.h"

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
#define MAX_PLAYERS      10

// ============================================================================
// Server State
// ============================================================================

typedef struct ServerState {
    Arena* persistent_arena;
    Arena* tick_arena;

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

static void server_tick(double dt) {
    (void)dt;  // Will be used when physics is implemented

    g_server.tick_count++;

    // Reset per-tick allocator
    arena_reset(g_server.tick_arena);

    // TODO: Process client inputs
    // TODO: Run ECS systems
    // TODO: Broadcast state updates

    // Print stats every 5 seconds
    if (g_server.tick_count % (TICK_RATE * 5) == 0) {
        printf("Server tick %lu (%.1f seconds)\n",
               (unsigned long)g_server.tick_count,
               (double)g_server.tick_count / TICK_RATE);
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
            server_tick(TICK_INTERVAL);
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

    printf("Arena Engine Server v0.2.0\n");
    printf("==========================\n");

    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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

    printf("Server ready on port %d\n", SERVER_PORT);
    printf("Press Ctrl+C to shutdown.\n\n");

    g_server.running = true;
    run_server_loop();

    // Cleanup
    arena_destroy(g_server.tick_arena);
    arena_destroy(g_server.persistent_arena);

    printf("Server shutdown complete.\n");
    return 0;
}
