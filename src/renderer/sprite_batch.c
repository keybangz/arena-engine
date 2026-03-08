#include "sprite_batch.h"
#include "renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../libs/stb/stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Internal Structures
// ============================================================================

typedef struct TextureInternal {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    uint32_t width;
    uint32_t height;
    bool in_use;
} TextureInternal;

struct SpriteBatch {
    Renderer* renderer;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;

    // Pipeline
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[SPRITE_BATCH_MAX_TEXTURES];

    // Vertex buffer
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;
    SpriteVertex* vertices;
    uint32_t vertex_count;
    uint32_t max_vertices;

    // Index buffer
    VkBuffer index_buffer;
    VkDeviceMemory index_memory;

    // Textures
    TextureInternal textures[SPRITE_BATCH_MAX_TEXTURES];
    uint32_t texture_count;
    uint32_t current_texture;

    // State
    bool is_drawing;
    float screen_width;
    float screen_height;
};

// ============================================================================
// Vulkan Helper Functions
// ============================================================================

static uint32_t find_memory_type(VkPhysicalDevice phys_device, uint32_t type_filter,
                                  VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

static bool create_buffer(SpriteBatch* batch, VkDeviceSize size,
                          VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer* buffer, VkDeviceMemory* memory) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(batch->device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(batch->device, *buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type(batch->physical_device, mem_reqs.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(batch->device, &alloc_info, NULL, memory) != VK_SUCCESS) {
        vkDestroyBuffer(batch->device, *buffer, NULL);
        return false;
    }

    vkBindBufferMemory(batch->device, *buffer, *memory, 0);
    return true;
}

// ============================================================================
// Texture Loading
// ============================================================================

Texture sprite_batch_create_white_texture(SpriteBatch* batch) {
    if (batch->texture_count >= SPRITE_BATCH_MAX_TEXTURES) {
        return TEXTURE_NULL;
    }

    // Create 1x1 white pixel
    uint32_t pixel = 0xFFFFFFFF;
    uint32_t id = batch->texture_count++;
    TextureInternal* tex = &batch->textures[id];
    tex->width = 1;
    tex->height = 1;
    tex->in_use = true;

    // For now, just mark as valid - full implementation would create Vulkan image
    // This is a simplified version that works with the colored quad fallback

    return (Texture){id, 1, 1};
}

Texture sprite_batch_load_texture(SpriteBatch* batch, const char* path) {
    if (batch->texture_count >= SPRITE_BATCH_MAX_TEXTURES) {
        fprintf(stderr, "SpriteBatch: Max textures reached\n");
        return TEXTURE_NULL;
    }

    int width, height, channels;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        fprintf(stderr, "SpriteBatch: Failed to load texture: %s\n", path);
        return TEXTURE_NULL;
    }

    uint32_t id = batch->texture_count++;
    TextureInternal* tex = &batch->textures[id];
    tex->width = width;
    tex->height = height;
    tex->in_use = true;

    // TODO: Create Vulkan image, copy pixels, create view and sampler
    // For now, store dimensions only

    stbi_image_free(pixels);
    printf("SpriteBatch: Loaded texture %s (%dx%d) as ID %u\n", path, width, height, id);

    return (Texture){id, (uint32_t)width, (uint32_t)height};
}

// ============================================================================
// Batch Operations
// ============================================================================

void sprite_batch_begin(SpriteBatch* batch) {
    if (batch->is_drawing) {
        fprintf(stderr, "SpriteBatch: begin() called while already drawing\n");
        return;
    }
    batch->is_drawing = true;
    batch->vertex_count = 0;
}

void sprite_batch_end(SpriteBatch* batch) {
    if (!batch->is_drawing) {
        fprintf(stderr, "SpriteBatch: end() called without begin()\n");
        return;
    }
    batch->is_drawing = false;

    // Flush remaining vertices
    // TODO: Record draw commands to command buffer
}

void sprite_batch_bind_texture(SpriteBatch* batch, Texture texture) {
    batch->current_texture = texture.id;
}

// ============================================================================
// Drawing Functions
// ============================================================================

void sprite_batch_draw(SpriteBatch* batch,
                       float x, float y, float width, float height,
                       float u0, float v0, float u1, float v1,
                       uint32_t color) {
    if (!batch->is_drawing) return;
    if (batch->vertex_count + 6 > batch->max_vertices) return;  // Need 6 verts for quad

    // Extract color components
    float r = (color & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = ((color >> 16) & 0xFF) / 255.0f;
    float a = ((color >> 24) & 0xFF) / 255.0f;

    // Quad vertices (two triangles)
    SpriteVertex* v = &batch->vertices[batch->vertex_count];

    // Triangle 1: top-left, bottom-left, bottom-right
    v[0] = (SpriteVertex){x, y, u0, v0, r, g, b, a};
    v[1] = (SpriteVertex){x, y + height, u0, v1, r, g, b, a};
    v[2] = (SpriteVertex){x + width, y + height, u1, v1, r, g, b, a};

    // Triangle 2: top-left, bottom-right, top-right
    v[3] = (SpriteVertex){x, y, u0, v0, r, g, b, a};
    v[4] = (SpriteVertex){x + width, y + height, u1, v1, r, g, b, a};
    v[5] = (SpriteVertex){x + width, y, u1, v0, r, g, b, a};

    batch->vertex_count += 6;
}

void sprite_batch_draw_quad(SpriteBatch* batch,
                            float x, float y, float width, float height,
                            uint32_t color) {
    sprite_batch_draw(batch, x, y, width, height, 0, 0, 1, 1, color);
}

void sprite_batch_draw_texture(SpriteBatch* batch, Texture texture,
                               float x, float y, float width, float height) {
    sprite_batch_bind_texture(batch, texture);
    sprite_batch_draw(batch, x, y, width, height, 0, 0, 1, 1, SPRITE_COLOR_WHITE);
}

void sprite_batch_draw_texture_region(SpriteBatch* batch, Texture texture,
                                      float x, float y, float width, float height,
                                      float u0, float v0, float u1, float v1) {
    sprite_batch_bind_texture(batch, texture);
    sprite_batch_draw(batch, x, y, width, height, u0, v0, u1, v1, SPRITE_COLOR_WHITE);
}

// ============================================================================
// Creation / Destruction
// ============================================================================

SpriteBatch* sprite_batch_create(Renderer* renderer) {
    SpriteBatch* batch = calloc(1, sizeof(SpriteBatch));
    if (!batch) return NULL;

    batch->renderer = renderer;
    batch->max_vertices = SPRITE_BATCH_MAX_SPRITES * 6;  // 6 verts per sprite
    batch->screen_width = 1280;  // Default, will be updated
    batch->screen_height = 720;

    // Allocate CPU-side vertex buffer
    batch->vertices = malloc(batch->max_vertices * sizeof(SpriteVertex));
    if (!batch->vertices) {
        free(batch);
        return NULL;
    }

    // Create white texture as default
    sprite_batch_create_white_texture(batch);

    printf("SpriteBatch: Created (max sprites: %d)\n", SPRITE_BATCH_MAX_SPRITES);
    return batch;
}

void sprite_batch_destroy(SpriteBatch* batch) {
    if (!batch) return;

    // TODO: Destroy Vulkan resources

    free(batch->vertices);
    free(batch);
    printf("SpriteBatch: Destroyed\n");
}

void sprite_batch_set_screen_size(SpriteBatch* batch, float width, float height) {
    batch->screen_width = width;
    batch->screen_height = height;
}
