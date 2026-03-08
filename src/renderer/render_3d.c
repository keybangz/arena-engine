#include "render_3d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Combined UBO structure for all uniforms (matches shader bindings)
// ============================================================================

typedef struct CombinedUBO {
    // Binding 0: MeshUBO
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 normalMatrix;

    // Binding 1: MaterialUBO
    Vec4 baseColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float padding1;

    // Binding 2: LightUBO
    Vec4 lightDirection;  // xyz = direction, w = intensity
    Vec4 lightColor;      // rgb = color, a = ambient intensity
    Vec4 cameraPosition;  // xyz = camera world position
} CombinedUBO;

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
// Image Layout Transition Helper
// ============================================================================

static void transition_image_layout_immediate(Render3D* r3d, VkImage image,
                                             VkImageLayout old_layout, VkImageLayout new_layout) {
    // Create temporary command buffer for layout transition
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = r3d->command_pool,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(r3d->device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags src_stage, dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED || old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
        barrier.srcAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0,
                        0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    vkQueueSubmit(r3d->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(r3d->graphics_queue);

    vkFreeCommandBuffers(r3d->device, r3d->command_pool, 1, &cmd);
}

// ============================================================================
// Shader Loading
// ============================================================================

static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(size);
    if (!data) { fclose(f); return NULL; }

    size_t read = fread(data, 1, size, f);
    fclose(f);

    if (read != size) { free(data); return NULL; }

    *out_size = size;
    return data;
}

static VkShaderModule create_shader_module(VkDevice device, const char* code, size_t size) {
    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const uint32_t*)code
    };
    VkShaderModule module;
    if (vkCreateShaderModule(device, &info, NULL, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

// ============================================================================
// Uniform Buffer Creation
// ============================================================================

static bool create_uniform_buffers(Render3D* r3d) {
    VkDeviceSize buffer_size = sizeof(CombinedUBO);

    for (int i = 0; i < 3; i++) {
        VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = buffer_size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        if (vkCreateBuffer(r3d->device, &buffer_info, NULL, &r3d->ubo_buffers[i]) != VK_SUCCESS) {
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(r3d->device, r3d->ubo_buffers[i], &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = find_memory_type(r3d->physical_device, mem_reqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        };

        if (vkAllocateMemory(r3d->device, &alloc_info, NULL, &r3d->ubo_memory[i]) != VK_SUCCESS) {
            return false;
        }

        vkBindBufferMemory(r3d->device, r3d->ubo_buffers[i], r3d->ubo_memory[i], 0);
        vkMapMemory(r3d->device, r3d->ubo_memory[i], 0, buffer_size, 0, &r3d->ubo_mapped[i]);
    }
    return true;
}

// ============================================================================
// Descriptor Set Layout & Pool
// ============================================================================

static bool create_descriptor_layouts(Render3D* r3d) {
    // Set 0: Combined UBO (MeshUBO + MaterialUBO + LightUBO as single buffer)
    VkDescriptorSetLayoutBinding ubo_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ubo_binding
    };

    if (vkCreateDescriptorSetLayout(r3d->device, &layout_info, NULL, &r3d->descriptor_set_layout) != VK_SUCCESS) {
        return false;
    }

    // Set 1: Texture sampler
    VkDescriptorSetLayoutBinding tex_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    layout_info.pBindings = &tex_binding;
    if (vkCreateDescriptorSetLayout(r3d->device, &layout_info, NULL, &r3d->texture_set_layout) != VK_SUCCESS) {
        return false;
    }

    return true;
}

static bool create_descriptor_pool(Render3D* r3d) {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 }  // 3 per-frame + 1 default
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 7,  // 3 UBO sets + 3 texture sets + 1 default texture
        .poolSizeCount = 2,
        .pPoolSizes = pool_sizes
    };

    if (vkCreateDescriptorPool(r3d->device, &pool_info, NULL, &r3d->descriptor_pool) != VK_SUCCESS) {
        return false;
    }

    // Allocate UBO descriptor sets
    VkDescriptorSetLayout layouts[3] = {
        r3d->descriptor_set_layout,
        r3d->descriptor_set_layout,
        r3d->descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = r3d->descriptor_pool,
        .descriptorSetCount = 3,
        .pSetLayouts = layouts
    };

    if (vkAllocateDescriptorSets(r3d->device, &alloc_info, r3d->descriptor_sets) != VK_SUCCESS) {
        return false;
    }

    // Update descriptor sets to point to UBO buffers
    for (int i = 0; i < 3; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = r3d->ubo_buffers[i],
            .offset = 0,
            .range = sizeof(CombinedUBO)
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = r3d->descriptor_sets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = &buffer_info
        };

        vkUpdateDescriptorSets(r3d->device, 1, &write, 0, NULL);
    }

    // Allocate and update default texture descriptor set
    VkDescriptorSetAllocateInfo tex_alloc = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = r3d->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &r3d->texture_set_layout
    };

    if (vkAllocateDescriptorSets(r3d->device, &tex_alloc, &r3d->default_texture_set) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorImageInfo image_info = {
        .sampler = r3d->default_sampler,
        .imageView = r3d->default_texture_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet tex_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = r3d->default_texture_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info
    };

    vkUpdateDescriptorSets(r3d->device, 1, &tex_write, 0, NULL);

    return true;
}

