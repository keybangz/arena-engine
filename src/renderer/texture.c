#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static uint32_t find_memory_type(VkPhysicalDevice pd, uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(pd, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props) return i;
    }
    return 0;
}

static void transition_image(TextureManager* tm, VkImage img, VkImageLayout old_l, VkImageLayout new_l) {
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL,
        tm->command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
    vkAllocateCommandBuffers(tm->device, &alloc, &cmd);
    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
    vkBeginCommandBuffer(cmd, &begin);
    
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, NULL, 0, 0,
        old_l, new_l, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, img,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    if (old_l == VK_IMAGE_LAYOUT_UNDEFINED) { barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; dst = VK_PIPELINE_STAGE_TRANSFER_BIT; }
    else if (old_l == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) { barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; src = VK_PIPELINE_STAGE_TRANSFER_BIT; }
    vkCmdPipelineBarrier(cmd, src, dst, 0, 0, NULL, 0, NULL, 1, &barrier);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &cmd, 0, NULL};
    vkQueueSubmit(tm->graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(tm->graphics_queue);
    vkFreeCommandBuffers(tm->device, tm->command_pool, 1, &cmd);
}

static void copy_buf_to_img(TextureManager* tm, VkBuffer buf, VkImage img, uint32_t w, uint32_t h) {
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL,
        tm->command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
    vkAllocateCommandBuffers(tm->device, &alloc, &cmd);
    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
    vkBeginCommandBuffer(cmd, &begin);
    VkBufferImageCopy region = {0, 0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0,0,0}, {w, h, 1}};
    vkCmdCopyBufferToImage(cmd, buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &cmd, 0, NULL};
    vkQueueSubmit(tm->graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(tm->graphics_queue);
    vkFreeCommandBuffers(tm->device, tm->command_pool, 1, &cmd);
}

static bool create_tex(TextureManager* tm, Texture* tex, const uint8_t* pixels, uint32_t w, uint32_t h) {
    VkDeviceSize size = w * h * 4;
    VkBuffer staging; VkDeviceMemory staging_mem;
    VkBufferCreateInfo buf_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL};
    vkCreateBuffer(tm->device, &buf_info, NULL, &staging);
    VkMemoryRequirements reqs; vkGetBufferMemoryRequirements(tm->device, staging, &reqs);
    VkMemoryAllocateInfo alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, reqs.size,
        find_memory_type(tm->physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
    vkAllocateMemory(tm->device, &alloc, NULL, &staging_mem);
    vkBindBufferMemory(tm->device, staging, staging_mem, 0);
    void* data; vkMapMemory(tm->device, staging_mem, 0, size, 0, &data); memcpy(data, pixels, size); vkUnmapMemory(tm->device, staging_mem);

    VkImageCreateInfo img_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
        {w, h, 1}, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, NULL, VK_IMAGE_LAYOUT_UNDEFINED};
    if (vkCreateImage(tm->device, &img_info, NULL, &tex->image) != VK_SUCCESS) {
        vkDestroyBuffer(tm->device, staging, NULL); vkFreeMemory(tm->device, staging_mem, NULL); return false;
    }
    vkGetImageMemoryRequirements(tm->device, tex->image, &reqs);
    alloc.allocationSize = reqs.size;
    alloc.memoryTypeIndex = find_memory_type(tm->physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(tm->device, &alloc, NULL, &tex->memory);
    vkBindImageMemory(tm->device, tex->image, tex->memory, 0);

    transition_image(tm, tex->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buf_to_img(tm, staging, tex->image, w, h);
    transition_image(tm, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(tm->device, staging, NULL); vkFreeMemory(tm->device, staging_mem, NULL);

    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0, tex->image, VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM, {0,0,0,0}, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    vkCreateImageView(tm->device, &view_info, NULL, &tex->view);

    VkSamplerCreateInfo samp = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, NULL, 0, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0, VK_FALSE, 1.0f, VK_FALSE, VK_COMPARE_OP_ALWAYS, 0, 1.0f, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE};
    vkCreateSampler(tm->device, &samp, NULL, &tex->sampler);

    VkDescriptorSetAllocateInfo ds_alloc = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, NULL, tm->descriptor_pool, 1, &tm->set_layout};
    vkAllocateDescriptorSets(tm->device, &ds_alloc, &tex->descriptor_set);
    VkDescriptorImageInfo img_desc = {tex->sampler, tex->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, tex->descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &img_desc, NULL, NULL};
    vkUpdateDescriptorSets(tm->device, 1, &write, 0, NULL);
    tex->width = w; tex->height = h; tex->mip_levels = 1; tex->is_valid = true;
    return true;
}

