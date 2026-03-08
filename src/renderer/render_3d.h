#ifndef ARENA_RENDER_3D_H
#define ARENA_RENDER_3D_H

#include <vulkan/vulkan.h>
#include <stdbool.h>
#include "../arena/math/math.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

// ============================================================================
// 3D Rendering Pipeline for Arena Engine
// ============================================================================

// Maximum draw calls per frame
#define MAX_DRAW_CALLS 1024

// ----------------------------------------------------------------------------
// Uniform Buffer Structures (must match shader layout)
// ----------------------------------------------------------------------------

typedef struct MeshUBO {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 normalMatrix;
} MeshUBO;

typedef struct MaterialUBO {
    Vec4 baseColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float padding;
} MaterialUBO;

typedef struct LightUBO {
    Vec4 lightDirection;  // xyz = direction, w = intensity
    Vec4 lightColor;      // rgb = color, a = ambient intensity
    Vec4 cameraPosition;  // xyz = camera world position
} LightUBO;

// ----------------------------------------------------------------------------
// Draw Command
// ----------------------------------------------------------------------------

typedef struct DrawCommand3D {
    MeshHandle mesh;
    Mat4 transform;
    uint32_t material_id;
} DrawCommand3D;

// ----------------------------------------------------------------------------
// Render3D Context
// ----------------------------------------------------------------------------

typedef struct Render3D {
    // Vulkan handles (references, not owned)
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkRenderPass render_pass;
    VkCommandPool command_pool;
    VkQueue graphics_queue;

    // 3D Pipeline
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    // Descriptor sets
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSetLayout texture_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_sets[3]; // Per-frame
    VkDescriptorSet default_texture_set;
    
    // Uniform buffers (per-frame)
    VkBuffer ubo_buffers[3];
    VkDeviceMemory ubo_memory[3];
    void* ubo_mapped[3];
    
    // Depth buffer
    VkImage depth_image;
    VkDeviceMemory depth_memory;
    VkImageView depth_view;
    VkFormat depth_format;
    
    // Default white texture (1x1)
    VkImage default_texture;
    VkDeviceMemory default_texture_memory;
    VkImageView default_texture_view;
    VkSampler default_sampler;
    VkImageLayout default_texture_layout;

    // Manager references
    MeshManager* mesh_manager;
    TextureManager* texture_manager;
    MaterialManager* material_manager;

    // Draw queue
    DrawCommand3D draw_queue[MAX_DRAW_CALLS];
    uint32_t draw_count;
    
    // Current frame state
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Vec3 camera_position;
    Vec3 light_direction;
    Vec3 light_color;
    float light_intensity;
    float ambient_intensity;
    
    // State
    bool initialized;
} Render3D;

// ----------------------------------------------------------------------------
// Initialization / Cleanup
// ----------------------------------------------------------------------------

// Initialize 3D rendering context
bool render3d_init(Render3D* r3d,
                   VkDevice device,
                   VkPhysicalDevice physical_device,
                   VkRenderPass render_pass,
                   VkCommandPool command_pool,
                   VkQueue graphics_queue,
                   uint32_t width, uint32_t height,
                   MeshManager* mesh_manager,
                   TextureManager* texture_manager,
                   MaterialManager* material_manager);

// Cleanup 3D rendering context
void render3d_cleanup(Render3D* r3d);

// Recreate depth buffer on window resize
bool render3d_resize(Render3D* r3d, uint32_t width, uint32_t height);

// ----------------------------------------------------------------------------
// Frame Rendering
// ----------------------------------------------------------------------------

// Begin a new frame - set camera and lighting
void render3d_begin_frame(Render3D* r3d,
                          Mat4 view, Mat4 projection, Vec3 camera_pos,
                          Vec3 light_dir, Vec3 light_color,
                          float light_intensity, float ambient);

// Submit a mesh to be drawn
void render3d_draw_mesh(Render3D* r3d, MeshHandle mesh, Mat4 transform, uint32_t material_id);

// Execute all draw commands
void render3d_end_frame(Render3D* r3d, VkCommandBuffer cmd, uint32_t frame_index);

// ----------------------------------------------------------------------------
// Depth Buffer Access
// ----------------------------------------------------------------------------

VkImageView render3d_get_depth_view(Render3D* r3d);
VkFormat render3d_get_depth_format(Render3D* r3d);

#endif // ARENA_RENDER_3D_H

