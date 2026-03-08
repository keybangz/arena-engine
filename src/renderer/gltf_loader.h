/**
 * gltf_loader.h - glTF 2.0 Model Loading for Arena Engine
 * 
 * Uses cgltf library to load 3D models from .gltf and .glb files.
 * Supports:
 * - Static meshes with positions, normals, UVs, tangents
 * - Skinned meshes with bone weights
 * - Multiple meshes per file (returns array of handles)
 * - Bounding box computation
 * 
 * Usage:
 *   GltfLoadResult result;
 *   if (gltf_load_file(&mesh_manager, "model.glb", &result)) {
 *       // Use result.mesh_handles[0..result.mesh_count-1]
 *   }
 *   gltf_load_result_free(&result);  // Free temporary data
 */

#ifndef ARENA_GLTF_LOADER_H
#define ARENA_GLTF_LOADER_H

#include "mesh.h"
#include <stdbool.h>

// ============================================================================
// Result Structure
// ============================================================================

#define GLTF_MAX_MESHES_PER_FILE 64
#define GLTF_MAX_NAME_LENGTH 128

typedef struct GltfMeshInfo {
    MeshHandle  handle;
    char        name[GLTF_MAX_NAME_LENGTH];
    bool        is_skinned;
    uint32_t    vertex_count;
    uint32_t    index_count;
} GltfMeshInfo;

typedef struct GltfLoadResult {
    GltfMeshInfo meshes[GLTF_MAX_MESHES_PER_FILE];
    uint32_t     mesh_count;
    
    // Overall bounding box (union of all meshes)
    Vec3         bounds_min;
    Vec3         bounds_max;
    
    // Error handling
    bool         success;
    char         error_message[256];
} GltfLoadResult;

// ============================================================================
// Load Options
// ============================================================================

typedef struct GltfLoadOptions {
    bool    calculate_tangents;   // Generate tangents if not present
    bool    flip_uvs;             // Flip V coordinate (some tools export Y-up UVs)
    float   scale;                // Uniform scale to apply (default 1.0)
    bool    merge_meshes;         // Combine all primitives into single mesh
} GltfLoadOptions;

// Default options
static inline GltfLoadOptions gltf_load_options_default(void) {
    return (GltfLoadOptions){
        .calculate_tangents = true,
        .flip_uvs = false,
        .scale = 1.0f,
        .merge_meshes = false
    };
}

// ============================================================================
// API Functions
// ============================================================================

/**
 * Load a glTF file (.gltf or .glb)
 * 
 * @param mm        Mesh manager to create meshes in
 * @param filepath  Path to the .gltf or .glb file
 * @param result    Output: loaded mesh handles and info
 * @return          true on success, false on error (check result.error_message)
 */
bool gltf_load_file(MeshManager* mm, const char* filepath, GltfLoadResult* result);

/**
 * Load a glTF file with custom options
 */
bool gltf_load_file_ex(MeshManager* mm, const char* filepath, 
                       const GltfLoadOptions* options, GltfLoadResult* result);

/**
 * Load glTF from memory buffer
 */
bool gltf_load_memory(MeshManager* mm, const void* data, size_t size,
                      const GltfLoadOptions* options, GltfLoadResult* result);

/**
 * Free any temporary data in the load result
 * Note: Does NOT destroy the meshes - call mesh_destroy() for that
 */
void gltf_load_result_free(GltfLoadResult* result);

/**
 * Destroy all meshes from a load result
 */
void gltf_unload(MeshManager* mm, GltfLoadResult* result);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Check if a file is a valid glTF file (checks magic bytes for .glb)
 */
bool gltf_is_valid_file(const char* filepath);

/**
 * Get the file extension for a glTF path (.gltf or .glb)
 */
const char* gltf_get_extension(const char* filepath);

#endif // ARENA_GLTF_LOADER_H