// ============================================================================
// Default Texture (1x1 white)
// ============================================================================

static bool create_default_texture(Render3D* r3d) {
    // Create 1x1 white image using GENERAL layout (simpler, no transition needed)
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {1, 1, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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

    // Store layout - we'll use GENERAL which is valid for both writing and sampling
    r3d->default_texture_layout = VK_IMAGE_LAYOUT_GENERAL;

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
// Graphics Pipeline
// ============================================================================

static bool create_3d_pipeline(Render3D* r3d) {
    // Load mesh shaders
    size_t vert_size, frag_size;
    char* vert_code = read_file("shaders/mesh.vert.spv", &vert_size);
    if (!vert_code) vert_code = read_file("../build/shaders/mesh.vert.spv", &vert_size);

    char* frag_code = read_file("shaders/mesh.frag.spv", &frag_size);
    if (!frag_code) frag_code = read_file("../build/shaders/mesh.frag.spv", &frag_size);

    if (!vert_code || !frag_code) {
        fprintf(stderr, "Render3D: Failed to load mesh shaders\n");
        free(vert_code);
        free(frag_code);
        return false;
    }

    VkShaderModule vert_module = create_shader_module(r3d->device, vert_code, vert_size);
    VkShaderModule frag_module = create_shader_module(r3d->device, frag_code, frag_size);
    free(vert_code);
    free(frag_code);

    if (!vert_module || !frag_module) {
        fprintf(stderr, "Render3D: Failed to create shader modules\n");
        if (vert_module) vkDestroyShaderModule(r3d->device, vert_module, NULL);
        if (frag_module) vkDestroyShaderModule(r3d->device, frag_module, NULL);
        return false;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main"
        }
    };

    // Vertex input: matches Vertex3D structure
    VkVertexInputBindingDescription binding = {
        .binding = 0,
        .stride = sizeof(Vertex3D),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription attributes[] = {
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex3D, position)},
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex3D, normal)},
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex3D, uv)},
        {.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Vertex3D, tangent)}
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding,
        .vertexAttributeDescriptionCount = 4,
        .pVertexAttributeDescriptions = attributes
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Viewport & scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // Color blending
    VkPipelineColorBlendAttachmentState blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment
    };

    // Dynamic states
    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states
    };

    // Pipeline layout
    VkDescriptorSetLayout set_layouts[] = { r3d->descriptor_set_layout, r3d->texture_set_layout };
    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 2,
        .pSetLayouts = set_layouts
    };

    if (vkCreatePipelineLayout(r3d->device, &layout_info, NULL, &r3d->pipeline_layout) != VK_SUCCESS) {
        vkDestroyShaderModule(r3d->device, vert_module, NULL);
        vkDestroyShaderModule(r3d->device, frag_module, NULL);
        return false;
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = r3d->pipeline_layout,
        .renderPass = r3d->render_pass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(r3d->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &r3d->pipeline);

    vkDestroyShaderModule(r3d->device, vert_module, NULL);
    vkDestroyShaderModule(r3d->device, frag_module, NULL);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Render3D: Failed to create graphics pipeline\n");
        return false;
    }

    printf("Render3D: 3D pipeline created successfully\n");
    return true;
}

// ============================================================================
// Public API
// ============================================================================

