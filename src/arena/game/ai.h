#ifndef ARENA_AI_H
#define ARENA_AI_H

#include "../ecs/ecs.h"
#include "../map/map.h"
#include <stdbool.h>

// ============================================================================
// AI States
// ============================================================================

typedef enum AIState {
    AI_STATE_IDLE,
    AI_STATE_PATROL,       // Following lane path
    AI_STATE_CHASE,        // Pursuing target
    AI_STATE_ATTACK,       // Attacking target
    AI_STATE_RETREAT,      // Running away
    AI_STATE_DEAD
} AIState;

// ============================================================================
// AI Types
// ============================================================================

typedef enum AIType {
    AI_TYPE_MINION,
    AI_TYPE_TOWER,
    AI_TYPE_MONSTER,
    AI_TYPE_CHAMPION_BOT
} AIType;

// ============================================================================
// Extended AI Component
// ============================================================================

typedef struct AIController {
    AIType type;
    AIState state;
    
    // Targeting
    Entity target;
    float aggro_range;
    float attack_range;
    float attack_cooldown;
    float attack_timer;
    float damage;
    
    // Pathing
    uint8_t team;
    uint8_t lane;
    uint8_t current_waypoint;
    float move_speed;
    
    // Behavior
    float state_timer;
    float search_timer;
} AIController;

// ============================================================================
// AI System API
// ============================================================================

// Initialize AI for an entity
void ai_init_minion(World* world, Entity entity, uint8_t team, uint8_t lane);
void ai_init_tower(World* world, Entity entity, uint8_t team, float damage, float range);

// AI System update (runs each tick)
void ai_system_update(World* world, const Map* map, float dt);

// Target finding
Entity ai_find_nearest_enemy(World* world, Entity self, uint8_t my_team, float range);
Entity ai_find_priority_target(World* world, Entity self, uint8_t my_team, float range);

// Helper functions
bool ai_is_enemy(World* world, Entity a, Entity b);
float ai_distance_to(World* world, Entity a, Entity b);

#endif

