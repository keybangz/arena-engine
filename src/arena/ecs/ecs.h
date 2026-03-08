#ifndef ARENA_ECS_H
#define ARENA_ECS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Arena Arena;
typedef struct World World;

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

// ============================================================================
// Built-in Component Types
// ============================================================================

// Component type IDs (registered at world creation)
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

    COMPONENT_TYPE_COUNT  // Must be last
} ComponentTypeId;

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

// ============================================================================
// World (ECS Container)
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

// ============================================================================
// Systems
// ============================================================================

// System function signature
typedef void (*SystemFn)(World* world, float dt);

// System registration
void world_register_system(World* world, const char* name, SystemFn fn, ComponentMask required);
void world_run_systems(World* world, float dt);

#endif
