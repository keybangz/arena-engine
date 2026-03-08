#ifndef ARENA_SPRITE_BATCH_H
#define ARENA_SPRITE_BATCH_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct Renderer Renderer;
typedef struct SpriteBatch SpriteBatch;

// ============================================================================
// Texture
// ============================================================================

typedef struct Texture {
    uint32_t id;
    uint32_t width;
    uint32_t height;
} Texture;

#define TEXTURE_NULL ((Texture){0, 0, 0})

// ============================================================================
// Sprite Vertex
// ============================================================================

typedef struct SpriteVertex {
    float x, y;         // Position
    float u, v;         // Texture coordinates
    float r, g, b, a;   // Color
} SpriteVertex;

// ============================================================================
// Sprite Batch API
// ============================================================================

// Batch configuration
#define SPRITE_BATCH_MAX_SPRITES    10000
#define SPRITE_BATCH_MAX_TEXTURES   16

// Creation/destruction
SpriteBatch* sprite_batch_create(Renderer* renderer);
void sprite_batch_destroy(SpriteBatch* batch);

// Texture management
Texture sprite_batch_load_texture(SpriteBatch* batch, const char* path);
Texture sprite_batch_create_white_texture(SpriteBatch* batch);
void sprite_batch_bind_texture(SpriteBatch* batch, Texture texture);

// Batch operations
void sprite_batch_begin(SpriteBatch* batch);
void sprite_batch_end(SpriteBatch* batch);

// Draw calls (call between begin/end)
void sprite_batch_draw(SpriteBatch* batch, 
                       float x, float y, float width, float height,
                       float u0, float v0, float u1, float v1,
                       uint32_t color);

void sprite_batch_draw_quad(SpriteBatch* batch,
                            float x, float y, float width, float height,
                            uint32_t color);

void sprite_batch_draw_texture(SpriteBatch* batch, Texture texture,
                               float x, float y, float width, float height);

void sprite_batch_draw_texture_region(SpriteBatch* batch, Texture texture,
                                      float x, float y, float width, float height,
                                      float u0, float v0, float u1, float v1);

// Utility
static inline uint32_t sprite_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
}

#define SPRITE_COLOR_WHITE  sprite_color(255, 255, 255, 255)
#define SPRITE_COLOR_RED    sprite_color(255, 0, 0, 255)
#define SPRITE_COLOR_GREEN  sprite_color(0, 255, 0, 255)
#define SPRITE_COLOR_BLUE   sprite_color(0, 0, 255, 255)
#define SPRITE_COLOR_YELLOW sprite_color(255, 255, 0, 255)

#endif

