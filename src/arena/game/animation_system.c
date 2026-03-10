#include "animation_system.h"
#include "../alloc/arena_allocator.h"
#include <string.h>
#include <math.h>
#include <assert.h>

// Helper function for string duplication in arena
static char* arena_strdup_impl(Arena* arena, const char* str) {
    if (!arena || !str) return NULL;
    
    size_t len = strlen(str);
    char* copy = arena_alloc(arena, len + 1, 1);
    if (!copy) return NULL;
    
    memcpy(copy, str, len + 1); // Copy null terminator
    return copy;
}

// ============================================================================
// Animation Manager Implementation
// ============================================================================

AnimationManager* animation_manager_create(Arena* arena) {
    AnimationManager* mgr = arena_alloc(arena, sizeof(AnimationManager), 8);
    if (!mgr) return NULL;
    
    mgr->clips = arena_alloc(arena, sizeof(AnimationClip) * 32, 8);
    mgr->clip_count = 0;
    mgr->clip_capacity = 32;
    
    mgr->skeletons = arena_alloc(arena, sizeof(Skeleton) * 16, 8);
    mgr->skeleton_count = 0;
    mgr->skeleton_capacity = 16;
    
    mgr->alloc = arena;
    return mgr;
}

void animation_manager_destroy(AnimationManager* mgr) {
    // All allocations use the arena, so no individual frees needed
    (void)mgr; // Suppress unused parameter warning
}

uint32_t animation_manager_load_clip(AnimationManager* mgr,
                                    const char* name,
                                    float duration,
                                    AnimationChannel* channels,
                                    uint32_t channel_count) {
    if (!mgr || !name || mgr->clip_count >= mgr->clip_capacity) {
        return 0; // Invalid input or full
    }
    
    AnimationClip* clip = &mgr->clips[mgr->clip_count];
    
    clip->id = mgr->clip_count + 1; // 1-based ID (0 = invalid)
    clip->name = arena_strdup_impl(mgr->alloc, name);
    clip->duration = duration;
    clip->channel_count = channel_count;
    clip->channels = arena_alloc(mgr->alloc, sizeof(AnimationChannel) * channel_count, 8);
    clip->is_looping = true;
    clip->playback_speed = 1.0f;
    
    // Copy channel data
    memcpy(clip->channels, channels, sizeof(AnimationChannel) * channel_count);
    
    // Copy keyframe data
    for (uint32_t i = 0; i < channel_count; i++) {
        AnimationChannel* dst = &clip->channels[i];
        const AnimationChannel* src = &channels[i];
        
        // Copy position keyframes
        dst->position_count = src->position_count;
        dst->position_keyframes = arena_alloc(mgr->alloc, 
                                           sizeof(AnimationKeyframe) * dst->position_count, 8);
        memcpy(dst->position_keyframes, src->position_keyframes,
               sizeof(AnimationKeyframe) * dst->position_count);
        
        // Copy rotation keyframes
        dst->rotation_count = src->rotation_count;
        dst->rotation_keyframes = arena_alloc(mgr->alloc,
                                           sizeof(AnimationKeyframe) * dst->rotation_count, 8);
        memcpy(dst->rotation_keyframes, src->rotation_keyframes,
               sizeof(AnimationKeyframe) * dst->rotation_count);
        
        // Copy scale keyframes
        dst->scale_count = src->scale_count;
        dst->scale_keyframes = arena_alloc(mgr->alloc,
                                         sizeof(AnimationKeyframe) * dst->scale_count, 8);
        memcpy(dst->scale_keyframes, src->scale_keyframes,
               sizeof(AnimationKeyframe) * dst->scale_count);
    }
    
    uint32_t clip_id = clip->id;
    mgr->clip_count++;
    
    return clip_id;
}

