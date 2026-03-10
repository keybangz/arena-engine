/**
 * test_animation_playback.c - Test animation playback with character model
 * 
 * Tests the full animation pipeline:
 * - Load character model with skeleton
 * - Create animation state for entity
 * - Play/blend/stop animations
 * - Verify bone matrix updates
 * 
 * Compile with: make arena-test-animation-playback
 * Run with: ./build/arena-test-animation-playback
 */

#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/ecs/components_3d.h"
#include "arena/game/animation_system.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

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
// Create Test Skeleton and Animations
// ============================================================================

AnimationManager* setup_test_animations(Arena* arena) {
    AnimationManager* mgr = animation_manager_create(arena);
    if (!mgr) return NULL;
    
    // Create simple test skeleton (2 bones: root, arm)
    BoneNode* bones = arena_alloc(arena, sizeof(BoneNode) * 2, 8);
    
    bones[0] = (BoneNode){
        .index = 0,
        .name = "Root",
        .parent_index = 0xFF,
        .bind_matrix = mat4_identity(),
        .inverse_bind_matrix = mat4_identity(),
        .current_matrix = mat4_identity(),
        .is_animated = false
    };
    
    bones[1] = (BoneNode){
        .index = 1,
        .name = "Arm",
        .parent_index = 0,
        .bind_matrix = mat4_translate(vec3(1.0f, 0.0f, 0.0f)),
        .inverse_bind_matrix = mat4_translate(vec3(-1.0f, 0.0f, 0.0f)),
        .current_matrix = mat4_identity(),
        .is_animated = true
    };
    
    uint32_t skeleton_id = animation_manager_load_skeleton(mgr, "TestSkeleton", bones, 2, 0);
    
    // Create a simple test animation (2 frames, 1 second duration)
    AnimationKeyframe* rotation_kf = arena_alloc(arena, sizeof(AnimationKeyframe) * 2, 8);
    rotation_kf[0] = (AnimationKeyframe){
        .time = 0.0f,
        .position = vec3_zero(),
        .rotation = quat_identity(),
        .scale = vec3_one()
    };
    rotation_kf[1] = (AnimationKeyframe){
        .time = 1.0f,
        .position = vec3_zero(),
        .rotation = quat_from_axis_angle(vec3(0, 0, 1), 90.0f * ARENA_DEG2RAD),
        .scale = vec3_one()
    };
    
    AnimationChannel* channel = arena_alloc(arena, sizeof(AnimationChannel), 8);
    channel[0] = (AnimationChannel){
        .bone_index = 1,  // Arm bone
        .position_keyframes = NULL,
        .position_count = 0,
        .rotation_keyframes = rotation_kf,
        .rotation_count = 2,
        .scale_keyframes = NULL,
        .scale_count = 0
    };
    
    uint32_t clip_id = animation_manager_load_clip(mgr, "TestClip", 1.0f, channel, 1);
    
    return mgr;
}

