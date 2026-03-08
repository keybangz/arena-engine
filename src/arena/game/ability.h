#ifndef ARENA_ABILITY_H
#define ARENA_ABILITY_H

#include "../ecs/ecs.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Ability Types
// ============================================================================

typedef enum AbilityTargetType {
    ABILITY_TARGET_NONE,        // No target (self-buff, toggle)
    ABILITY_TARGET_POINT,       // Target a location (skillshot)
    ABILITY_TARGET_DIRECTION,   // Target a direction (line skillshot)
    ABILITY_TARGET_UNIT,        // Target a unit
    ABILITY_TARGET_SELF         // Self-cast only
} AbilityTargetType;

typedef enum AbilityEffectType {
    ABILITY_EFFECT_DAMAGE,
    ABILITY_EFFECT_HEAL,
    ABILITY_EFFECT_BUFF,
    ABILITY_EFFECT_DEBUFF,
    ABILITY_EFFECT_DASH,
    ABILITY_EFFECT_PROJECTILE,
    ABILITY_EFFECT_AREA
} AbilityEffectType;

// ============================================================================
// Ability Definition (Data-driven)
// ============================================================================

typedef struct AbilityDef {
    uint32_t id;
    const char* name;
    
    // Targeting
    AbilityTargetType target_type;
    float range;
    float radius;  // For area effects
    
    // Cooldown
    float cooldown;
    float cast_time;
    
    // Cost
    float mana_cost;
    
    // Effects
    AbilityEffectType effect_type;
    float damage;
    float heal;
    float duration;  // For buffs/debuffs
    
    // Projectile (if applicable)
    float projectile_speed;
    float projectile_width;
} AbilityDef;

// ============================================================================
// Ability Instance (Runtime state)
// ============================================================================

typedef struct AbilityInstance {
    uint32_t ability_id;
    float cooldown_remaining;
    bool is_ready;
} AbilityInstance;

// ============================================================================
// Ability Container Component
// ============================================================================

#define MAX_ABILITIES 4

typedef struct Abilities {
    AbilityInstance abilities[MAX_ABILITIES];
    uint8_t ability_count;
} Abilities;

// ============================================================================
// Ability Cast Request
// ============================================================================

typedef struct AbilityCast {
    Entity caster;
    uint8_t ability_slot;       // 0-3 (Q, W, E, R)
    float target_x, target_y;   // For point/direction targeting
    Entity target_entity;       // For unit targeting
    bool is_pending;
} AbilityCast;

// ============================================================================
// Ability System API
// ============================================================================

// Ability definitions (global registry)
void ability_registry_init(void);
const AbilityDef* ability_get_def(uint32_t id);
uint32_t ability_register(AbilityDef def);

// Ability instance management
void abilities_init(Abilities* abilities);
void abilities_set(Abilities* abilities, uint8_t slot, uint32_t ability_id);
bool abilities_can_cast(const Abilities* abilities, uint8_t slot);
void abilities_start_cooldown(Abilities* abilities, uint8_t slot);
void abilities_update_cooldowns(Abilities* abilities, float dt);

// Ability casting
bool ability_try_cast(World* world, Entity caster, uint8_t slot, 
                      float target_x, float target_y, Entity target);

// Ability system (runs each tick)
void ability_system_update(World* world, float dt);

// Built-in ability IDs
#define ABILITY_FIREBALL    1
#define ABILITY_HEAL        2
#define ABILITY_DASH        3
#define ABILITY_SHIELD      4

#endif

