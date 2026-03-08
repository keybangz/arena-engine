#include "render_3d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Helper Functions
// ============================================================================

static uint32_t find_memory_type(VkPhysicalDevice physical_device,
                                  uint32_t type_filter,
                                  VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

static VkFormat find_depth_format(VkPhysicalDevice physical_device) {
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    for (size_t i = 0; i < 3; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return candidates[i];
        }
    }
    return VK_FORMAT_D32_SFLOAT; // Fallback
}

// ============================================================================
// Depth Buffer
// ============================================================================

static bool create_depth_buffer(Render3D* r3d, uint32_t width, uint32_t height) {
    r3d->depth_format = find_depth_format(r3d->physical_device);
    
    // Create depth image
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = r3d->depth_format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    
    if (vkCreateImage(r3d->device, &image_info, NULL, &r3d->depth_image) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate memory
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(r3d->device, r3d->depth_image, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type(r3d->physical_device, mem_reqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    
    if (vkAllocateMemory(r3d->device, &alloc_info, NULL, &r3d->depth_memory) != VK_SUCCESS) {
        vkDestroyImage(r3d->device, r3d->depth_image, NULL);
        return false;
    }
    
    vkBindImageMemory(r3d->device, r3d->depth_image, r3d->depth_memory, 0);
    
    // Create image view
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = r3d->depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = r3d->depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    if (vkCreateImageView(r3d->device, &view_info, NULL, &r3d->depth_view) != VK_SUCCESS) {
        vkFreeMemory(r3d->device, r3d->depth_memory, NULL);
        vkDestroyImage(r3d->device, r3d->depth_image, NULL);
        return false;
    }
    
    return true;
}

static void destroy_depth_buffer(Render3D* r3d) {
    if (r3d->depth_view) vkDestroyImageView(r3d->device, r3d->depth_view, NULL);
    if (r3d->depth_memory) vkFreeMemory(r3d->device, r3d->depth_memory, NULL);
    if (r3d->depth_image) vkDestroyImage(r3d->device, r3d->depth_image, NULL);
    r3d->depth_view = VK_NULL_HANDLE;
    r3d->depth_memory = VK_NULL_HANDLE;
    r3d->depth_image = VK_NULL_HANDLE;
}

// ============================================================================
// Default Texture (1x1 white)
// ============================================================================

static bool create_default_texture(Render3D* r3d) {
    // Create 1x1 white image
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {1, 1, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
    };
    
    if (vkCreateImage(r3d->device, &image_info, NULL, &r3d->default_texture) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(r3d->device, r3d->default_texture, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type(r3d->physical_device, mem_reqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };
    
    if (vkAllocateMemory(r3d->device, &alloc_info, NULL, &r3d->default_texture_memory) != VK_SUCCESS) {
        vkDestroyImage(r3d->device, r3d->default_texture, NULL);
        return false;
    }
    
    vkBindImageMemory(r3d->device, r3d->default_texture, r3d->default_texture_memory, 0);
    
    // Write white pixel
    void* data;
    vkMapMemory(r3d->device, r3d->default_texture_memory, 0, 4, 0, &data);
    uint8_t white[] = {255, 255, 255, 255};
    memcpy(data, white, 4);
    vkUnmapMemory(r3d->device, r3d->default_texture_memory);

    // Create image view
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = r3d->default_texture,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if (vkCreateImageView(r3d->device, &view_info, NULL, &r3d->default_texture_view) != VK_SUCCESS) {
        return false;
    }

    // Create sampler
    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .minLod = 0.0f,
        .maxLod = 1.0f
    };

    if (vkCreateSampler(r3d->device, &sampler_info, NULL, &r3d->default_sampler) != VK_SUCCESS) {
        return false;
    }

    return true;
}

// ============================================================================
// Public API
// ============================================================================

bool render3d_init(Render3D* r3d,
                   VkDevice device,
                   VkPhysicalDevice physical_device,
                   VkRenderPass render_pass,
                   uint32_t width, uint32_t height,
                   MeshManager* mesh_manager) {
    memset(r3d, 0, sizeof(Render3D));

    r3d->device = device;
    r3d->physical_device = physical_device;
    r3d->render_pass = render_pass;
    r3d->mesh_manager = mesh_manager;

    // Create depth buffer
    if (!create_depth_buffer(r3d, width, height)) {
        fprintf(stderr, "Failed to create depth buffer\n");
        return false;
    }

    // Create default texture
    if (!create_default_texture(r3d)) {
        fprintf(stderr, "Failed to create default texture\n");
        render3d_cleanup(r3d);
        return false;
    }

    // Set default light
    r3d->light_direction = vec3_normalize(vec3(-0.5f, -1.0f, -0.3f));
    r3d->light_color = vec3(1.0f, 0.95f, 0.9f);
    r3d->light_intensity = 1.0f;
    r3d->ambient_intensity = 0.2f;

    r3d->initialized = true;
    printf("Render3D initialized (depth format: %d)\n", r3d->depth_format);

    return true;
}

void render3d_cleanup(Render3D* r3d) {
    if (!r3d) return;

    // Wait for device idle
    if (r3d->device) {
        vkDeviceWaitIdle(r3d->device);
    }

    // Cleanup depth buffer
    destroy_depth_buffer(r3d);

    // Cleanup default texture
    if (r3d->default_sampler) vkDestroySampler(r3d->device, r3d->default_sampler, NULL);
    if (r3d->default_texture_view) vkDestroyImageView(r3d->device, r3d->default_texture_view, NULL);
    if (r3d->default_texture_memory) vkFreeMemory(r3d->device, r3d->default_texture_memory, NULL);
    if (r3d->default_texture) vkDestroyImage(r3d->device, r3d->default_texture, NULL);

    // Cleanup pipeline
    if (r3d->pipeline) vkDestroyPipeline(r3d->device, r3d->pipeline, NULL);
    if (r3d->pipeline_layout) vkDestroyPipelineLayout(r3d->device, r3d->pipeline_layout, NULL);

    // Cleanup descriptors
    if (r3d->descriptor_pool) vkDestroyDescriptorPool(r3d->device, r3d->descriptor_pool, NULL);
    if (r3d->descriptor_set_layout) vkDestroyDescriptorSetLayout(r3d->device, r3d->descriptor_set_layout, NULL);
    if (r3d->texture_set_layout) vkDestroyDescriptorSetLayout(r3d->device, r3d->texture_set_layout, NULL);

    // Cleanup uniform buffers
    for (int i = 0; i < 3; i++) {
        if (r3d->ubo_buffers[i]) vkDestroyBuffer(r3d->device, r3d->ubo_buffers[i], NULL);
        if (r3d->ubo_memory[i]) vkFreeMemory(r3d->device, r3d->ubo_memory[i], NULL);
    }

    memset(r3d, 0, sizeof(Render3D));
}

bool render3d_resize(Render3D* r3d, uint32_t width, uint32_t height) {
    destroy_depth_buffer(r3d);
    return create_depth_buffer(r3d, width, height);
}

void render3d_begin_frame(Render3D* r3d,
                          Mat4 view, Mat4 projection, Vec3 camera_pos,
                          Vec3 light_dir, Vec3 light_color,
                          float light_intensity, float ambient) {
    r3d->view_matrix = view;
    r3d->projection_matrix = projection;
    r3d->camera_position = camera_pos;
    r3d->light_direction = light_dir;
    r3d->light_color = light_color;
    r3d->light_intensity = light_intensity;
    r3d->ambient_intensity = ambient;
    r3d->draw_count = 0;
}

void render3d_draw_mesh(Render3D* r3d, MeshHandle mesh, Mat4 transform, uint32_t material_id) {
    if (r3d->draw_count >= MAX_DRAW_CALLS) return;
    if (!mesh_is_valid(r3d->mesh_manager, mesh)) return;

    DrawCommand3D* cmd = &r3d->draw_queue[r3d->draw_count++];
    cmd->mesh = mesh;
    cmd->transform = transform;
    cmd->material_id = material_id;
}

void render3d_end_frame(Render3D* r3d, VkCommandBuffer cmd, uint32_t frame_index) {
    (void)cmd;
    (void)frame_index;
    // TODO: Bind pipeline, update UBOs, execute draw calls
    // For now, just reset draw count
    r3d->draw_count = 0;
}

VkImageView render3d_get_depth_view(Render3D* r3d) {
    return r3d->depth_view;
}

VkFormat render3d_get_depth_format(Render3D* r3d) {
    return r3d->depth_format;
}

