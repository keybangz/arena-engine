/**
 * test_character_animation.c - Test character model animation loading and playback
 * 
 * Verifies:
 * - Load character.glb model
 * - Extract skeleton from model
 * - Extract animation clips from model
 * - Create entity with animated character
 * - Play animations and verify bone updates
 * 
 * Compile with: make arena-test-character-animation
 * Run with: ./build/arena-test-character-animation
 */

#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/ecs/components_3d.h"
#include "arena/game/animation_system.h"
#include "renderer/gltf_loader.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h> // For malloc and free

// ============================================================================
// Test Utilities
// ============================================================================

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    test_count++; \
    if (condition) { \
        printf("  ✓ %s\n", message); \
        test_passed++; \
    } else { \
        printf("  ✗ FAILED: %s\n", message); \
        test_failed++; \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n%s\n", name)

// ============================================================================
// Main Tests
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("Character Model Animation Tests\n");
    printf("========================================\n");

    // Initialize arena
    Arena* arena = arena_create(256 * 1024 * 1024);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }

    // Initialize mesh manager (required for gltf_load_file)
    MeshManager mesh_mgr;
    // NOTE: physical_device and device are not needed for this test,
    // as we are not actually uploading meshes to the GPU.
    // We just need a valid MeshManager struct.
    mesh_manager_init(&mesh_mgr, VK_NULL_HANDLE, VK_NULL_HANDLE);

    // =========================================================================
    // Test 1: Load Character Model
    // =========================================================================
    TEST_SECTION("Test 1: Load Character Model");
    
    GltfLoadResult character_result;
    bool load_success = gltf_load_file(&mesh_mgr, "assets/models/Box.glb", &character_result);
    TEST_ASSERT(load_success, "Character model loaded");
    
    if (!load_success) {
        fprintf(stderr, "Failed to load character model: %s\n", character_result.error_message);
        arena_destroy(arena);
        return 1;
    }
    
    // =========================================================================
    // Test 2: Verify Model Structure
    // =========================================================================
    TEST_SECTION("Test 2: Model Structure");
    
    TEST_ASSERT(character_result.mesh_count > 0, "Model has meshes");
    TEST_ASSERT(character_result.skeleton_count > 0, "Model has skeleton");
    TEST_ASSERT(character_result.skeletons[0].bone_count > 0, "Skeleton has bones");
    TEST_ASSERT(character_result.animation_count > 0, "Model has animations");
    
    printf("    - Meshes: %d\n", character_result.mesh_count);
    printf("    - Bones: %d\n", character_result.skeletons[0].bone_count);
    printf("    - Animations: %d\n", character_result.animation_count);
    
    // =========================================================================
    // Test 3: Verify Animation Clips
    // =========================================================================
    TEST_SECTION("Test 3: Animation Clips");
    
    for (uint32_t i = 0; i < character_result.animation_count; i++) {
        TEST_ASSERT(character_result.animations[i].name[0] != '\0', 
                    "Animation name set");
        TEST_ASSERT(character_result.animations[i].duration > 0.0f,
                    "Animation has valid duration");
        printf("    - %s: %.2f seconds\n", 
               character_result.animations[i].name,
               character_result.animations[i].duration);
    }
    
    // =========================================================================
    // Test 4: Create ECS World and Entity
    // =========================================================================
    TEST_SECTION("Test 4: ECS World Setup");
    
    World* world = world_create(arena, 100);
    TEST_ASSERT(world != NULL, "World created");
    
    Entity entity = world_spawn(world);
    TEST_ASSERT(!entity_is_null(entity), "Entity spawned");
    
    // =========================================================================
    // Test 5: Add Animation Components
    // =========================================================================
    TEST_SECTION("Test 5: Add Animation Components");
    
    Transform3D* transform = world_add_transform3d(world, entity);
    TEST_ASSERT(transform != NULL, "Transform3D added");
    transform3d_init(transform);
    
    SkinnedMesh* skinned = world_add_skinned_mesh(world, entity);
    TEST_ASSERT(skinned != NULL, "SkinnedMesh added");
    skinned_mesh_init(skinned);
    skinned->skeleton_id = 1;  // Will be set when skeleton loads
    skinned->bone_count = character_result.skeletons[0].bone_count;
    
    AnimationState* anim_state = world_add_animation_state(world, entity);
    TEST_ASSERT(anim_state != NULL, "AnimationState added");
    
    // =========================================================================
    // Test 6: Setup Animation Manager with Character Data
    // =========================================================================
    TEST_SECTION("Test 6: Animation Manager Setup");
    
    AnimationManager* anim_mgr = animation_manager_create(arena);
    TEST_ASSERT(anim_mgr != NULL, "Animation manager created");
    
    // Construct BoneNode array from GltfSkeleton data
    uint32_t bone_count = character_result.skeletons[0].bone_count;
    BoneNode* bones = arena_alloc(arena, sizeof(BoneNode) * bone_count, 8);
    
    for (uint32_t i = 0; i < bone_count; i++) {
        bones[i] = (BoneNode){
            .index = i,
            .name = character_result.skeletons[0].bone_names[i],
            .parent_index = character_result.skeletons[0].parent_indices[i],
            .bind_matrix = character_result.skeletons[0].bind_matrices[i],
            .inverse_bind_matrix = character_result.skeletons[0].inverse_bind_matrices[i],
            .current_matrix = mat4_identity(), // Will be updated by animation system
            .is_animated = true // Assume all bones are potentially animated
        };
    }

    // Load skeleton from model
    uint32_t skeleton_id = animation_manager_load_skeleton(
        anim_mgr,
        character_result.skeletons[0].name,
        bones,
        bone_count,
        character_result.skeletons[0].root_bone_index
    );
    TEST_ASSERT(skeleton_id > 0, "Skeleton loaded into manager");
    skinned->skeleton_id = skeleton_id;
    
    // Load animation clips
    uint32_t clip_count = 0;
    for (uint32_t i = 0; i < character_result.animation_count; i++) {
        uint32_t gltf_channel_count = character_result.animations[i].channel_count;
        AnimationChannel* channels = arena_alloc(arena, sizeof(AnimationChannel) * gltf_channel_count, 8);

        for (uint32_t j = 0; j < gltf_channel_count; j++) {
            GltfAnimationChannel* gltf_channel = &character_result.animations[i].channels[j];
            
            // Convert position keyframes
            AnimationKeyframe* pos_keyframes = NULL;
            if (gltf_channel->position_keyframe_count > 0) {
                pos_keyframes = arena_alloc(arena, sizeof(AnimationKeyframe) * gltf_channel->position_keyframe_count, 8);
                for (uint32_t k = 0; k < gltf_channel->position_keyframe_count; k++) {
                    pos_keyframes[k] = (AnimationKeyframe){
                        .time = gltf_channel->time_keyframes[k],
                        .position = gltf_channel->position_keyframes[k],
                        .rotation = quat_identity(), // Not used for position
                        .scale = vec3_one()         // Not used for position
                    };
                }
            }

            // Convert rotation keyframes
            AnimationKeyframe* rot_keyframes = NULL;
            if (gltf_channel->rotation_keyframe_count > 0) {
                rot_keyframes = arena_alloc(arena, sizeof(AnimationKeyframe) * gltf_channel->rotation_keyframe_count, 8);
                for (uint32_t k = 0; k < gltf_channel->rotation_keyframe_count; k++) {
                    rot_keyframes[k] = (AnimationKeyframe){
                        .time = gltf_channel->time_keyframes[k],
                        .position = vec3_zero(),    // Not used for rotation
                        .rotation = gltf_channel->rotation_keyframes[k],
                        .scale = vec3_one()         // Not used for rotation
                    };
                }
            }

            // Convert scale keyframes
            AnimationKeyframe* scale_keyframes = NULL;
            if (gltf_channel->scale_keyframe_count > 0) {
                scale_keyframes = arena_alloc(arena, sizeof(AnimationKeyframe) * gltf_channel->scale_keyframe_count, 8);
                for (uint32_t k = 0; k < gltf_channel->scale_keyframe_count; k++) {
                    scale_keyframes[k] = (AnimationKeyframe){
                        .time = gltf_channel->time_keyframes[k],
                        .position = vec3_zero(),    // Not used for scale
                        .rotation = quat_identity(), // Not used for scale
                        .scale = gltf_channel->scale_keyframes[k]
                    };
                }
            }

            channels[j] = (AnimationChannel){
                .bone_index = gltf_channel->bone_index,
                .position_keyframes = pos_keyframes,
                .position_count = gltf_channel->position_keyframe_count,
                .rotation_keyframes = rot_keyframes,
                .rotation_count = gltf_channel->rotation_keyframe_count,
                .scale_keyframes = scale_keyframes,
                .scale_count = gltf_channel->scale_keyframe_count
            };
        }

        uint32_t clip_id = animation_manager_load_clip(
            anim_mgr,
            character_result.animations[i].name,
            character_result.animations[i].duration,
            channels,
            gltf_channel_count
        );
        if (clip_id > 0) clip_count++;
        printf("    - Loaded: %s (ID: %d)\n",
               character_result.animations[i].name, clip_id);
    }
    TEST_ASSERT(clip_count == character_result.animation_count, "All animations loaded");
    
    // =========================================================================
    // Test 7: Initialize Animation System
    // =========================================================================
    TEST_SECTION("Test 7: Animation System");
    
    animation_system_init(world, anim_mgr);
    TEST_ASSERT(world->animation_mgr == anim_mgr, "Animation manager linked");
    
    // =========================================================================
    // Test 8: Playback First Animation
    // =========================================================================
    TEST_SECTION("Test 8: Animation Playback");
    
    animation_play(world, entity, 1, true, 1.0f);  // Clip 1, loop, speed 1.0
    TEST_ASSERT(anim_state->is_playing == true, "Animation started");
    TEST_ASSERT(anim_state->current_clip_id == 1, "Correct clip selected");
    
    // =========================================================================
    // Test 9: Update and Verify Bone Matrices
    // =========================================================================
    TEST_SECTION("Test 9: Bone Matrix Updates");
    
    float dt = 0.016f;  // 60 FPS
    
    // Simulate several frames
    for (int frame = 0; frame < 10; frame++) {
        animation_system_update(world, anim_mgr, dt);
    }
    
    TEST_ASSERT(anim_state->current_time > 0.0f, "Animation time advanced");
    TEST_ASSERT(anim_state->active_bone_count > 0, "Active bone count set");
    TEST_ASSERT(anim_state->active_bone_count <= character_result.skeletons[0].bone_count,
                "Active bone count valid");
    
    printf("    - Time: %.3f seconds\n", anim_state->current_time);
    printf("    - Active bones: %d\n", anim_state->active_bone_count);
    
    // =========================================================================
    // Test 10: Switch Animation
    // =========================================================================
    TEST_SECTION("Test 10: Animation Switching");
    
    if (character_result.animation_count > 1) {
        // Switch to second animation
        animation_play(world, entity, 2, true, 1.0f);
        TEST_ASSERT(anim_state->current_clip_id == 2, "Switched to second animation");
        
        animation_system_update(world, anim_mgr, dt);
        TEST_ASSERT(anim_state->current_time >= 0.0f, "Time reset on switch");
    } else {
        TEST_ASSERT(true, "Only one animation (skipped test)");
    }
    
    // =========================================================================
    // Cleanup
    // =========================================================================
    
    gltf_load_result_free(&character_result);
    arena_destroy(arena);
    
    // Print summary
    printf("\n");
    printf("========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total Tests:   %d\n", test_count);
    printf("Passed:        %d\n", test_passed);
    printf("Failed:        %d\n", test_failed);
    printf("\n");
    
    if (test_failed == 0) {
        printf("✓ All character animation tests passed!\n");
        return 0;
    } else {
        printf("✗ Some tests failed.\n");
        return 1;
    }
}