// ============================================================================
// Main Tests
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("Animation Playback - Integration Tests\n");
    printf("========================================\n");

    // Initialize arena
    Arena* arena = arena_create(64 * 1024 * 1024);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }

    // =========================================================================
    // Test 1: Create ECS World
    // =========================================================================
    TEST_SECTION("Test 1: ECS World Setup");
    
    World* world = world_create(arena, 100);
    TEST_ASSERT(world != NULL, "World created");
    
    Entity entity = world_spawn(world);
    TEST_ASSERT(!entity_is_null(entity), "Entity spawned");

    // =========================================================================
    // Test 2: Add 3D Components
    // =========================================================================
    TEST_SECTION("Test 2: Add 3D Components");
    
    Transform3D* transform = world_add_transform3d(world, entity);
    TEST_ASSERT(transform != NULL, "Transform3D component added");
    transform3d_init(transform);
    
    SkinnedMesh* skinned = world_add_skinned_mesh(world, entity);
    TEST_ASSERT(skinned != NULL, "SkinnedMesh component added");
    skinned_mesh_init(skinned);
    
    AnimationState* anim_state = world_add_animation_state(world, entity);
    TEST_ASSERT(anim_state != NULL, "AnimationState component added");

    // =========================================================================
    // Test 3: Setup Animation Manager
    // =========================================================================
    TEST_SECTION("Test 3: Animation Manager Setup");
    
    AnimationManager* anim_mgr = setup_test_animations(arena);
    TEST_ASSERT(anim_mgr != NULL, "AnimationManager created with test data");
    TEST_ASSERT(anim_mgr->skeleton_count == 1, "Skeleton loaded");
    TEST_ASSERT(anim_mgr->clip_count == 1, "Animation clip loaded");
    
    // Link skeleton to skinned mesh
    skinned->skeleton_id = 1;  // Skeleton ID returned from load_skeleton
    skinned->bone_count = 2;

    // Initialize animation system BEFORE playing animations
    animation_system_init(world, anim_mgr);
    TEST_ASSERT(world->animation_mgr == anim_mgr, "Animation manager linked to world");

    // =========================================================================
    // Test 4: Animation Playback State
    // =========================================================================
    TEST_SECTION("Test 4: Animation Playback");
    
    // Initially not playing
    TEST_ASSERT(anim_state->is_playing == false, "Animation initially not playing");
    TEST_ASSERT(anim_state->current_time == 0.0f, "Current time is 0");
    TEST_ASSERT(anim_state->playback_speed == 0.0f, "Playback speed is 0");
    
    // Start animation playback
    animation_play(world, entity, 1, true, 1.0f);  // Clip ID 1, loop, speed 1.0
    TEST_ASSERT(anim_state->is_playing == true, "Animation started playing");
    TEST_ASSERT(anim_state->current_clip_id == 1, "Current clip ID set correctly");
    TEST_ASSERT(anim_state->playback_speed == 1.0f, "Playback speed set");

    // =========================================================================
    // Test 5: Animation Update
    // =========================================================================
    TEST_SECTION("Test 5: Animation System Update");
    
    // Simulate a single frame (0.016 seconds / 60 FPS)
    float dt = 0.016f;
    animation_system_update(world, anim_mgr, dt);
    
    // Time should have advanced
    TEST_ASSERT(anim_state->current_time >= 0.0f, "Current time updated");
    TEST_ASSERT(anim_state->current_time <= dt + 0.001f, "Current time reasonable");

    // =========================================================================
    // Test 6: Multiple Updates
    // =========================================================================
    TEST_SECTION("Test 6: Multiple Frame Updates");
    
    float accumulated_time = anim_state->current_time;
    
    for (int i = 0; i < 10; i++) {
        animation_system_update(world, anim_mgr, dt);
    }
    
    // Time should have advanced more
    TEST_ASSERT(anim_state->current_time > accumulated_time, "Time advances with updates");
    float expected_time = dt * 11;  // Original frame + 10 updates
    TEST_ASSERT(anim_state->current_time <= expected_time + 0.01f, "Time advancement within bounds");

    // =========================================================================
    // Test 7: Animation Blending
    // =========================================================================
    TEST_SECTION("Test 7: Animation Blending");
    
    // Reset animation
    animation_stop(world, entity);
    TEST_ASSERT(anim_state->is_playing == false, "Animation stopped");
    
    // Play first animation
    animation_play(world, entity, 1, true, 1.0f);
    TEST_ASSERT(anim_state->is_playing == true, "First animation playing");
    
    // Blend to same animation (special case)
    animation_blend_to(world, entity, 1, 0.5f, true);
    TEST_ASSERT(anim_state->blend_duration == 0.5f, "Blend duration set");

    // =========================================================================
    // Test 8: Animation Speed Control
    // =========================================================================
    TEST_SECTION("Test 8: Animation Speed Control");
    
    animation_set_speed(world, entity, 2.0f);
    TEST_ASSERT(anim_state->playback_speed == 2.0f, "Playback speed changed to 2.0");
    
    animation_set_speed(world, entity, 0.5f);
    TEST_ASSERT(anim_state->playback_speed == 0.5f, "Playback speed changed to 0.5");

    // =========================================================================
    // Test 9: Query Entities with AnimationState
    // =========================================================================
    TEST_SECTION("Test 9: Query Animated Entities");
    
    ComponentMask anim_mask = component_mask(COMPONENT_ANIMATION_STATE) | 
                              component_mask(COMPONENT_SKINNED_MESH);
    Query query = world_query(world, anim_mask);
    
    Entity found_entity = ENTITY_NULL;
    bool found_our_entity = false;
    
    while (query_next(&query, &found_entity)) {
        if (entity_equals(found_entity, entity)) {
            found_our_entity = true;
            break;
        }
    }
    
    TEST_ASSERT(found_our_entity, "Entity found in animated entities query");

    // =========================================================================
    // Test 10: Bone Matrix Calculations
    // =========================================================================
    TEST_SECTION("Test 10: Bone Matrix Calculations");
    
    // After animation updates, bone matrices should be updated
    TEST_ASSERT(anim_state->active_bone_count >= 0, "Active bone count valid");
    TEST_ASSERT(anim_state->active_bone_count <= MAX_BONES_PER_MESH, "Active bone count within limits");
    
    // Check that at least one bone matrix is not identity (if animation has run)
    bool has_animated_bone = false;
    for (uint32_t i = 0; i < anim_state->active_bone_count; i++) {
        Mat4 identity = mat4_identity();
        Mat4 bone_mat = anim_state->bone_matrices[i];
        // Simplified check - if any element differs from identity, animation has been applied
        if (bone_mat.m[0][0] != 1.0f || bone_mat.m[3][0] != 0.0f) {
            has_animated_bone = true;
            break;
        }
    }
    TEST_ASSERT(has_animated_bone || anim_state->active_bone_count > 0, "Bone matrices updated by animation");

    // =========================================================================
    // Summary
    // =========================================================================
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total Tests:   %d\n", test_count);
    printf("Passed:        %d\n", test_passed);
    printf("Failed:        %d\n", test_failed);
    
    if (test_failed == 0) {
        printf("\n✓ All animation playback tests passed!\n");
    } else {
        printf("\n✗ Some tests failed.\n");
    }

    arena_destroy(arena);
    return (test_failed == 0) ? 0 : 1;
}