uint32_t animation_manager_load_skeleton(AnimationManager* mgr,
                                         const char* name,
                                         BoneNode* bones,
                                         uint32_t bone_count,
                                         uint32_t root_bone_index) {
    if (!mgr || !name || mgr->skeleton_count >= mgr->skeleton_capacity) {
        return 0; // Invalid input or full
    }
    
    Skeleton* skeleton = &mgr->skeletons[mgr->skeleton_count];
    
    skeleton->id = mgr->skeleton_count + 1; // 1-based ID (0 = invalid)
    skeleton->name = arena_strdup_impl(mgr->alloc, name);
    skeleton->bone_count = bone_count;
    skeleton->bones = arena_alloc(mgr->alloc, sizeof(BoneNode) * bone_count, 8);
    skeleton->root_bone_index = root_bone_index;
    
    // Copy bone data
    memcpy(skeleton->bones, bones, sizeof(BoneNode) * bone_count);
    
    // Copy strings and matrices for each bone
    for (uint32_t i = 0; i < bone_count; i++) {
        BoneNode* bone = &skeleton->bones[i];
        const BoneNode* src = &bones[i];
        
        bone->name = arena_strdup_impl(mgr->alloc, src->name);
        bone->bind_matrix = src->bind_matrix;
        bone->inverse_bind_matrix = src->inverse_bind_matrix;
        bone->current_matrix = src->bind_matrix; // Start with bind pose
        bone->is_animated = src->is_animated;
    }
    
    uint32_t skeleton_id = skeleton->id;
    mgr->skeleton_count++;
    
    return skeleton_id;
}

const AnimationClip* animation_manager_get_clip(AnimationManager* mgr, uint32_t clip_id) {
    if (!mgr || clip_id == 0 || clip_id > mgr->clip_count) {
        return NULL;
    }
    return &mgr->clips[clip_id - 1]; // Convert to 0-based index
}

const Skeleton* animation_manager_get_skeleton(AnimationManager* mgr, uint32_t skeleton_id) {
    if (!mgr || skeleton_id == 0 || skeleton_id > mgr->skeleton_count) {
        return NULL;
    }
    return &mgr->skeletons[skeleton_id - 1]; // Convert to 0-based index
}

uint32_t animation_manager_get_clip_id(AnimationManager* mgr, const char* name) {
    if (!mgr || !name) return 0;
    
    for (uint32_t i = 0; i < mgr->clip_count; i++) {
        if (strcmp(mgr->clips[i].name, name) == 0) {
            return mgr->clips[i].id;
        }
    }
    return 0; // Not found
}

uint32_t animation_manager_get_skeleton_id(AnimationManager* mgr, const char* name) {
    if (!mgr || !name) return 0;
    
    for (uint32_t i = 0; i < mgr->skeleton_count; i++) {
        if (strcmp(mgr->skeletons[i].name, name) == 0) {
            return mgr->skeletons[i].id;
        }
    }
    return 0; // Not found
}

// ============================================================================
// Animation State Management
// ============================================================================

static AnimationState* animation_state_get(World* world, Entity entity) {
    // Get or create AnimationState component
    AnimationState* state = world_get_component(world, entity, COMPONENT_ANIMATION_STATE);
    if (!state) {
        state = world_add_component(world, entity, COMPONENT_ANIMATION_STATE);
        if (state) {
            state->entity = entity_make(entity_index(entity), 0);
            state->current_clip_id = 0;
            state->current_time = 0.0f;
            state->playback_speed = 1.0f;
            state->is_playing = false;
            state->is_looping = true;
            state->blend_from_clip_id = 0;
            state->blend_start_time = 0.0f;
            state->blend_duration = 0.3f;
            state->blend_factor = 1.0f;
            
            // Initialize bone matrices to identity
            for (uint32_t i = 0; i < MAX_BONES_PER_MESH; i++) {
                state->bone_matrices[i] = mat4_identity();
            }
            state->active_bone_count = 0;
        }
    }
    return state;
}

// ============================================================================
// Animation System Implementation
// ============================================================================

void animation_system_init(World* world, AnimationManager* mgr) {
    if (!world) return;
    
    // Store animation manager in world for public API access
    world->animation_mgr = mgr;
    
    // Register the AnimationState component type
    // This should be done during world creation in the future
}

