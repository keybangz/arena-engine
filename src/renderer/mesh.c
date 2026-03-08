#include "mesh.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// Helper: Find memory type for buffer allocation
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
    return UINT32_MAX; // Should never happen
}

// ============================================================================
// Helper: Create buffer with memory
// ============================================================================

static bool create_buffer(VkDevice device, VkPhysicalDevice physical_device,
                          VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer* buffer, VkDeviceMemory* memory) {
    // Create buffer
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
        return false;
    }
    
    // Get memory requirements
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);
    
    // Allocate memory
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type(physical_device, mem_reqs.memoryTypeBits, properties)
    };
    
    if (vkAllocateMemory(device, &alloc_info, NULL, memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, *buffer, NULL);
        return false;
    }
    
    vkBindBufferMemory(device, *buffer, *memory, 0);
    return true;
}

// ============================================================================
// Mesh Manager
// ============================================================================

void mesh_manager_init(MeshManager* mm, VkDevice device, VkPhysicalDevice physical_device) {
    memset(mm, 0, sizeof(MeshManager));
    mm->device = device;
    mm->physical_device = physical_device;
    mm->mesh_count = 1; // Slot 0 is reserved (invalid handle)
    mm->free_count = 0;
}

void mesh_manager_cleanup(MeshManager* mm) {
    for (uint32_t i = 1; i < mm->mesh_count; i++) {
        Mesh* mesh = &mm->meshes[i];
        if (mesh->is_uploaded) {
            vkDestroyBuffer(mm->device, mesh->vertex_buffer, NULL);
            vkDestroyBuffer(mm->device, mesh->index_buffer, NULL);
            vkFreeMemory(mm->device, mesh->vertex_memory, NULL);
            vkFreeMemory(mm->device, mesh->index_memory, NULL);
        }
    }
    memset(mm, 0, sizeof(MeshManager));
}

// ============================================================================
// Mesh Creation
// ============================================================================

static MeshHandle allocate_mesh_slot(MeshManager* mm) {
    // Try to reuse a free slot
    if (mm->free_count > 0) {
        return mm->free_list[--mm->free_count];
    }
    // Allocate new slot
    if (mm->mesh_count >= MAX_MESHES) {
        return MESH_HANDLE_INVALID;
    }
    return mm->mesh_count++;
}

static void compute_bounds(const float* positions, uint32_t vertex_count, 
                           uint32_t stride, Mesh* mesh) {
    if (vertex_count == 0) return;
    
    mesh->bounds_min = (Vec3){positions[0], positions[1], positions[2]};
    mesh->bounds_max = mesh->bounds_min;
    
    for (uint32_t i = 1; i < vertex_count; i++) {
        const float* p = positions + (i * stride / sizeof(float));
        if (p[0] < mesh->bounds_min.x) mesh->bounds_min.x = p[0];
        if (p[1] < mesh->bounds_min.y) mesh->bounds_min.y = p[1];
        if (p[2] < mesh->bounds_min.z) mesh->bounds_min.z = p[2];
        if (p[0] > mesh->bounds_max.x) mesh->bounds_max.x = p[0];
        if (p[1] > mesh->bounds_max.y) mesh->bounds_max.y = p[1];
        if (p[2] > mesh->bounds_max.z) mesh->bounds_max.z = p[2];
    }
    
    mesh->bounds_center = vec3_scale(vec3_add(mesh->bounds_min, mesh->bounds_max), 0.5f);
    mesh->bounds_radius = vec3_length(vec3_sub(mesh->bounds_max, mesh->bounds_center));
}

MeshHandle mesh_create(MeshManager* mm,
                       const Vertex3D* vertices, uint32_t vertex_count,
                       const uint32_t* indices, uint32_t index_count) {
    MeshHandle handle = allocate_mesh_slot(mm);
    if (handle == MESH_HANDLE_INVALID) return handle;
    
    Mesh* mesh = &mm->meshes[handle];
    memset(mesh, 0, sizeof(Mesh));
    
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->is_skinned = false;
    
    // Compute bounds
    compute_bounds((const float*)vertices, vertex_count, sizeof(Vertex3D), mesh);
    
    // Create vertex buffer
    VkDeviceSize vb_size = vertex_count * sizeof(Vertex3D);
    if (!create_buffer(mm->device, mm->physical_device, vb_size,
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &mesh->vertex_buffer, &mesh->vertex_memory)) {
        return MESH_HANDLE_INVALID;
    }
    
    // Upload vertex data
    void* data;
    vkMapMemory(mm->device, mesh->vertex_memory, 0, vb_size, 0, &data);
    memcpy(data, vertices, vb_size);
    vkUnmapMemory(mm->device, mesh->vertex_memory);
    
    // Create index buffer  
    VkDeviceSize ib_size = index_count * sizeof(uint32_t);
    if (!create_buffer(mm->device, mm->physical_device, ib_size,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &mesh->index_buffer, &mesh->index_memory)) {
        vkDestroyBuffer(mm->device, mesh->vertex_buffer, NULL);
        vkFreeMemory(mm->device, mesh->vertex_memory, NULL);
        return MESH_HANDLE_INVALID;
    }
    
    // Upload index data
    vkMapMemory(mm->device, mesh->index_memory, 0, ib_size, 0, &data);
    memcpy(data, indices, ib_size);
    vkUnmapMemory(mm->device, mesh->index_memory);
    
    mesh->is_uploaded = true;
    return handle;
}

