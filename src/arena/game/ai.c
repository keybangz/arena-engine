#include "ai.h"
#include "combat.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// Distance Helpers
// ============================================================================

float ai_distance_to(World* world, Entity a, Entity b) {
    Transform* ta = world_get_transform(world, a);
    Transform* tb = world_get_transform(world, b);
    if (!ta || !tb) return 999999.0f;

    float dx = tb->x - ta->x;
    float dy = tb->y - ta->y;
    return sqrtf(dx * dx + dy * dy);
}

bool ai_is_enemy(World* world, Entity a, Entity b) {
    // Get team components
    Team* team_a = (Team*)world_get_component(world, a, COMPONENT_TEAM);
    Team* team_b = (Team*)world_get_component(world, b, COMPONENT_TEAM);

    if (!team_a || !team_b) return false;
    return team_a->team_id != team_b->team_id;
}

// ============================================================================
// Target Finding
// ============================================================================

Entity ai_find_nearest_enemy(World* world, Entity self, uint8_t my_team, float range) {
    Transform* my_t = world_get_transform(world, self);
    if (!my_t) return ENTITY_NULL;

    Entity nearest = ENTITY_NULL;
    float nearest_dist = range + 1.0f;

    Query query = world_query(world,
        component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_TEAM));

    Entity entity;
    while (query_next(&query, &entity)) {
        if (entity_equals(entity, self)) continue;

        Team* team = (Team*)world_get_component(world, entity, COMPONENT_TEAM);
        if (!team || team->team_id == my_team) continue;

        float dist = ai_distance_to(world, self, entity);
        if (dist < nearest_dist) {
            nearest_dist = dist;
            nearest = entity;
        }
    }

    return nearest;
}

Entity ai_find_priority_target(World* world, Entity self, uint8_t my_team, float range) {
    // Priority: 1) Attacking champions 2) Minions 3) Towers
    // For now, just find nearest enemy with health

    Transform* my_t = world_get_transform(world, self);
    if (!my_t) return ENTITY_NULL;

    Entity best = ENTITY_NULL;
    float best_dist = range + 1.0f;
    int best_priority = 0;

    Query query = world_query(world,
        component_mask(COMPONENT_TRANSFORM) |
        component_mask(COMPONENT_TEAM) |
        component_mask(COMPONENT_HEALTH));

    Entity entity;
    while (query_next(&query, &entity)) {
        if (entity_equals(entity, self)) continue;

        Team* team = (Team*)world_get_component(world, entity, COMPONENT_TEAM);
        if (!team || team->team_id == my_team) continue;

        float dist = ai_distance_to(world, self, entity);
        if (dist > range) continue;

        // Determine priority (higher = better target)
        int priority = 1;
        if (world_has_component(world, entity, COMPONENT_PLAYER)) {
            priority = 3;  // Champions are priority
        } else if (world_has_component(world, entity, COMPONENT_AI)) {
            AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
            if (ai && ai->state < 10) {  // Minion-like
                priority = 2;
            }
        }

        if (priority > best_priority || (priority == best_priority && dist < best_dist)) {
            best = entity;
            best_dist = dist;
            best_priority = priority;
        }
    }

    return best;
}

// ============================================================================
// AI Initialization
// ============================================================================

void ai_init_minion(World* world, Entity entity, uint8_t team, uint8_t lane) {
    AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
    if (!ai) {
        ai = (AI*)world_add_component(world, entity, COMPONENT_AI);
    }

    ai->state = AI_STATE_PATROL;
    ai->target = ENTITY_NULL;
    ai->aggro_range = 200.0f;

    // Add team component
    Team* t = (Team*)world_add_component(world, entity, COMPONENT_TEAM);
    t->team_id = team;

    (void)lane;  // Will be used for pathing
}

void ai_init_tower(World* world, Entity entity, uint8_t team, float damage, float range) {
    AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
    if (!ai) {
        ai = (AI*)world_add_component(world, entity, COMPONENT_AI);
    }

    ai->state = AI_STATE_IDLE;  // Towers don't move
    ai->target = ENTITY_NULL;
    ai->aggro_range = range;

    // Add team component
    Team* t = (Team*)world_add_component(world, entity, COMPONENT_TEAM);
    t->team_id = team;

    (void)damage;  // Will be stored in separate damage component
}


// ============================================================================
// AI System Update
// ============================================================================

