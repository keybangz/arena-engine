#ifndef ARENA_CHAMPION_H
#define ARENA_CHAMPION_H

#include "../ecs/ecs.h"
#include "ability.h"
#include <stdint.h>

// ============================================================================
// Champion Stats
// ============================================================================

typedef struct ChampionStats {
    // Base stats
    float health;
    float health_regen;
    float mana;
    float mana_regen;
    float attack_damage;
    float ability_power;
    float armor;
    float magic_resist;
    float attack_speed;
    float move_speed;
    float crit_chance;
    float crit_damage;
    
    // Per-level scaling
    float health_per_level;
    float mana_per_level;
    float attack_damage_per_level;
    float armor_per_level;
    float magic_resist_per_level;
} ChampionStats;

// ============================================================================
// Champion Definition
// ============================================================================

#define MAX_CHAMPION_ABILITIES 4

typedef struct ChampionDef {
    uint32_t id;
    const char* name;
    const char* title;
    
    // Base stats
    ChampionStats base_stats;
    
    // Abilities (Q, W, E, R)
    uint32_t ability_ids[MAX_CHAMPION_ABILITIES];
    
    // Visual
    uint32_t sprite_id;
    uint32_t color;
} ChampionDef;

// ============================================================================
// Champion Component (Runtime state)
// ============================================================================

typedef struct Champion {
    uint32_t def_id;        // ChampionDef ID
    uint8_t level;
    uint32_t experience;
    uint32_t gold;
    
    // Current stats (base + items + buffs)
    float current_mana;
    float max_mana;
    float mana_regen;
    float attack_damage;
    float ability_power;
    float armor;
    float magic_resist;
    float attack_speed;
    float move_speed;
    
    // Combat state
    float attack_cooldown;
    uint8_t ability_levels[MAX_CHAMPION_ABILITIES];
    
    // Respawn
    float death_timer;
    bool is_dead;
} Champion;

// ============================================================================
// Experience & Leveling
// ============================================================================

#define MAX_CHAMPION_LEVEL 18

// Experience required to reach each level (cumulative)
extern const uint32_t CHAMPION_EXP_TABLE[MAX_CHAMPION_LEVEL];

// ============================================================================
// Champion API
// ============================================================================

// Registry
void champion_registry_init(void);
const ChampionDef* champion_get_def(uint32_t id);
uint32_t champion_register(ChampionDef def);

// Champion creation
Entity champion_spawn(World* world, uint32_t def_id, float x, float y, uint8_t team);
void champion_init_stats(World* world, Entity entity);

// Leveling
void champion_add_experience(World* world, Entity entity, uint32_t exp);
void champion_level_up(World* world, Entity entity);
bool champion_can_level_ability(World* world, Entity entity, uint8_t slot);
void champion_level_ability(World* world, Entity entity, uint8_t slot);

// Stats
void champion_recalculate_stats(World* world, Entity entity);
float champion_get_stat(World* world, Entity entity, const char* stat_name);

// Combat
void champion_take_damage(World* world, Entity entity, float amount, bool is_magic);
void champion_heal(World* world, Entity entity, float amount);
void champion_die(World* world, Entity entity, Entity killer);
void champion_respawn(World* world, Entity entity);

// Gold
void champion_add_gold(World* world, Entity entity, uint32_t amount);
bool champion_spend_gold(World* world, Entity entity, uint32_t amount);

// System
void champion_system_update(World* world, float dt);

// Built-in champion IDs
#define CHAMPION_WARRIOR    1
#define CHAMPION_MAGE       2
#define CHAMPION_RANGER     3

#endif

