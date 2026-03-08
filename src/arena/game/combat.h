#ifndef ARENA_COMBAT_H
#define ARENA_COMBAT_H

#include "../ecs/ecs.h"
#include <stdbool.h>

// ============================================================================
// Damage Types
// ============================================================================

typedef enum DamageType {
    DAMAGE_PHYSICAL,
    DAMAGE_MAGIC,
    DAMAGE_TRUE,        // Ignores resistances
    DAMAGE_PURE         // Cannot be reduced or increased
} DamageType;

// ============================================================================
// Damage Event
// ============================================================================

typedef struct DamageEvent {
    Entity source;
    Entity target;
    float amount;
    DamageType type;
    bool is_crit;
    bool is_dot;        // Damage over time (not burst)
} DamageEvent;

// ============================================================================
// Combat API
// ============================================================================

// Apply damage to an entity
// Returns actual damage dealt after resistances
float combat_apply_damage(World* world, DamageEvent* event);

// Apply healing to an entity
// Returns actual amount healed (capped at max health)
float combat_apply_heal(World* world, Entity target, float amount, Entity source);

// Check if entity is dead
bool combat_is_dead(World* world, Entity entity);

// Handle death (despawn, drop loot, etc)
void combat_handle_death(World* world, Entity entity, Entity killer);

// ============================================================================
// Combat Systems
// ============================================================================

// Process projectile collisions and apply damage
void combat_projectile_system(World* world, float dt);

// Process damage over time effects
void combat_dot_system(World* world, float dt);

// Check for and handle deaths
void combat_death_system(World* world, float dt);

// ============================================================================
// Collision Detection
// ============================================================================

// Check if two entities are colliding (circle-circle)
bool combat_check_collision(World* world, Entity a, Entity b);

// Find all entities colliding with given entity
// Returns count, writes entities to out_entities (max_count)
uint32_t combat_find_collisions(World* world, Entity entity, 
                                 Entity* out_entities, uint32_t max_count);

#endif