void animation_system_update(World* world, AnimationManager* mgr, float dt) {
    if (!mgr || !world) return;
    
    // Query all entities with SkinnedMesh component
    Query query = world_query(world, component_mask(COMPONENT_SKINNED_MESH));
    Entity entity;
    
    while (query_next(&query, &entity)) {
        SkinnedMesh* skinned_mesh = world_get_component(world, entity, COMPONENT_SKINNED_MESH);
        AnimationState* anim_state = animation_state_get(world, entity);
        
        if (!skinned_mesh || !anim_state || !anim_state->is_playing) {
            continue;
        }
        
        const AnimationClip* current_clip = animation_manager_get_clip(mgr, anim_state->current_clip_id);
        const Skeleton* skeleton = animation_manager_get_skeleton(mgr, skinned_mesh->skeleton_id);
        
        if (!current_clip || !skeleton) {
            continue;
        }
        
        // Update animation time
        anim_state->current_time += dt * anim_state->playback_speed * current_clip->playback_speed;
        
        // Handle looping
        if (anim_state->current_time > current_clip->duration) {
            if (anim_state->is_looping) {
                anim_state->current_time = fmodf(anim_state->current_time, current_clip->duration);
            } else {
                anim_state->current_time = current_clip->duration;
                anim_state->is_playing = false;
            }
        }
        
        // Handle blending
        if (anim_state->blend_factor < 1.0f) {
            const AnimationClip* from_clip = animation_manager_get_clip(mgr, anim_state->blend_from_clip_id);
            
            if (from_clip) {
                float blend_progress = (anim_state->current_time - anim_state->blend_start_time) / anim_state->blend_duration;
                anim_state->blend_factor = animation_clamp(blend_progress, 0.0f, 1.0f);
                
                if (anim_state->blend_factor >= 1.0f) {
                    // Blending complete
                    anim_state->blend_from_clip_id = 0;
                    anim_state->blend_factor = 1.0f;
                    anim_state->blend_start_time = 0.0f;
                }
            }
        }
        
        // Sample animation and update bone matrices
        if (anim_state->blend_factor >= 1.0f) {
            // Single animation
            animation_sample_clip(current_clip, anim_state->current_time, 
                                anim_state->bone_matrices, &anim_state->active_bone_count);
        } else {
            // Blending between two animations
            const AnimationClip* from_clip = animation_manager_get_clip(mgr, anim_state->blend_from_clip_id);
            if (from_clip) {
                animation_blend_clips(from_clip, current_clip, anim_state->blend_factor,
                                    anim_state->current_time,
                                    anim_state->bone_matrices, &anim_state->active_bone_count);
            }
        }
        
        // Update SkinnedMesh bone matrices for GPU rendering
        if (skinned_mesh->bone_count <= anim_state->active_bone_count) {
            memcpy(skinned_mesh->bone_matrices, anim_state->bone_matrices,
                   sizeof(Mat4) * skinned_mesh->bone_count);
        } else {
            // Copy active bones, pad with identity for unused bones
            memcpy(skinned_mesh->bone_matrices, anim_state->bone_matrices,
                   sizeof(Mat4) * anim_state->active_bone_count);
            
            // Fill remaining with identity
            for (uint32_t i = anim_state->active_bone_count; i < skinned_mesh->bone_count; i++) {
                skinned_mesh->bone_matrices[i] = mat4_identity();
            }
        }
    }
    
    query_reset(&query);
}

// ============================================================================
// Public Animation API
// ============================================================================

void animation_play(World* world, Entity entity, uint32_t clip_id, bool loop, float speed) {
    AnimationState* state = animation_state_get(world, entity);
    if (!state) return;
    
    const AnimationClip* clip = animation_manager_get_clip(world->animation_mgr, clip_id);
    if (!clip) return;
    
    state->current_clip_id = clip_id;
    state->current_time = 0.0f;
    state->is_playing = true;
    state->is_looping = loop;
    state->playback_speed = speed;
    state->blend_from_clip_id = 0;
    state->blend_factor = 1.0f;
}