MeshHandle mesh_create_skinned(MeshManager* mm,
                               const VertexSkinned* vertices, uint32_t vertex_count,
                               const uint32_t* indices, uint32_t index_count) {
    MeshHandle handle = allocate_mesh_slot(mm);
    if (handle == MESH_HANDLE_INVALID) return handle;

    Mesh* mesh = &mm->meshes[handle];
    memset(mesh, 0, sizeof(Mesh));

    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->is_skinned = true;

    compute_bounds((const float*)vertices, vertex_count, sizeof(VertexSkinned), mesh);

    VkDeviceSize vb_size = vertex_count * sizeof(VertexSkinned);
    if (!create_buffer(mm->device, mm->physical_device, vb_size,
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &mesh->vertex_buffer, &mesh->vertex_memory)) {
        return MESH_HANDLE_INVALID;
    }

    void* data;
    vkMapMemory(mm->device, mesh->vertex_memory, 0, vb_size, 0, &data);
    memcpy(data, vertices, vb_size);
    vkUnmapMemory(mm->device, mesh->vertex_memory);

    VkDeviceSize ib_size = index_count * sizeof(uint32_t);
    if (!create_buffer(mm->device, mm->physical_device, ib_size,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &mesh->index_buffer, &mesh->index_memory)) {
        vkDestroyBuffer(mm->device, mesh->vertex_buffer, NULL);
        vkFreeMemory(mm->device, mesh->vertex_memory, NULL);
        return MESH_HANDLE_INVALID;
    }

    vkMapMemory(mm->device, mesh->index_memory, 0, ib_size, 0, &data);
    memcpy(data, indices, ib_size);
    vkUnmapMemory(mm->device, mesh->index_memory);

    mesh->is_uploaded = true;
    return handle;
}

void mesh_destroy(MeshManager* mm, MeshHandle handle) {
    if (handle == MESH_HANDLE_INVALID || handle >= mm->mesh_count) return;

    Mesh* mesh = &mm->meshes[handle];
    if (mesh->is_uploaded) {
        vkDestroyBuffer(mm->device, mesh->vertex_buffer, NULL);
        vkDestroyBuffer(mm->device, mesh->index_buffer, NULL);
        vkFreeMemory(mm->device, mesh->vertex_memory, NULL);
        vkFreeMemory(mm->device, mesh->index_memory, NULL);
    }
    memset(mesh, 0, sizeof(Mesh));

    // Add to free list
    if (mm->free_count < MAX_MESHES) {
        mm->free_list[mm->free_count++] = handle;
    }
}

Mesh* mesh_get(MeshManager* mm, MeshHandle handle) {
    if (handle == MESH_HANDLE_INVALID || handle >= mm->mesh_count) return NULL;
    return &mm->meshes[handle];
}

bool mesh_is_valid(MeshManager* mm, MeshHandle handle) {
    if (handle == MESH_HANDLE_INVALID || handle >= mm->mesh_count) return false;
    return mm->meshes[handle].is_uploaded;
}

// ============================================================================
// Vertex Attribute Descriptions
// ============================================================================

VkVertexInputBindingDescription vertex3d_get_binding_description(void) {
    return (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(Vertex3D),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

void vertex3d_get_attribute_descriptions(VkVertexInputAttributeDescription* attrs, uint32_t* count) {
    *count = 4;

    // Position
    attrs[0] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex3D, position)
    };
    // Normal
    attrs[1] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex3D, normal)
    };
    // UV
    attrs[2] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex3D, uv)
    };
    // Tangent
    attrs[3] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 3,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Vertex3D, tangent)
    };
}

