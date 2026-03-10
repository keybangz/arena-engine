#ifndef ARENA_ECS_H
#define ARENA_ECS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Math includes
#include "../math/math.h"

// Forward declarations
typedef struct Arena Arena;
typedef struct AnimationManager AnimationManager; // Forward declare AnimationManager

// Maximum bones per skinned mesh (copied from components_3d.h to avoid circular include)
#define MAX_BONES_PER_MESH 64

// ============================================================================
// Component Types
// ============================================================================

// Component type ID (max 64 component types for bitmask)
typedef uint32_t ComponentType;

#define MAX_COMPONENT_TYPES 64

// Component mask for archetype matching
typedef uint64_t ComponentMask;

#define COMPONENT_MASK_NONE  ((ComponentMask)0)
#define COMPONENT_MASK_ALL   ((ComponentMask)~0ULL)

static inline ComponentMask component_mask(ComponentType type) {
    return (ComponentMask)1 << type;
}
static inline bool component_mask_has(ComponentMask mask, ComponentType type) {
    return (mask & component_mask(type)) != 0;
}
static inline ComponentMask component_mask_add(ComponentMask mask, ComponentType type) {
    return mask | component_mask(type);
}
static inline ComponentMask component_mask_remove(ComponentMask mask, ComponentType type) {
    return mask & ~component_mask(type);
}

// Built-in Component Types
typedef enum ComponentTypeId {
    // 2D Components
    COMPONENT_TRANSFORM = 0,
    COMPONENT_VELOCITY,
    COMPONENT_SPRITE,
    COMPONENT_HEALTH,
    COMPONENT_PLAYER,
    COMPONENT_AI,
    COMPONENT_COLLIDER,
    COMPONENT_TEAM,

    // 3D Components (v0.8.0+)
    COMPONENT_TRANSFORM3D,
    COMPONENT_MESH_RENDERER,
    COMPONENT_CAMERA,
    COMPONENT_LIGHT,
    COMPONENT_SKINNED_MESH,

    // Animation system (v0.9.0+)
    COMPONENT_ANIMATION_STATE,

    COMPONENT_TYPE_COUNT  // Must be last
} ComponentTypeId;

// ============================================================================
// Entity
// ============================================================================

#define ENTITY_INDEX_BITS    20
#define ENTITY_INDEX_MASK    ((1 << ENTITY_INDEX_BITS) - 1)
#define ENTITY_GENERATION_BITS 12
#define ENTITY_GENERATION_MASK ((1 << ENTITY_GENERATION_BITS) - 1)
#define MAX_ENTITIES         (1 << ENTITY_INDEX_BITS)  // ~1 million entities

// Entity handle - combines index and generation for safe references
typedef struct Entity {
    uint32_t id;  // Lower 20 bits: index, Upper 12 bits: generation
} Entity;

#define ENTITY_NULL ((Entity){0})

