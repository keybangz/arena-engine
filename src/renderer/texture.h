#ifndef TEXTURE_H
#define TEXTURE_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Texture Handle System
// ============================================================================

typedef uint32_t TextureHandle;
#define TEXTURE_HANDLE_INVALID 0xFFFFFFFF
#define MAX_TEXTURES 256

typedef struct Texture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet descriptor_set;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    bool is_valid;
} Texture;

typedef struct TextureManager {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;
    
    // Descriptor pool for texture sets
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout set_layout;
    
    // Texture storage
    Texture textures[MAX_TEXTURES];
    uint32_t texture_count;
    
    // Default textures
    TextureHandle white_texture;
    TextureHandle black_texture;
    TextureHandle normal_texture;  // flat normal (128, 128, 255)
} TextureManager;

// ============================================================================
// API
// ============================================================================

// Initialize texture manager
bool texture_manager_init(TextureManager* tm,
                          VkDevice device,
                          VkPhysicalDevice physical_device,
                          VkCommandPool command_pool,
                          VkQueue graphics_queue);

// Cleanup all textures
void texture_manager_cleanup(TextureManager* tm);

// Load texture from file (PNG, JPG, etc.)
TextureHandle texture_load(TextureManager* tm, const char* path);

// Load texture from memory
TextureHandle texture_load_memory(TextureManager* tm, 
                                  const uint8_t* data, 
                                  uint32_t width, 
                                  uint32_t height,
                                  uint32_t channels);

// Get texture for binding
Texture* texture_get(TextureManager* tm, TextureHandle handle);

// Get descriptor set layout (for pipeline creation)
VkDescriptorSetLayout texture_manager_get_set_layout(TextureManager* tm);

// Get default textures
TextureHandle texture_get_white(TextureManager* tm);
TextureHandle texture_get_black(TextureManager* tm);
TextureHandle texture_get_normal(TextureManager* tm);

#endif // TEXTURE_H