VkVertexInputBindingDescription vertex_skinned_get_binding_description(void) {
    return (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(VertexSkinned),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

void vertex_skinned_get_attribute_descriptions(VkVertexInputAttributeDescription* attrs, uint32_t* count) {
    *count = 6;

    attrs[0] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexSkinned, position)
    };
    attrs[1] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(VertexSkinned, normal)
    };
    attrs[2] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(VertexSkinned, uv)
    };
    attrs[3] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 3,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(VertexSkinned, tangent)
    };
    attrs[4] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 4,
        .format = VK_FORMAT_R8G8B8A8_UINT,
        .offset = offsetof(VertexSkinned, bone_ids)
    };
    attrs[5] = (VkVertexInputAttributeDescription){
        .binding = 0, .location = 5,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(VertexSkinned, bone_weights)
    };
}

// ============================================================================
// Primitive Mesh Creation
// ============================================================================

MeshHandle mesh_create_cube(MeshManager* mm) {
    // Unit cube vertices with normals and UVs
    static const Vertex3D vertices[] = {
        // Front face (z = 0.5)
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}},
        // Back face (z = -0.5)
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 1}, {-1, 0, 0, 1}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 1}, {-1, 0, 0, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 0}, {-1, 0, 0, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 0}, {-1, 0, 0, 1}},
        // Top face (y = 0.5)
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},
        // Bottom face (y = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}, {1, 0, 0, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}, {1, 0, 0, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 0}, {1, 0, 0, 1}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 0}, {1, 0, 0, 1}},
        // Right face (x = 0.5)
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 1}, {0, 0, 1, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 1}, {0, 0, 1, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 0}, {0, 0, 1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 0}, {0, 0, 1, 1}},
        // Left face (x = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 1}, {0, 0, -1, 1}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 1}, {0, 0, -1, 1}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 0}, {0, 0, -1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 0}, {0, 0, -1, 1}},
    };

    static const uint32_t indices[] = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    return mesh_create(mm, vertices, 24, indices, 36);
}

MeshHandle mesh_create_plane(MeshManager* mm, float width, float depth) {
    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    Vertex3D vertices[] = {
        {{-hw, 0, -hd}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},
        {{ hw, 0, -hd}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},
        {{ hw, 0,  hd}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}},
        {{-hw, 0,  hd}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}},
    };

    static const uint32_t indices[] = {0, 2, 1, 0, 3, 2};

    return mesh_create(mm, vertices, 4, indices, 6);
}

MeshHandle mesh_create_sphere(MeshManager* mm, uint32_t segments) {
    if (segments < 4) segments = 4;
    if (segments > 64) segments = 64;

    uint32_t rings = segments;
    uint32_t vertex_count = (rings + 1) * (segments + 1);
    uint32_t index_count = rings * segments * 6;

    Vertex3D* vertices = malloc(vertex_count * sizeof(Vertex3D));
    uint32_t* indices = malloc(index_count * sizeof(uint32_t));
    if (!vertices || !indices) {
        free(vertices);
        free(indices);
        return MESH_HANDLE_INVALID;
    }

    // Generate vertices
    uint32_t v = 0;
    for (uint32_t ring = 0; ring <= rings; ring++) {
        float phi = (float)ring / rings * 3.14159265f;
        for (uint32_t seg = 0; seg <= segments; seg++) {
            float theta = (float)seg / segments * 2.0f * 3.14159265f;

            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);

            vertices[v].position[0] = x * 0.5f;
            vertices[v].position[1] = y * 0.5f;
            vertices[v].position[2] = z * 0.5f;
            vertices[v].normal[0] = x;
            vertices[v].normal[1] = y;
            vertices[v].normal[2] = z;
            vertices[v].uv[0] = (float)seg / segments;
            vertices[v].uv[1] = (float)ring / rings;
            vertices[v].tangent[0] = -sinf(theta);
            vertices[v].tangent[1] = 0;
            vertices[v].tangent[2] = cosf(theta);
            vertices[v].tangent[3] = 1;
            v++;
        }
    }

    // Generate indices
    uint32_t i = 0;
    for (uint32_t ring = 0; ring < rings; ring++) {
        for (uint32_t seg = 0; seg < segments; seg++) {
            uint32_t current = ring * (segments + 1) + seg;
            uint32_t next = current + segments + 1;

            indices[i++] = current;
            indices[i++] = next;
            indices[i++] = current + 1;

            indices[i++] = current + 1;
            indices[i++] = next;
            indices[i++] = next + 1;
        }
    }

    MeshHandle handle = mesh_create(mm, vertices, vertex_count, indices, index_count);

    free(vertices);
    free(indices);

    return handle;
}