static inline uint32_t entity_index(Entity e) { return e.id & ENTITY_INDEX_MASK; }
static inline uint32_t entity_generation(Entity e) { return (e.id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK; }
static inline Entity entity_make(uint32_t index, uint32_t gen) {
    return (Entity){(index & ENTITY_INDEX_MASK) | ((gen & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS)};
}
static inline bool entity_is_null(Entity e) { return e.id == 0; }
static inline bool entity_equals(Entity a, Entity b) { return a.id == b.id; }

// ============================================================================
// Systems
// ============================================================================

// Forward declaration of World for SystemFn
typedef struct World World;

// System function signature
typedef void (*SystemFn)(World* world, float dt);

#define MAX_SYSTEMS 32 // Max number of systems

typedef struct System {
    SystemFn fn;
    ComponentMask required;
} System;

// ============================================================================
// World Structure
// ============================================================================

struct World {
    Arena* arena;
    uint32_t max_entities;
    uint32_t entity_count;

    // Entity data (parallel arrays)
    uint8_t* generations;      // Generation counter per slot
    ComponentMask* masks;      // Component mask per entity

    // Free list for recycling entity slots
    uint32_t* free_list;
    uint32_t free_count;
    uint32_t next_index;       // Next unused index

    // Component storage (sparse arrays)
    void* components[COMPONENT_TYPE_COUNT];

    // Animation manager (v0.9.0+)
    AnimationManager* animation_mgr; // Use forward declared type

    // Systems
    System systems[MAX_SYSTEMS];
    uint32_t system_count;
};

// ============================================================================
// Component Definitions (after World struct)
// ============================================================================

// Transform component
typedef struct Transform {
    float x, y, z;
    float rotation;
    float scale_x, scale_y;
} Transform;

// Velocity component
typedef struct Velocity {
    float x, y, z;
} Velocity;

// Sprite component
typedef struct Sprite {
    uint32_t texture_id;
    float u0, v0, u1, v1;  // UV coordinates
    float width, height;
    uint32_t layer;        // Render order
    uint32_t color;        // RGBA tint
} Sprite;

// Health component
typedef struct Health {
    float current;
    float max;
    float regen;
} Health;

// Player component (marker + player ID)
typedef struct Player {
    uint8_t player_id;
    uint8_t team;
} Player;

// AI component
typedef struct AI {
    uint8_t state;
    Entity target;
    float aggro_range;
} AI;

// Collider component
typedef struct Collider {
    float radius;
    uint16_t layer;
    uint16_t mask;
} Collider;

// Team component
typedef struct Team {
    uint8_t team_id;
} Team;

// Animation state component
typedef struct AnimationState {
    Entity entity;          // Entity this state belongs to
    
    uint32_t current_clip_id;    // Currently playing animation
    float current_time;         // Current playback time
    float playback_speed;       // Speed multiplier
    bool is_playing;            // Is animation currently playing?
    bool is_looping;           // Should the current clip loop?
    
    // Blending state
    uint32_t blend_from_clip_id; // Animation blending from
    float blend_start_time;     // When blending started
    float blend_duration;       // How long blending should take
    float blend_factor;         // Current blend factor (0-1)
    
    // Resulting bone matrices (sent to GPU)
    Mat4 bone_matrices[MAX_BONES_PER_MESH];
    uint32_t active_bone_count; // Number of bones actually animated
} AnimationState;

#define world_add_animation_state(w, e)   ((AnimationState*)world_add_component(w, e, COMPONENT_ANIMATION_STATE))
#define world_get_animation_state(w, e)  ((AnimationState*)world_get_component(w, e, COMPONENT_ANIMATION_STATE))

// ============================================================================
// World API
// ============================================================================

// World creation/destruction
World* world_create(Arena* arena, uint32_t max_entities);
void world_destroy(World* world);

// Entity management
Entity world_spawn(World* world);
void world_despawn(World* world, Entity entity);
bool world_is_alive(World* world, Entity entity);
uint32_t world_entity_count(World* world);

// Component management
void* world_add_component(World* world, Entity entity, ComponentType type);
void* world_get_component(World* world, Entity entity, ComponentType type);
bool world_has_component(World* world, Entity entity, ComponentType type);
void world_remove_component(World* world, Entity entity, ComponentType type);

// Convenience macros for typed component access
#define world_add_transform(w, e) ((Transform*)world_add_component(w, e, COMPONENT_TRANSFORM))
#define world_add_velocity(w, e)  ((Velocity*)world_add_component(w, e, COMPONENT_VELOCITY))
#define world_add_sprite(w, e)    ((Sprite*)world_add_component(w, e, COMPONENT_SPRITE))
#define world_add_health(w, e)    ((Health*)world_add_component(w, e, COMPONENT_HEALTH))
#define world_add_player(w, e)    ((Player*)world_add_component(w, e, COMPONENT_PLAYER))
#define world_add_collider(w, e)  ((Collider*)world_add_component(w, e, COMPONENT_COLLIDER))

#define world_get_transform(w, e) ((Transform*)world_get_component(w, e, COMPONENT_TRANSFORM))
#define world_get_velocity(w, e)  ((Velocity*)world_get_component(w, e, COMPONENT_VELOCITY))
#define world_get_sprite(w, e)    ((Sprite*)world_get_component(w, e, COMPONENT_SPRITE))
#define world_get_health(w, e)    ((Health*)world_get_component(w, e, COMPONENT_HEALTH))
#define world_get_player(w, e)    ((Player*)world_get_component(w, e, COMPONENT_PLAYER))
#define world_get_collider(w, e)  ((Collider*)world_get_component(w, e, COMPONENT_COLLIDER))

// 3D component macros (components_3d.h must be included separately)
#define world_add_transform3d(w, e)    ((Transform3D*)world_add_component(w, e, COMPONENT_TRANSFORM3D))
#define world_add_mesh_renderer(w, e)  ((MeshRenderer*)world_add_component(w, e, COMPONENT_MESH_RENDERER))
#define world_add_camera(w, e)         ((Camera*)world_add_component(w, e, COMPONENT_CAMERA))
#define world_add_light(w, e)          ((Light*)world_add_component(w, e, COMPONENT_LIGHT))
#define world_add_skinned_mesh(w, e)   ((SkinnedMesh*)world_add_component(w, e, COMPONENT_SKINNED_MESH))

#define world_get_transform3d(w, e)    ((Transform3D*)world_get_component(w, e, COMPONENT_TRANSFORM3D))
#define world_get_mesh_renderer(w, e)  ((MeshRenderer*)world_get_component(w, e, COMPONENT_MESH_RENDERER))
#define world_get_camera(w, e)         ((Camera*)world_get_component(w, e, COMPONENT_CAMERA))
#define world_get_light(w, e)          ((Light*)world_get_component(w, e, COMPONENT_LIGHT))
#define world_get_skinned_mesh(w, e)   ((SkinnedMesh*)world_get_component(w, e, COMPONENT_SKINNED_MESH))

// Maximum bones per skinned mesh (copied from components_3d.h to avoid circular include)
#define MAX_BONES_PER_MESH 64

// ============================================================================
// Query (Iterate entities with specific components)
// ============================================================================

typedef struct Query {
    World* world;
    ComponentMask required;
    ComponentMask excluded;
    uint32_t current_index;
} Query;

// Query creation
Query world_query(World* world, ComponentMask required);
Query world_query_exclude(World* world, ComponentMask required, ComponentMask excluded);

// Query iteration
bool query_next(Query* query, Entity* out_entity);
void query_reset(Query* query);

// System registration
void world_register_system(World* world, SystemFn fn, ComponentMask required);
void world_run_systems(World* world, float dt);

#endif
