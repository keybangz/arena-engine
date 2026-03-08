#ifndef ARENA_MESH_H
#define ARENA_MESH_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include "../arena/math/math.h"

// ============================================================================
// 3D Mesh Structures for Arena Engine
// ============================================================================

// Maximum vertices/indices per mesh
#define MAX_MESH_VERTICES (1024 * 64)   // 64K vertices
#define MAX_MESH_INDICES  (1024 * 128)  // 128K indices

// ----------------------------------------------------------------------------
// Vertex3D - Vertex format for 3D meshes
// ----------------------------------------------------------------------------

typedef struct Vertex3D {
    float position[3];      // xyz position
    float normal[3];        // Surface normal
    float uv[2];            // Texture coordinates
    float tangent[4];       // Tangent + handedness (for normal mapping)
} Vertex3D;

// Extended vertex for skeletal animation
typedef struct VertexSkinned {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[4];
    uint8_t bone_ids[4];    // Up to 4 bones per vertex
    float bone_weights[4];
} VertexSkinned;

// ----------------------------------------------------------------------------
// Mesh - GPU mesh data
// ----------------------------------------------------------------------------

typedef struct Mesh {
    // Vulkan resources
    VkBuffer        vertex_buffer;
    VkBuffer        index_buffer;
    VkDeviceMemory  vertex_memory;
    VkDeviceMemory  index_memory;
    
    // Mesh info
    uint32_t        vertex_count;
    uint32_t        index_count;
    
    // Bounding volume (for culling)
    Vec3            bounds_min;
    Vec3            bounds_max;
    Vec3            bounds_center;
    float           bounds_radius;
    
    // Flags
    bool            is_skinned;     // Uses VertexSkinned format
    bool            is_uploaded;    // Has been uploaded to GPU
} Mesh;

// ----------------------------------------------------------------------------
// MeshManager - Manages mesh loading and GPU resources
// ----------------------------------------------------------------------------

#define MAX_MESHES 256

typedef uint32_t MeshHandle;
#define MESH_HANDLE_INVALID 0

typedef struct MeshManager {
    Mesh            meshes[MAX_MESHES];
    uint32_t        mesh_count;
    
    // Free list for reusing slots
    uint32_t        free_list[MAX_MESHES];
    uint32_t        free_count;
    
    // Vulkan device reference (for buffer creation)
    VkDevice        device;
    VkPhysicalDevice physical_device;
} MeshManager;

// ----------------------------------------------------------------------------
// Mesh Manager Functions
// ----------------------------------------------------------------------------

// Initialize the mesh manager
void mesh_manager_init(MeshManager* mm, VkDevice device, VkPhysicalDevice physical_device);

// Cleanup the mesh manager (frees all GPU resources)
void mesh_manager_cleanup(MeshManager* mm);

// Create a mesh from vertex/index data (returns handle)
MeshHandle mesh_create(MeshManager* mm, 
                       const Vertex3D* vertices, uint32_t vertex_count,
                       const uint32_t* indices, uint32_t index_count);

// Create a skinned mesh
MeshHandle mesh_create_skinned(MeshManager* mm,
                               const VertexSkinned* vertices, uint32_t vertex_count,
                               const uint32_t* indices, uint32_t index_count);

// Destroy a mesh
void mesh_destroy(MeshManager* mm, MeshHandle handle);

// Get mesh data by handle
Mesh* mesh_get(MeshManager* mm, MeshHandle handle);

// Check if handle is valid
bool mesh_is_valid(MeshManager* mm, MeshHandle handle);

// ----------------------------------------------------------------------------
// Primitive Mesh Creation (for testing)
// ----------------------------------------------------------------------------

// Create a unit cube centered at origin
MeshHandle mesh_create_cube(MeshManager* mm);

// Create a unit sphere (16x16 segments)
MeshHandle mesh_create_sphere(MeshManager* mm, uint32_t segments);

// Create a plane (XZ plane, centered at origin)
MeshHandle mesh_create_plane(MeshManager* mm, float width, float depth);

// ----------------------------------------------------------------------------
// Vertex Attribute Descriptions (for Vulkan pipeline)
// ----------------------------------------------------------------------------

// Get binding description for Vertex3D
VkVertexInputBindingDescription vertex3d_get_binding_description(void);

// Get attribute descriptions for Vertex3D (position, normal, uv, tangent)
void vertex3d_get_attribute_descriptions(VkVertexInputAttributeDescription* attrs, uint32_t* count);

// Get binding description for VertexSkinned
VkVertexInputBindingDescription vertex_skinned_get_binding_description(void);

// Get attribute descriptions for VertexSkinned
void vertex_skinned_get_attribute_descriptions(VkVertexInputAttributeDescription* attrs, uint32_t* count);

#endif // ARENA_MESH_H