bool render3d_init(Render3D* r3d,
                   VkDevice device,
                   VkPhysicalDevice physical_device,
                   VkRenderPass render_pass,
                   VkCommandPool command_pool,
                   VkQueue graphics_queue,
                   uint32_t width, uint32_t height,
                   MeshManager* mesh_manager,
                   TextureManager* texture_manager,
                   MaterialManager* material_manager) {
    memset(r3d, 0, sizeof(Render3D));

    r3d->device = device;
    r3d->physical_device = physical_device;
    r3d->render_pass = render_pass;
    r3d->command_pool = command_pool;
    r3d->graphics_queue = graphics_queue;
    r3d->mesh_manager = mesh_manager;
    r3d->texture_manager = texture_manager;
    r3d->material_manager = material_manager;

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

    // Transition default texture to GENERAL layout (requires command pool)
    if (r3d->command_pool && r3d->graphics_queue) {
        transition_image_layout_immediate(r3d, r3d->default_texture,
                                         VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_GENERAL);
    }

    // Create descriptor layouts
    if (!create_descriptor_layouts(r3d)) {
        fprintf(stderr, "Failed to create descriptor layouts\n");
        render3d_cleanup(r3d);
        return false;
    }

    // Create uniform buffers
    if (!create_uniform_buffers(r3d)) {
        fprintf(stderr, "Failed to create uniform buffers\n");
        render3d_cleanup(r3d);
        return false;
    }

    // Create descriptor pool and sets
    if (!create_descriptor_pool(r3d)) {
        fprintf(stderr, "Failed to create descriptor pool\n");
        render3d_cleanup(r3d);
        return false;
    }

    // Create 3D pipeline
    if (!create_3d_pipeline(r3d)) {
        fprintf(stderr, "Failed to create 3D pipeline\n");
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
    if (r3d->draw_count == 0) return;
    if (!r3d->pipeline) return;

    // Bind 3D pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r3d->pipeline);

    // Bind default texture (set 1)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r3d->pipeline_layout,
                           1, 1, &r3d->default_texture_set, 0, NULL);

    // Process each draw call
    for (uint32_t i = 0; i < r3d->draw_count; i++) {
        DrawCommand3D* draw = &r3d->draw_queue[i];

        // Update UBO for this draw call
        CombinedUBO ubo;
        ubo.model = draw->transform;
        ubo.view = r3d->view_matrix;
        ubo.projection = r3d->projection_matrix;
        ubo.normalMatrix = draw->transform;  // OK for uniform scale

        // Get material properties
        Material* mat = r3d->material_manager ? material_get(r3d->material_manager, draw->material_id) : NULL;
        if (mat && mat->is_valid) {
            ubo.baseColor = mat->base_color;
            ubo.metallic = mat->metallic;
            ubo.roughness = mat->roughness;
            ubo.ambientOcclusion = mat->ambient_occlusion;
        } else {
            ubo.baseColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
            ubo.metallic = 0.0f;
            ubo.roughness = 0.5f;
            ubo.ambientOcclusion = 1.0f;
        }
        ubo.padding1 = 0.0f;

        // Lighting
        ubo.lightDirection = vec4(r3d->light_direction.x, r3d->light_direction.y,
                                  r3d->light_direction.z, r3d->light_intensity);
        ubo.lightColor = vec4(r3d->light_color.x, r3d->light_color.y,
                              r3d->light_color.z, r3d->ambient_intensity);
        ubo.cameraPosition = vec4(r3d->camera_position.x, r3d->camera_position.y,
                                  r3d->camera_position.z, 0.0f);

        // Copy UBO data (use frame_index for triple buffering)
        memcpy(r3d->ubo_mapped[frame_index % 3], &ubo, sizeof(CombinedUBO));

        // Bind UBO descriptor set (set 0)
        uint32_t dynamic_offset = 0;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r3d->pipeline_layout,
                               0, 1, &r3d->descriptor_sets[frame_index % 3], 1, &dynamic_offset);

        // Bind texture (set 1) from material
        if (mat && mat->is_valid && r3d->texture_manager) {
            Texture* tex = texture_get(r3d->texture_manager, mat->albedo_texture);
            if (tex && tex->is_valid) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r3d->pipeline_layout,
                                       1, 1, &tex->descriptor_set, 0, NULL);
            } else {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r3d->pipeline_layout,
                                       1, 1, &r3d->default_texture_set, 0, NULL);
            }
        }

        // Get mesh data and bind buffers
        Mesh* mesh = mesh_get(r3d->mesh_manager, draw->mesh);
        if (!mesh || !mesh->is_uploaded) continue;

        VkBuffer vertex_buffers[] = { mesh->vertex_buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(cmd, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed
        vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
    }

    r3d->draw_count = 0;
}

VkImageView render3d_get_depth_view(Render3D* r3d) {
    return r3d->depth_view;
}

VkFormat render3d_get_depth_format(Render3D* r3d) {
    return r3d->depth_format;
}

