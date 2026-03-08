#include "combat.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// Damage Application
// ============================================================================

float combat_apply_damage(World* world, DamageEvent* event) {
    if (!world_is_alive(world, event->target)) return 0.0f;
    
    Health* health = world_get_health(world, event->target);
    if (!health) return 0.0f;
    
    float damage = event->amount;
    
    // TODO: Apply resistances based on damage type
    // For now, apply damage directly
    
    health->current -= damage;
    if (health->current < 0.0f) {
        health->current = 0.0f;
    }
    
    return damage;
}

float combat_apply_heal(World* world, Entity target, float amount, Entity source) {
    (void)source;
    
    if (!world_is_alive(world, target)) return 0.0f;
    
    Health* health = world_get_health(world, target);
    if (!health) return 0.0f;
    
    float missing = health->max - health->current;
    float healed = (amount < missing) ? amount : missing;
    
    health->current += healed;
    
    return healed;
}

bool combat_is_dead(World* world, Entity entity) {
    if (!world_is_alive(world, entity)) return true;
    
    Health* health = world_get_health(world, entity);
    if (!health) return false;  // No health = invulnerable
    
    return health->current <= 0.0f;
}

void combat_handle_death(World* world, Entity entity, Entity killer) {
    (void)killer;  // TODO: Credit kill, drop loot
    
    if (!world_is_alive(world, entity)) return;
    
    printf("Entity %u died\n", entity_index(entity));
    world_despawn(world, entity);
}

// ============================================================================
// Collision Detection
// ============================================================================

bool combat_check_collision(World* world, Entity a, Entity b) {
    if (!world_is_alive(world, a) || !world_is_alive(world, b)) return false;
    if (entity_equals(a, b)) return false;
    
    Transform* ta = world_get_transform(world, a);
    Transform* tb = world_get_transform(world, b);
    Collider* ca = world_get_collider(world, a);
    Collider* cb = world_get_collider(world, b);
    
    if (!ta || !tb || !ca || !cb) return false;
    
    float dx = tb->x - ta->x;
    float dy = tb->y - ta->y;
    float dist_sq = dx * dx + dy * dy;
    float radius_sum = ca->radius + cb->radius;
    
    return dist_sq <= radius_sum * radius_sum;
}

uint32_t combat_find_collisions(World* world, Entity entity,
                                 Entity* out_entities, uint32_t max_count) {
    uint32_t count = 0;
    
    Query query = world_query(world, 
        component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_COLLIDER));
    
    Entity other;
    while (query_next(&query, &other) && count < max_count) {
        if (combat_check_collision(world, entity, other)) {
            out_entities[count++] = other;
        }
    }
    
    return count;
}

// ============================================================================
// Combat Systems
// ============================================================================

void combat_projectile_system(World* world, float dt) {
    (void)dt;
    
    // Query all projectiles (entities with velocity + collider but no AI/Player)
    Query query = world_query(world,
        component_mask(COMPONENT_TRANSFORM) | 
        component_mask(COMPONENT_VELOCITY) |
        component_mask(COMPONENT_COLLIDER));
    
    Entity proj;
    while (query_next(&query, &proj)) {
        // Skip if it's a player or AI unit
        if (world_has_component(world, proj, COMPONENT_PLAYER)) continue;
        if (world_has_component(world, proj, COMPONENT_AI)) continue;
        
        // Find collisions
        Entity hits[8];
        uint32_t hit_count = combat_find_collisions(world, proj, hits, 8);
        
        for (uint32_t i = 0; i < hit_count; i++) {
            // Skip non-health entities
            if (!world_has_component(world, hits[i], COMPONENT_HEALTH)) continue;
            
            // Apply damage
            DamageEvent event = {
                .source = ENTITY_NULL,  // TODO: Track projectile owner
                .target = hits[i],
                .amount = 25.0f,  // TODO: Get from projectile component
                .type = DAMAGE_MAGIC,
                .is_crit = false,
                .is_dot = false
            };
            combat_apply_damage(world, &event);
            
            // Destroy projectile on hit
            world_despawn(world, proj);
            break;
        }
    }
}

void combat_dot_system(World* world, float dt) {
    (void)world; (void)dt;
    // TODO: Process damage over time effects
}

void combat_death_system(World* world, float dt) {
    (void)dt;
    
    // Find all dead entities
    Query query = world_query(world, component_mask(COMPONENT_HEALTH));
    
    Entity entity;
    while (query_next(&query, &entity)) {
        if (combat_is_dead(world, entity)) {
            combat_handle_death(world, entity, ENTITY_NULL);
        }
    }
}

