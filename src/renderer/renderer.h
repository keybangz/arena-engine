#ifndef ARENA_RENDERER_H
#define ARENA_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct GLFWwindow GLFWwindow;
typedef struct Arena Arena;

// ============================================================================
// Renderer Types
// ============================================================================

typedef struct Renderer Renderer;

typedef struct RendererConfig {
    bool enable_validation;
    bool enable_vsync;
    uint32_t max_frames_in_flight;
} RendererConfig;

// Default configuration
static inline RendererConfig renderer_config_default(void) {
    return (RendererConfig){
        .enable_validation = true,
        .enable_vsync = true,
        .max_frames_in_flight = 2
    };
}

// ============================================================================
// Renderer API
// ============================================================================

// Lifecycle
Renderer* renderer_create(GLFWwindow* window, Arena* arena);
Renderer* renderer_create_with_config(GLFWwindow* window, Arena* arena, RendererConfig config);
void renderer_destroy(Renderer* renderer);

// Frame operations
bool renderer_begin_frame(Renderer* renderer);
void renderer_end_frame(Renderer* renderer);

// State queries
bool renderer_is_valid(const Renderer* renderer);
void renderer_wait_idle(Renderer* renderer);

// Window resize handling
void renderer_on_resize(Renderer* renderer, int width, int height);

// Stats
uint64_t renderer_get_frame_count(const Renderer* renderer);

// Simple shape drawing (temporary until sprite system)
void renderer_set_viewport_size(Renderer* renderer, float width, float height);
void renderer_draw_quad(Renderer* renderer, float x, float y, float width, float height, uint32_t color);

#endif