void animation_blend_to(World* world, Entity entity, uint32_t clip_id, float duration, bool loop) {
    AnimationState* state = animation_state_get(world, entity);
    if (!state) return;
    
    const AnimationClip* clip = animation_manager_get_clip(world->animation_mgr, clip_id);
    if (!clip) return;
    
    // Start blending from current animation
    state->blend_from_clip_id = state->current_clip_id;
    state->blend_start_time = state->current_time;
    state->blend_duration = duration;
    state->blend_factor = 0.0f;
    
    // Set new animation as target
    state->current_clip_id = clip_id;
    state->is_playing = true;
    state->is_looping = loop;
    state->playback_speed = 1.0f;
}

void animation_stop(World* world, Entity entity) {
    AnimationState* state = animation_state_get(world, entity);
    if (state) {
        state->is_playing = false;
        state->blend_from_clip_id = 0;
        state->blend_factor = 1.0f;
    }
}

const AnimationState* animation_get_state(World* world, Entity entity) {
    return world_get_component(world, entity, COMPONENT_ANIMATION_STATE);
}

void animation_set_speed(World* world, Entity entity, float speed) {
    AnimationState* state = animation_state_get(world, entity);
    if (state) {
        state->playback_speed = speed;
    }
}

const Mat4* animation_get_bone_matrix(World* world, Entity entity, uint32_t bone_index) {
    const AnimationState* state = animation_get_state(world, entity);
    if (!state || bone_index >= MAX_BONES_PER_MESH) {
        return NULL;
    }
    return &state->bone_matrices[bone_index];
}

// ============================================================================
// Animation Utilities
// ============================================================================

float animation_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Vec3 animation_lerp_vec3(Vec3 a, Vec3 b, float t) {
    return (Vec3){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

Mat4 animation_lerp_mat4(Mat4 a, Mat4 b, float t) {
    return (Mat4){{
        {animation_lerp(a.m[0][0], b.m[0][0], t), animation_lerp(a.m[0][1], b.m[0][1], t), animation_lerp(a.m[0][2], b.m[0][2], t), animation_lerp(a.m[0][3], b.m[0][3], t)},
        {animation_lerp(a.m[1][0], b.m[1][0], t), animation_lerp(a.m[1][1], b.m[1][1], t), animation_lerp(a.m[1][2], b.m[1][2], t), animation_lerp(a.m[1][3], b.m[1][3], t)},
        {animation_lerp(a.m[2][0], b.m[2][0], t), animation_lerp(a.m[2][1], b.m[2][1], t), animation_lerp(a.m[2][2], b.m[2][2], t), animation_lerp(a.m[2][3], b.m[2][3], t)},
        {animation_lerp(a.m[3][0], b.m[3][0], t), animation_lerp(a.m[3][1], b.m[3][1], t), animation_lerp(a.m[3][2], b.m[3][2], t), animation_lerp(a.m[3][3], b.m[3][3], t)}
    }};
}

Quat animation_slerp(Quat a, Quat b, float t) {
    // Ensure quaternions are normalized
    a = quat_normalize(a);
    b = quat_normalize(b);
    
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    // If dot product is negative, use -b to ensure shortest path
    if (dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }
    
    // If quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        Quat result = {
            animation_lerp(a.x, b.x, t),
            animation_lerp(a.y, b.y, t),
            animation_lerp(a.z, b.z, t),
            animation_lerp(a.w, b.w, t)
        };
        return quat_normalize(result);
    }
    
    // Calculate angle between quaternions
    float theta_0 = acosf(dot);
    float theta = theta_0 * t;
    float sin_theta = sinf(theta);
    float sin_theta_0 = sinf(theta_0);
    
    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    
    return (Quat){
        s0 * a.x + s1 * b.x,
        s0 * a.y + s1 * b.y,
        s0 * a.z + s1 * b.z,
        s0 * a.w + s1 * b.w
    };
}

