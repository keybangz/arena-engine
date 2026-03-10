#ifndef ARENA_ANIMATION_SYSTEM_H
#define ARENA_ANIMATION_SYSTEM_H

#include "../ecs/ecs.h"
#include "../ecs/components_3d.h"
#include "../math/math.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Animation System - Arena Engine v0.9.0
// ============================================================================

// Animation keyframe data
typedef struct AnimationKeyframe {
    float time;              // Keyframe time in seconds
    Vec3 position;           // Position at this keyframe
    Quat rotation;           // Rotation at this keyframe  
    Vec3 scale;              // Scale at this keyframe
} AnimationKeyframe;

// Animation channel data for a single bone
typedef struct AnimationChannel {
    uint32_t bone_index;     // Which bone this channel animates
    
    // Position keyframes
    AnimationKeyframe* position_keyframes;
    uint32_t position_count;
    
    // Rotation keyframes  
    AnimationKeyframe* rotation_keyframes;
    uint32_t rotation_count;
    
    // Scale keyframes
    AnimationKeyframe* scale_keyframes;
    uint32_t scale_count;
} AnimationChannel;

// Complete animation clip
typedef struct AnimationClip {
    uint32_t id;             // Unique clip ID
    const char* name;        // Human-readable name
    float duration;          // Total animation duration in seconds
    
    uint32_t channel_count;  // Number of animated bones
    AnimationChannel* channels; // Array of animation channels
    
    // Playback state
    bool is_looping;         // Should animation loop?
    float playback_speed;    // Speed multiplier (1.0 = normal)
} AnimationClip;

// Skeleton hierarchy for bone relationships
typedef struct BoneNode {
    uint32_t index;          // Bone index in skeleton
    const char* name;        // Bone name
    uint32_t parent_index;  // Parent bone index (0xFF if root)
    
    // Bind pose transforms
    Mat4 bind_matrix;       // Model-space bind pose
    Mat4 inverse_bind_matrix; // Inverse for skinning
    
    // Animation state
    Mat4 current_matrix;    // Current transform in world space
    bool is_animated;       // Is this bone animated?
} BoneNode;

typedef struct Skeleton {
    uint32_t id;             // Unique skeleton ID
    const char* name;        // Skeleton name
    
    uint32_t bone_count;     // Total number of bones
    BoneNode* bones;         // Array of bones
    
    // Root bone (used for root motion)
    uint32_t root_bone_index;
} Skeleton;

// Animation manager state
typedef struct AnimationManager {
    AnimationClip* clips;   // Array of loaded animation clips
    uint32_t clip_count;    // Number of loaded clips
    uint32_t clip_capacity;  // Current capacity
    
    Skeleton* skeletons;     // Array of loaded skeletons
    uint32_t skeleton_count; // Number of loaded skeletons
    uint32_t skeleton_capacity; // Current capacity
    
    Arena* alloc;            // Memory allocator for clip/skeleton data
} AnimationManager;

// Animation playback state for entities (defined in ecs.h to avoid duplication)

// ============================================================================
// Animation Manager API
// ============================================================================

// Create animation manager
AnimationManager* animation_manager_create(Arena* arena);

// Destroy animation manager and all loaded data
void animation_manager_destroy(AnimationManager* mgr);

// Load animation clip from glTF data
uint32_t animation_manager_load_clip(AnimationManager* mgr, 
                                   const char* name, 
                                   float duration,
                                   AnimationChannel* channels, 
                                   uint32_t channel_count);

// Load skeleton from glTF data
uint32_t animation_manager_load_skeleton(AnimationManager* mgr,
                                        const char* name,
                                        BoneNode* bones,
                                        uint32_t bone_count,
                                        uint32_t root_bone_index);

// Get animation clip by ID
const AnimationClip* animation_manager_get_clip(AnimationManager* mgr, uint32_t clip_id);

// Get skeleton by ID
const Skeleton* animation_manager_get_skeleton(AnimationManager* mgr, uint32_t skeleton_id);

// Get animation clip by name (returns ID)
uint32_t animation_manager_get_clip_id(AnimationManager* mgr, const char* name);

// Get skeleton by name (returns ID)
uint32_t animation_manager_get_skeleton_id(AnimationManager* mgr, const char* name);

// ============================================================================
// Animation System API
// ============================================================================

// Initialize animation system
void animation_system_init(World* world, AnimationManager* mgr);

// Update animation system (call once per frame)
void animation_system_update(World* world, AnimationManager* mgr, float dt);

// Play animation on entity
void animation_play(World* world, Entity entity, uint32_t clip_id, bool loop, float speed);

// Blend to animation (crossfade)
void animation_blend_to(World* world, Entity entity, uint32_t clip_id, float duration, bool loop);

// Stop animation playback
void animation_stop(World* world, Entity entity);

// Get current animation state for entity
const AnimationState* animation_get_state(World* world, Entity entity);

// Set animation playback speed
void animation_set_speed(World* world, Entity entity, float speed);

// Get bone transform matrix (for debugging/in-game logic)
const Mat4* animation_get_bone_matrix(World* world, Entity entity, uint32_t bone_index);

// ============================================================================
// Animation Utilities
// ============================================================================

// Linear interpolation for scalars
float animation_lerp(float a, float b, float t);

// Linear interpolation for vectors
Vec3 animation_lerp_vec3(Vec3 a, Vec3 b, float t);

// Linear interpolation for matrices
Mat4 animation_lerp_mat4(Mat4 a, Mat4 b, float t);

// Spherical linear interpolation for quaternions
Quat animation_slerp(Quat a, Quat b, float t);

// Clamp value to range
float animation_clamp(float value, float min, float max);

// Sample animation clip at specific time
void animation_sample_clip(const AnimationClip* clip, float time, 
                          Mat4* bone_matrices, uint32_t* active_bone_count);

// Blend two animation clips
void animation_blend_clips(const AnimationClip* from_clip, const AnimationClip* to_clip,
                          float blend_factor, float time,
                          Mat4* bone_matrices, uint32_t* active_bone_count);

#endif // ARENA_ANIMATION_SYSTEM_H