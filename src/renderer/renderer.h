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

// Get current command buffer (for sprite batch integration)
void* renderer_get_command_buffer(Renderer* renderer);
void renderer_get_extent(Renderer* renderer, uint32_t* width, uint32_t* height);

// Draw a colored quad (call between begin_frame and end_frame)
void renderer_draw_quad(Renderer* renderer,
                        float x, float y, float width, float height,
                        float r, float g, float b, float a);

#endif