float animation_clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void animation_sample_clip(const AnimationClip* clip, float time,
                          Mat4* bone_matrices, uint32_t* active_bone_count) {
    if (!clip || !bone_matrices || !active_bone_count) return;
    
    *active_bone_count = 0;
    
    for (uint32_t i = 0; i < clip->channel_count; i++) {
        const AnimationChannel* channel = &clip->channels[i];
        
        if (channel->position_count == 0 && channel->rotation_count == 0 && channel->scale_count == 0) {
            continue;
        }
        
        *active_bone_count = i + 1; // Track highest animated bone
        
        // Sample position
        Vec3 position = vec3_zero();
        if (channel->position_count > 0) {
            // Find keyframes around the current time
            uint32_t idx = 0;
            while (idx < channel->position_count - 1 && 
                   channel->position_keyframes[idx + 1].time <= time) {
                idx++;
            }
            
            if (idx < channel->position_count) {
                AnimationKeyframe* key1 = &channel->position_keyframes[idx];
                AnimationKeyframe* key2 = &channel->position_keyframes[idx + 1];
                
                if (key2->time > key1->time) {
                    float t = (time - key1->time) / (key2->time - key1->time);
                    position = animation_lerp_vec3(key1->position, key2->position, t);
                } else {
                    position = key1->position;
                }
            } else {
                position = channel->position_keyframes[channel->position_count - 1].position;
            }
        }
        
        // Sample rotation
        Quat rotation = quat_identity();
        if (channel->rotation_count > 0) {
            uint32_t idx = 0;
            while (idx < channel->rotation_count - 1 && 
                   channel->rotation_keyframes[idx + 1].time <= time) {
                idx++;
            }
            
            if (idx < channel->rotation_count) {
                AnimationKeyframe* key1 = &channel->rotation_keyframes[idx];
                AnimationKeyframe* key2 = &channel->rotation_keyframes[idx + 1];
                
                if (key2->time > key1->time) {
                    float t = (time - key1->time) / (key2->time - key1->time);
                    rotation = animation_slerp(key1->rotation, key2->rotation, t);
                } else {
                    rotation = key1->rotation;
                }
            } else {
                rotation = channel->rotation_keyframes[channel->rotation_count - 1].rotation;
            }
        }
        
        // Sample scale
        Vec3 scale = vec3_one();
        if (channel->scale_count > 0) {
            uint32_t idx = 0;
            while (idx < channel->scale_count - 1 && 
                   channel->scale_keyframes[idx + 1].time <= time) {
                idx++;
            }
            
            if (idx < channel->scale_count) {
                AnimationKeyframe* key1 = &channel->scale_keyframes[idx];
                AnimationKeyframe* key2 = &channel->scale_keyframes[idx + 1];
                
                if (key2->time > key1->time) {
                    float t = (time - key1->time) / (key2->time - key1->time);
                    scale = animation_lerp_vec3(key1->scale, key2->scale, t);
                } else {
                    scale = key1->scale;
                }
            } else {
                scale = channel->scale_keyframes[channel->scale_count - 1].scale;
            }
        }
        
        // Create transformation matrix TRS
        Mat4 translation = mat4_translate(position);
        Mat4 rotation_mat = quat_to_mat4(rotation);
        Mat4 scaling = mat4_scale(scale);
        
        bone_matrices[i] = mat4_mul(translation, mat4_mul(rotation_mat, scaling));
    }
}

void animation_blend_clips(const AnimationClip* from_clip, const AnimationClip* to_clip,
                           float blend_factor, float time,
                           Mat4* bone_matrices, uint32_t* active_bone_count) {
    if (!from_clip || !to_clip || !bone_matrices || !active_bone_count) return;
    
    // Sample both animations
    Mat4 from_matrices[MAX_BONES_PER_MESH];
    Mat4 to_matrices[MAX_BONES_PER_MESH];
    
    uint32_t from_count = 0;
    uint32_t to_count = 0;
    
    animation_sample_clip(from_clip, time, from_matrices, &from_count);
    animation_sample_clip(to_clip, time, to_matrices, &to_count);
    
    *active_bone_count = (from_count > to_count) ? from_count : to_count;
    
    // Blend corresponding bones
    for (uint32_t i = 0; i < *active_bone_count; i++) {
        // For now, just linear interpolate the matrices
        // In a production system, you'd want more sophisticated blending
        Mat4 from_mat = (i < from_count) ? from_matrices[i] : mat4_identity();
        Mat4 to_mat = (i < to_count) ? to_matrices[i] : mat4_identity();
        
        // Simple linear interpolation of matrix components
        bone_matrices[i] = animation_lerp_mat4(from_mat, to_mat, blend_factor);
    }
}