static void update_minion_ai(World* world, Entity entity, const Map* map, float dt) {
    (void)map;

    AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
    Team* team = (Team*)world_get_component(world, entity, COMPONENT_TEAM);
    Transform* t = world_get_transform(world, entity);
    Velocity* v = world_get_velocity(world, entity);

    if (!ai || !team || !t || !v) return;

    float move_speed = 100.0f;

    switch (ai->state) {
        case AI_STATE_PATROL:
        case AI_STATE_IDLE: {
            // Look for enemies
            Entity enemy = ai_find_nearest_enemy(world, entity, team->team_id, ai->aggro_range);
            if (!entity_is_null(enemy)) {
                ai->target = enemy;
                ai->state = AI_STATE_CHASE;
            } else {
                // Move toward enemy base
                float target_x = (team->team_id == 0) ? 1000.0f : 200.0f;
                float dx = target_x - t->x;
                float dist = fabsf(dx);
                if (dist > 10.0f) {
                    v->x = (dx > 0 ? 1.0f : -1.0f) * move_speed;
                } else {
                    v->x = 0;
                }
                v->y = 0;
            }
            break;
        }

        case AI_STATE_CHASE: {
            if (entity_is_null(ai->target) || !world_is_alive(world, ai->target)) {
                ai->target = ENTITY_NULL;
                ai->state = AI_STATE_PATROL;
                break;
            }

            float dist = ai_distance_to(world, entity, ai->target);
            if (dist > ai->aggro_range * 1.5f) {
                // Lost target
                ai->target = ENTITY_NULL;
                ai->state = AI_STATE_PATROL;
            } else if (dist < 50.0f) {
                // In attack range
                ai->state = AI_STATE_ATTACK;
                v->x = 0;
                v->y = 0;
            } else {
                // Move toward target
                Transform* target_t = world_get_transform(world, ai->target);
                if (target_t) {
                    float dx = target_t->x - t->x;
                    float dy = target_t->y - t->y;
                    float len = sqrtf(dx * dx + dy * dy);
                    if (len > 0.1f) {
                        v->x = (dx / len) * move_speed;
                        v->y = (dy / len) * move_speed;
                    }
                }
            }
            break;
        }

        case AI_STATE_ATTACK: {
            v->x = 0;
            v->y = 0;

            if (entity_is_null(ai->target) || !world_is_alive(world, ai->target)) {
                ai->target = ENTITY_NULL;
                ai->state = AI_STATE_PATROL;
                break;
            }

            float dist = ai_distance_to(world, entity, ai->target);
            if (dist > 60.0f) {
                ai->state = AI_STATE_CHASE;
                break;
            }

            // Attack (simple cooldown-based)
            // For now, just deal damage every 1 second
            static float attack_timer = 0;
            attack_timer += dt;
            if (attack_timer >= 1.0f) {
                attack_timer = 0;
                DamageEvent event = {
                    .source = entity,
                    .target = ai->target,
                    .amount = 10.0f,
                    .type = DAMAGE_PHYSICAL
                };
                combat_apply_damage(world, &event);
            }
            break;
        }

        default:
            break;
    }
}

static void update_tower_ai(World* world, Entity entity, float dt) {
    AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
    Team* team = (Team*)world_get_component(world, entity, COMPONENT_TEAM);

    if (!ai || !team) return;

    // Towers don't move
    Velocity* v = world_get_velocity(world, entity);
    if (v) {
        v->x = 0;
        v->y = 0;
    }

    // Find target
    if (entity_is_null(ai->target) || !world_is_alive(world, ai->target)) {
        ai->target = ai_find_priority_target(world, entity, team->team_id, ai->aggro_range);
    }

    if (!entity_is_null(ai->target)) {
        float dist = ai_distance_to(world, entity, ai->target);
        if (dist > ai->aggro_range) {
            ai->target = ENTITY_NULL;
        } else {
            // Attack
            static float tower_attack_timer = 0;
            tower_attack_timer += dt;
            if (tower_attack_timer >= 0.8f) {
                tower_attack_timer = 0;
                DamageEvent event = {
                    .source = entity,
                    .target = ai->target,
                    .amount = 40.0f,  // Tower damage
                    .type = DAMAGE_PHYSICAL
                };
                combat_apply_damage(world, &event);
            }
        }
    }
}

void ai_system_update(World* world, const Map* map, float dt) {
    Query query = world_query(world, component_mask(COMPONENT_AI));

    Entity entity;
    while (query_next(&query, &entity)) {
        AI* ai = (AI*)world_get_component(world, entity, COMPONENT_AI);
        if (!ai) continue;

        // Determine AI type by checking for movement
        Velocity* v = world_get_velocity(world, entity);
        if (v) {
            // Mobile unit (minion)
            update_minion_ai(world, entity, map, dt);
        } else {
            // Stationary (tower)
            update_tower_ai(world, entity, dt);
        }
    }
}


