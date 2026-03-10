#include "ecs.h"
#include "../alloc/arena_allocator.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// Component Registry
// ============================================================================

// Include 3D components for size registration
#include "components_3d.h"

static const size_t COMPONENT_SIZES[COMPONENT_TYPE_COUNT] = {
    // 2D Components
    [COMPONENT_TRANSFORM] = sizeof(Transform),
    [COMPONENT_VELOCITY]  = sizeof(Velocity),
    [COMPONENT_SPRITE]    = sizeof(Sprite),
    [COMPONENT_HEALTH]    = sizeof(Health),
    [COMPONENT_PLAYER]    = sizeof(Player),
    [COMPONENT_AI]        = sizeof(AI),
    [COMPONENT_COLLIDER]  = sizeof(Collider),
    [COMPONENT_TEAM]      = sizeof(Team),

    // 3D Components (v0.8.0+)
    [COMPONENT_TRANSFORM3D]    = sizeof(Transform3D),
    [COMPONENT_MESH_RENDERER]  = sizeof(MeshRenderer),
    [COMPONENT_CAMERA]         = sizeof(Camera),
    [COMPONENT_LIGHT]          = sizeof(Light),
    [COMPONENT_SKINNED_MESH]   = sizeof(SkinnedMesh),

    // Animation system (v0.9.0+)
    [COMPONENT_ANIMATION_STATE] = sizeof(AnimationState),
};

// ============================================================================
// System Entry
// ============================================================================

// ============================================================================
// World Structure (defined in ecs.h)
// ============================================================================



// ============================================================================
// World Creation/Destruction
// ============================================================================

World* world_create(Arena* arena, uint32_t max_entities) {
    if (max_entities > MAX_ENTITIES) {
        max_entities = MAX_ENTITIES;
    }

    World* world = arena_alloc(arena, sizeof(World), 8);
    if (!world) return NULL;

    memset(world, 0, sizeof(World));
    world->arena = arena;
    world->max_entities = max_entities;

    // Allocate entity data
    world->generations = arena_alloc(arena, max_entities * sizeof(uint8_t), 1);
    world->masks = arena_alloc(arena, max_entities * sizeof(ComponentMask), 8);
    world->free_list = arena_alloc(arena, max_entities * sizeof(uint32_t), 4);

    if (!world->generations || !world->masks || !world->free_list) {
        return NULL;
    }

    memset(world->generations, 0, max_entities * sizeof(uint8_t));
    memset(world->masks, 0, max_entities * sizeof(ComponentMask));

    // Allocate component arrays
    for (uint32_t i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        size_t size = COMPONENT_SIZES[i] * max_entities;
        world->components[i] = arena_alloc(arena, size, 8);  // 8-byte alignment for components
        if (!world->components[i]) return NULL;
        memset(world->components[i], 0, size);
    }

    // Index 0 is reserved for null entity
    world->next_index = 1;
    world->generations[0] = 0;  // Null entity always has gen 0

    return world;
}

void world_destroy(World* world) {
    // Arena allocator handles cleanup
    (void)world;
}

// ============================================================================
// Entity Management
// ============================================================================

Entity world_spawn(World* world) {
    uint32_t index;

    // Try to reuse from free list
    if (world->free_count > 0) {
        index = world->free_list[--world->free_count];
    } else if (world->next_index < world->max_entities) {
        index = world->next_index++;
    } else {
        fprintf(stderr, "ECS: Max entities reached\n");
        return ENTITY_NULL;
    }

    uint32_t gen = world->generations[index];
    world->masks[index] = COMPONENT_MASK_NONE;
    world->entity_count++;

    return entity_make(index, gen);
}

void world_despawn(World* world, Entity entity) {
    uint32_t index = entity_index(entity);
    uint32_t gen = entity_generation(entity);

    if (index >= world->max_entities) return;
    if (world->generations[index] != gen) return;  // Already dead

    // Increment generation to invalidate handles
    world->generations[index]++;
    world->masks[index] = COMPONENT_MASK_NONE;

    // Add to free list
    world->free_list[world->free_count++] = index;
    world->entity_count--;
}

bool world_is_alive(World* world, Entity entity) {
    uint32_t index = entity_index(entity);
    if (index >= world->max_entities) return false;
    return world->generations[index] == entity_generation(entity);
}

uint32_t world_entity_count(World* world) {
    return world->entity_count;
}

// ============================================================================
// Component Management
// ============================================================================

void* world_add_component(World* world, Entity entity, ComponentType type) {
    if (type >= COMPONENT_TYPE_COUNT) return NULL;
    if (!world_is_alive(world, entity)) return NULL;

    uint32_t index = entity_index(entity);
    world->masks[index] = component_mask_add(world->masks[index], type);

    size_t size = COMPONENT_SIZES[type];
    void* component = (uint8_t*)world->components[type] + (index * size);
    memset(component, 0, size);

    return component;
}

void* world_get_component(World* world, Entity entity, ComponentType type) {
    if (type >= COMPONENT_TYPE_COUNT) return NULL;
    if (!world_is_alive(world, entity)) return NULL;

    uint32_t index = entity_index(entity);
    if (!component_mask_has(world->masks[index], type)) return NULL;

    size_t size = COMPONENT_SIZES[type];
    return (uint8_t*)world->components[type] + (index * size);
}

bool world_has_component(World* world, Entity entity, ComponentType type) {
    if (type >= COMPONENT_TYPE_COUNT) return false;
    if (!world_is_alive(world, entity)) return false;

    uint32_t index = entity_index(entity);
    return component_mask_has(world->masks[index], type);
}

void world_remove_component(World* world, Entity entity, ComponentType type) {
    if (type >= COMPONENT_TYPE_COUNT) return;
    if (!world_is_alive(world, entity)) return;

    uint32_t index = entity_index(entity);
    world->masks[index] = component_mask_remove(world->masks[index], type);
}

// ============================================================================
// Query System
// ============================================================================

Query world_query(World* world, ComponentMask required) {
    return (Query){
        .world = world,
        .required = required,
        .excluded = COMPONENT_MASK_NONE,
        .current_index = 0
    };
}

Query world_query_exclude(World* world, ComponentMask required, ComponentMask excluded) {
    return (Query){
        .world = world,
        .required = required,
        .excluded = excluded,
        .current_index = 0
    };
}

bool query_next(Query* query, Entity* out_entity) {
    World* world = query->world;

    while (query->current_index < world->next_index) {
        uint32_t index = query->current_index++;

        // Skip index 0 (null entity)
        if (index == 0) continue;

        ComponentMask mask = world->masks[index];

        // Check if entity is alive (has any components or is in use)
        if (mask == COMPONENT_MASK_NONE) continue;

        // Check required components
        if ((mask & query->required) != query->required) continue;

        // Check excluded components
        if ((mask & query->excluded) != 0) continue;

        *out_entity = entity_make(index, world->generations[index]);
        return true;
    }

    return false;
}

void query_reset(Query* query) {
    query->current_index = 0;
}

// ============================================================================
// System Registration
// ============================================================================

void world_register_system(World* world, SystemFn fn, ComponentMask required) {
    if (world->system_count >= MAX_SYSTEMS) {
        fprintf(stderr, "ECS: Max systems reached\n");
        return;
    }

    System* sys = &world->systems[world->system_count++];
    sys->fn = fn;
    sys->required = required;
}

void world_run_systems(World* world, float dt) {
    for (uint32_t i = 0; i < world->system_count; i++) {
        world->systems[i].fn(world, dt);
    }
}