bool texture_manager_init(TextureManager* tm, VkDevice device, VkPhysicalDevice pd, VkCommandPool pool, VkQueue queue) {
    memset(tm, 0, sizeof(TextureManager));
    tm->device = device; tm->physical_device = pd; tm->command_pool = pool; tm->graphics_queue = queue;
    VkDescriptorSetLayoutBinding bind = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL};
    VkDescriptorSetLayoutCreateInfo layout = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, NULL, 0, 1, &bind};
    if (vkCreateDescriptorSetLayout(device, &layout, NULL, &tm->set_layout) != VK_SUCCESS) return false;
    VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES};
    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, NULL, 0, MAX_TEXTURES, 1, &ps};
    if (vkCreateDescriptorPool(device, &pool_info, NULL, &tm->descriptor_pool) != VK_SUCCESS) return false;
    uint8_t white[] = {255,255,255,255}, black[] = {0,0,0,255}, norm[] = {128,128,255,255};
    tm->white_texture = texture_load_memory(tm, white, 1, 1, 4);
    tm->black_texture = texture_load_memory(tm, black, 1, 1, 4);
    tm->normal_texture = texture_load_memory(tm, norm, 1, 1, 4);
    printf("TextureManager: Initialized (%d default textures)\n", 3);
    return true;
}

void texture_manager_cleanup(TextureManager* tm) {
    vkDeviceWaitIdle(tm->device);
    for (uint32_t i = 0; i < tm->texture_count; i++) {
        Texture* t = &tm->textures[i];
        if (t->is_valid) { vkDestroySampler(tm->device, t->sampler, NULL); vkDestroyImageView(tm->device, t->view, NULL);
            vkDestroyImage(tm->device, t->image, NULL); vkFreeMemory(tm->device, t->memory, NULL); }
    }
    if (tm->descriptor_pool) vkDestroyDescriptorPool(tm->device, tm->descriptor_pool, NULL);
    if (tm->set_layout) vkDestroyDescriptorSetLayout(tm->device, tm->set_layout, NULL);
}

TextureHandle texture_load(TextureManager* tm, const char* path) {
    int w, h, ch; stbi_set_flip_vertically_on_load(1);
    uint8_t* px = stbi_load(path, &w, &h, &ch, STBI_rgb_alpha);
    if (!px) { fprintf(stderr, "TextureManager: Failed to load '%s'\n", path); return TEXTURE_HANDLE_INVALID; }
    TextureHandle h2 = texture_load_memory(tm, px, w, h, 4); stbi_image_free(px);
    if (h2 != TEXTURE_HANDLE_INVALID) printf("TextureManager: Loaded '%s' (%dx%d)\n", path, w, h);
    return h2;
}

TextureHandle texture_load_memory(TextureManager* tm, const uint8_t* data, uint32_t w, uint32_t h, uint32_t ch) {
    if (tm->texture_count >= MAX_TEXTURES) return TEXTURE_HANDLE_INVALID; (void)ch;
    TextureHandle handle = tm->texture_count++;
    if (!create_tex(tm, &tm->textures[handle], data, w, h)) { tm->texture_count--; return TEXTURE_HANDLE_INVALID; }
    return handle;
}

Texture* texture_get(TextureManager* tm, TextureHandle h) { return h < tm->texture_count ? &tm->textures[h] : NULL; }
VkDescriptorSetLayout texture_manager_get_set_layout(TextureManager* tm) { return tm->set_layout; }
TextureHandle texture_get_white(TextureManager* tm) { return tm->white_texture; }
TextureHandle texture_get_black(TextureManager* tm) { return tm->black_texture; }
TextureHandle texture_get_normal(TextureManager* tm) { return tm->normal_texture; }

