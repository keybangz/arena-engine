/**
 * gltf_loader.h - glTF 2.0 Model Loading for Arena Engine
 * 
 * Uses cgltf library to load 3D models from .gltf and .glb files.
 * Supports:
 * - Static meshes with positions, normals, UVs, tangents
 * - Skinned meshes with bone weights
 * - Multiple meshes per file (returns array of handles)
 * - Bounding box computation
 * - Animation loading and skeleton extraction
 * 
 * Usage:
 *   GltfLoadResult result;
 *   if (gltf_load_file(&mesh_manager, "model.glb", &result)) {
 *       // Use result.mesh_handles[0..result.mesh_count-1]
 *       // Load animations if available
 *       uint32_t skeleton_id;
 *       int32_t anim_count = gltf_load_animations(&animation_manager, &result, &skeleton_id);
 *       if (anim_count > 0) {
 *           printf("Loaded %d animations\n", anim_count);
 *       }
 *   }
 *   gltf_load_result_free(&result);  // Free temporary data
 */

#ifndef ARENA_GLTF_LOADER_H
#define ARENA_GLTF_LOADER_H

#include "mesh.h"
#include "../arena/game/animation_system.h"
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

// ============================================================================
// Animation Support
// ============================================================================

#define GLTF_MAX_ANIMATIONS_PER_FILE 16
#define GLTF_MAX_CHANNELS_PER_ANIMATION 64
#define GLTF_MAX_KEYFRAMES_PER_CHANNEL 256

typedef struct GltfAnimationChannel {
    uint32_t bone_index;          // Which bone this channel animates
    
    // Keyframe data
    float* time_keyframes;        // Time values for each keyframe
    uint32_t time_keyframe_count;
    
    Vec3* position_keyframes;    // Position at each keyframe
    uint32_t position_keyframe_count;
    
    Quat* rotation_keyframes;    // Rotation at each keyframe  
    uint32_t rotation_keyframe_count;
    
    Vec3* scale_keyframes;        // Scale at each keyframe
    uint32_t scale_keyframe_count;
} GltfAnimationChannel;

typedef struct GltfAnimationClip {
    char name[GLTF_MAX_NAME_LENGTH];
    float duration;               // Total animation duration in seconds
    
    uint32_t channel_count;      // Number of animation channels
    GltfAnimationChannel* channels; // Array of channels
    
    bool is_looping;             // Should animation loop?
    float playback_speed;        // Speed multiplier (1.0 = normal)
} GltfAnimationClip;

typedef struct GltfSkeleton {
    char name[GLTF_MAX_NAME_LENGTH];
    
    uint32_t bone_count;         // Total number of bones
    uint32_t root_bone_index;   // Index of root bone
    
    // Bone data
    char** bone_names;          // Bone names
    Mat4* bind_matrices;        // Model-space bind pose matrices
    Mat4* inverse_bind_matrices; // Inverse bind matrices for skinning
    uint32_t* parent_indices;   // Parent bone indices (0xFF if root)
} GltfSkeleton;

typedef struct GltfLoadResult {
    GltfMeshInfo meshes[GLTF_MAX_MESHES_PER_FILE];
    uint32_t     mesh_count;
    
    // Animation data
    GltfAnimationClip animations[GLTF_MAX_ANIMATIONS_PER_FILE];
    uint32_t animation_count;
    
    // Skeleton data  
    GltfSkeleton skeletons[GLTF_MAX_ANIMATIONS_PER_FILE]; // Usually one skeleton per model
    uint32_t skeleton_count;
    
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

/**
 * Load animation data from glTF result into AnimationManager
 * 
 * @param mgr           Animation manager to load animations into
 * @param result        glTF load result containing animation data
 * @param skeleton_id   Output: skeleton ID that was loaded
 * @return              Number of animation clips loaded, or -1 on error
 */
int32_t gltf_load_animations(AnimationManager* mgr, const GltfLoadResult* result, uint32_t* skeleton_id);

/**
 * Get skeleton by name from glTF result
 * 
 * @param result        glTF load result 
 * @param name          Skeleton name to search for
 * @return              Index of skeleton, or -1 if not found
 */
int32_t gltf_find_skeleton(const GltfLoadResult* result, const char* name);

/**
 * Get animation clip by name from glTF result
 * 
 * @param result        glTF load result
 * @param name          Animation name to search for  
 * @return              Index of animation, or -1 if not found
 */
int32_t gltf_find_animation(const GltfLoadResult* result, const char* name);

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

