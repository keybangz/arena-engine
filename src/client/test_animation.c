/**
 * test_animation.c - Animation System Unit Tests
 * 
 * Tests the animation system core functionality:
 * - Skeleton loading and hierarchy
 * - Animation clip loading
 * - Bone matrix computations
 * - Playback state management
 * 
 * Compile with: make arena-test-animation
 * Run with: ./build/arena-test-animation
 */

#include "arena/arena.h"
#include "arena/math/math.h"
#include "arena/ecs/ecs.h"
#include "arena/ecs/components_3d.h"
#include "arena/game/animation_system.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
    printf("Animation System - Unit Tests\n");
    printf("========================================\n");

    // Initialize arena allocator
    Arena* arena = arena_create(64 * 1024 * 1024);  // 64 MB
    if (!arena) {
        fprintf(stderr, "Failed to create arena allocator\n");
        return 1;
    }
    // =========================================================================
    // Test 1: AnimationManager Creation
    // =========================================================================
    TEST_SECTION("Test 1: AnimationManager Creation");
    
    AnimationManager* anim_mgr = animation_manager_create(arena);
    TEST_ASSERT(anim_mgr != NULL, "AnimationManager created successfully");
    TEST_ASSERT(anim_mgr->clip_count == 0, "Initial clip count is 0");
    TEST_ASSERT(anim_mgr->skeleton_count == 0, "Initial skeleton count is 0");
    TEST_ASSERT(anim_mgr->alloc == arena, "Arena allocator set correctly");

    // =========================================================================
    // Test 2: Skeleton Loading
    // =========================================================================
    TEST_SECTION("Test 2: Skeleton Loading");
    
    // Create test skeleton with 3 bones: root, arm, hand
    BoneNode* test_bones = arena_alloc(arena, sizeof(BoneNode) * 3, 8);
    TEST_ASSERT(test_bones != NULL, "Bone array allocated");
    
    test_bones[0] = (BoneNode){
        .index = 0,
        .name = "Root",
        .parent_index = 0xFF,
        .bind_matrix = mat4_identity(),
        .inverse_bind_matrix = mat4_identity(),
        .current_matrix = mat4_identity(),
        .is_animated = false
    };
    
    test_bones[1] = (BoneNode){
        .index = 1,
        .name = "Arm",
        .parent_index = 0,
        .bind_matrix = mat4_translate(vec3(1.0f, 0.0f, 0.0f)),
        .inverse_bind_matrix = mat4_translate(vec3(-1.0f, 0.0f, 0.0f)),
        .current_matrix = mat4_identity(),
        .is_animated = true
    };
    
    test_bones[2] = (BoneNode){
        .index = 2,
        .name = "Hand",
        .parent_index = 1,
        .bind_matrix = mat4_translate(vec3(1.0f, 0.0f, 0.0f)),
        .inverse_bind_matrix = mat4_translate(vec3(-1.0f, 0.0f, 0.0f)),
        .current_matrix = mat4_identity(),
        .is_animated = true
    };
    
    uint32_t skeleton_id = animation_manager_load_skeleton(anim_mgr, "TestSkeleton", 
                                                           test_bones, 3, 0);
    TEST_ASSERT(skeleton_id > 0, "Skeleton loaded successfully (ID > 0)");
    TEST_ASSERT(anim_mgr->skeleton_count == 1, "Skeleton count incremented to 1");
    
    // =========================================================================
    // Test 3: Skeleton Retrieval
    // =========================================================================
    TEST_SECTION("Test 3: Skeleton Retrieval");
    
    const Skeleton* loaded_skel = animation_manager_get_skeleton(anim_mgr, skeleton_id);
    TEST_ASSERT(loaded_skel != NULL, "Skeleton retrieved by ID");
    TEST_ASSERT(loaded_skel->id == skeleton_id, "Retrieved skeleton has correct ID");
    TEST_ASSERT(loaded_skel->bone_count == 3, "Retrieved skeleton has 3 bones");
    TEST_ASSERT(strcmp(loaded_skel->name, "TestSkeleton") == 0, "Skeleton name correct");
    TEST_ASSERT(loaded_skel->root_bone_index == 0, "Root bone index is 0");
    
    // =========================================================================
    // Test 4: Bone Hierarchy
    // =========================================================================
    TEST_SECTION("Test 4: Bone Hierarchy");
    
    if (loaded_skel) {
        const BoneNode* root = &loaded_skel->bones[0];
        const BoneNode* arm = &loaded_skel->bones[1];
        const BoneNode* hand = &loaded_skel->bones[2];
        
        TEST_ASSERT(root->parent_index == 0xFF, "Root bone has no parent (0xFF)");
        TEST_ASSERT(arm->parent_index == 0, "Arm bone's parent is root (0)");
        TEST_ASSERT(hand->parent_index == 1, "Hand bone's parent is arm (1)");
        
        TEST_ASSERT(strcmp(root->name, "Root") == 0, "Root bone name correct");
        TEST_ASSERT(strcmp(arm->name, "Arm") == 0, "Arm bone name correct");
        TEST_ASSERT(strcmp(hand->name, "Hand") == 0, "Hand bone name correct");
        
        TEST_ASSERT(arm->is_animated == true, "Arm bone marked as animated");
        TEST_ASSERT(hand->is_animated == true, "Hand bone marked as animated");
        TEST_ASSERT(root->is_animated == false, "Root bone marked as not animated");
    }

    // =========================================================================
    // Test 5: Bone Matrices
    // =========================================================================
    TEST_SECTION("Test 5: Bone Matrices");
    
    if (loaded_skel) {
        const BoneNode* arm = &loaded_skel->bones[1];
        
        // Arm should have been translated by 1.0 on X axis (bind pose)
        float arm_translate_x = arm->bind_matrix.m[3][0];
        TEST_ASSERT(arm_translate_x == 1.0f, "Arm bind matrix translation X = 1.0");
        
        // Inverse bind matrix should be the inverse translation
        float arm_inv_translate_x = arm->inverse_bind_matrix.m[3][0];
        TEST_ASSERT(arm_inv_translate_x == -1.0f, "Arm inverse bind matrix translation X = -1.0");
    }

    // =========================================================================
    // Test 6: Skeleton Name Lookup
    // =========================================================================
    TEST_SECTION("Test 6: Skeleton Name Lookup");
    
    uint32_t looked_up_id = animation_manager_get_skeleton_id(anim_mgr, "TestSkeleton");
    TEST_ASSERT(looked_up_id == skeleton_id, "Skeleton lookup by name returns correct ID");
    
    uint32_t invalid_lookup = animation_manager_get_skeleton_id(anim_mgr, "NonExistent");
    TEST_ASSERT(invalid_lookup == 0, "Invalid skeleton lookup returns 0");

    // =========================================================================
    // Test 7: Multiple Skeletons
    // =========================================================================
    TEST_SECTION("Test 7: Multiple Skeletons");
    
    BoneNode* test_bones2 = arena_alloc(arena, sizeof(BoneNode), 8);
    test_bones2[0] = (BoneNode){
        .index = 0,
        .name = "SingleBone",
        .parent_index = 0xFF,
        .bind_matrix = mat4_identity(),
        .inverse_bind_matrix = mat4_identity(),
        .current_matrix = mat4_identity(),
        .is_animated = false
    };
    
    uint32_t skeleton_id2 = animation_manager_load_skeleton(anim_mgr, "SingleBoneSkeleton",
                                                            test_bones2, 1, 0);
    TEST_ASSERT(skeleton_id2 > 0, "Second skeleton loaded successfully");
    TEST_ASSERT(skeleton_id2 != skeleton_id, "Second skeleton has different ID");
    TEST_ASSERT(anim_mgr->skeleton_count == 2, "Skeleton count is now 2");
    
    // Verify both skeletons are still accessible
    const Skeleton* skel1 = animation_manager_get_skeleton(anim_mgr, skeleton_id);
    const Skeleton* skel2 = animation_manager_get_skeleton(anim_mgr, skeleton_id2);
    TEST_ASSERT(skel1 != NULL && skel2 != NULL, "Both skeletons accessible after loading second");
    TEST_ASSERT(skel1->bone_count == 3 && skel2->bone_count == 1, "Both skeletons have correct bone counts");

    // =========================================================================
    // Test 8: ECS World Integration
    // =========================================================================
    TEST_SECTION("Test 8: ECS World Integration");
    
    World* world = world_create(arena, 100);
    TEST_ASSERT(world != NULL, "ECS world created");
    
    Entity test_entity = world_spawn(world);
    TEST_ASSERT(!entity_is_null(test_entity), "Entity spawned");
    
    AnimationState* anim_state = world_add_animation_state(world, test_entity);
    TEST_ASSERT(anim_state != NULL, "AnimationState component added to entity");
    
    AnimationState* retrieved_state = world_get_animation_state(world, test_entity);
    TEST_ASSERT(retrieved_state != NULL, "AnimationState component retrieved from entity");
    TEST_ASSERT(retrieved_state == anim_state, "Retrieved component is the same instance");
    
    // Check initial animation state values
    TEST_ASSERT(anim_state->is_playing == false, "Initial is_playing is false");
    TEST_ASSERT(anim_state->current_time == 0.0f, "Initial current_time is 0");
    TEST_ASSERT(anim_state->playback_speed == 0.0f, "Initial playback_speed is 0");

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
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed.\n");
    }

    arena_destroy(arena);
    return (test_failed == 0) ? 0 : 1;
}
