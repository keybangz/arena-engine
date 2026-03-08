#include "champion.h"
#include "combat.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Experience Table
// ============================================================================

const uint32_t CHAMPION_EXP_TABLE[MAX_CHAMPION_LEVEL] = {
    0,      // Level 1
    280,    // Level 2
    660,    // Level 3
    1140,   // Level 4
    1720,   // Level 5
    2400,   // Level 6
    3180,   // Level 7
    4060,   // Level 8
    5040,   // Level 9
    6120,   // Level 10
    7300,   // Level 11
    8580,   // Level 12
    9960,   // Level 13
    11440,  // Level 14
    13020,  // Level 15
    14700,  // Level 16
    16480,  // Level 17
    18360   // Level 18
};

// ============================================================================
// Champion Registry
// ============================================================================

#define MAX_CHAMPION_DEFS 32

static ChampionDef g_champion_defs[MAX_CHAMPION_DEFS];
static uint32_t g_champion_count = 0;

void champion_registry_init(void) {
    g_champion_count = 0;
    memset(g_champion_defs, 0, sizeof(g_champion_defs));

    // Register built-in champions
    champion_register((ChampionDef){
        .id = CHAMPION_WARRIOR,
        .name = "Warrior",
        .title = "The Brave",
        .base_stats = {
            .health = 600, .health_regen = 8,
            .mana = 200, .mana_regen = 6,
            .attack_damage = 65, .ability_power = 0,
            .armor = 35, .magic_resist = 32,
            .attack_speed = 0.7f, .move_speed = 340,
            .health_per_level = 95, .mana_per_level = 40,
            .attack_damage_per_level = 3.5f,
            .armor_per_level = 4, .magic_resist_per_level = 1.25f
        },
        .ability_ids = {ABILITY_DASH, ABILITY_SHIELD, 0, 0},
        .color = 0xFF00AAFF  // Orange
    });

    champion_register((ChampionDef){
        .id = CHAMPION_MAGE,
        .name = "Mage",
        .title = "The Arcane",
        .base_stats = {
            .health = 500, .health_regen = 6,
            .mana = 400, .mana_regen = 10,
            .attack_damage = 50, .ability_power = 0,
            .armor = 20, .magic_resist = 30,
            .attack_speed = 0.6f, .move_speed = 330,
            .health_per_level = 80, .mana_per_level = 50,
            .attack_damage_per_level = 2.5f,
            .armor_per_level = 3, .magic_resist_per_level = 1.0f
        },
        .ability_ids = {ABILITY_FIREBALL, ABILITY_HEAL, 0, 0},
        .color = 0xFFFF00FF  // Purple
    });

    champion_register((ChampionDef){
        .id = CHAMPION_RANGER,
        .name = "Ranger",
        .title = "The Swift",
        .base_stats = {
            .health = 550, .health_regen = 7,
            .mana = 300, .mana_regen = 8,
            .attack_damage = 60, .ability_power = 0,
            .armor = 25, .magic_resist = 28,
            .attack_speed = 0.8f, .move_speed = 350,
            .health_per_level = 85, .mana_per_level = 35,
            .attack_damage_per_level = 3.0f,
            .armor_per_level = 3.5f, .magic_resist_per_level = 1.0f
        },
        .ability_ids = {ABILITY_DASH, 0, 0, 0},
        .color = 0xFF00FF00  // Green
    });

    printf("Champion registry initialized with %u champions\n", g_champion_count);
}

const ChampionDef* champion_get_def(uint32_t id) {
    for (uint32_t i = 0; i < g_champion_count; i++) {
        if (g_champion_defs[i].id == id) {
            return &g_champion_defs[i];
        }
    }
    return NULL;
}

uint32_t champion_register(ChampionDef def) {
    if (g_champion_count >= MAX_CHAMPION_DEFS) {
        fprintf(stderr, "Champion registry full\n");
        return 0;
    }
    g_champion_defs[g_champion_count++] = def;
    return def.id;
}

// ============================================================================
// Champion Spawning
// ============================================================================

Entity champion_spawn(World* world, uint32_t def_id, float x, float y, uint8_t team) {
    const ChampionDef* def = champion_get_def(def_id);
    if (!def) {
        fprintf(stderr, "Unknown champion ID: %u\n", def_id);
        return ENTITY_NULL;
    }

    Entity entity = world_spawn(world);

    // Transform
    Transform* t = world_add_transform(world, entity);
    t->x = x;
    t->y = y;
    t->scale_x = 1.0f;
    t->scale_y = 1.0f;

    // Velocity
    Velocity* v = world_add_velocity(world, entity);
    v->x = 0;
    v->y = 0;

    // Sprite
    Sprite* s = world_add_sprite(world, entity);
    s->width = 36.0f;
    s->height = 36.0f;
    s->color = def->color;
    s->layer = 10;

    // Health (initialized in champion_init_stats)
    world_add_health(world, entity);

    // Collider
    Collider* c = world_add_collider(world, entity);
    c->radius = 18.0f;

    // Team
    Team* tm = (Team*)world_add_component(world, entity, COMPONENT_TEAM);
    tm->team_id = team;

    // Player marker
    Player* p = world_add_player(world, entity);
    p->player_id = 0;
    p->team = team;

    champion_init_stats(world, entity);

    printf("Spawned champion %s at (%.0f, %.0f) team %u\n", def->name, x, y, team);
    return entity;
}

// ============================================================================
// Stats & Leveling
// ============================================================================

void champion_init_stats(World* world, Entity entity) {
    // Set initial health based on default warrior stats
    Health* h = world_get_health(world, entity);
    if (h) {
        h->max = 600.0f;
        h->current = h->max;
        h->regen = 8.0f;
    }
}

void champion_add_experience(World* world, Entity entity, uint32_t exp) {
    (void)world; (void)entity; (void)exp;
    // TODO: Implement when Champion component is added to ECS
}

void champion_level_up(World* world, Entity entity) {
    (void)world; (void)entity;
    // TODO: Implement leveling
    printf("Champion leveled up!\n");
}

void champion_recalculate_stats(World* world, Entity entity) {
    (void)world; (void)entity;
    // TODO: Recalculate stats from base + items + buffs
}

// ============================================================================
// Combat
// ============================================================================

void champion_take_damage(World* world, Entity entity, float amount, bool is_magic) {
    (void)is_magic;  // TODO: Apply armor/MR reduction

    Health* h = world_get_health(world, entity);
    if (!h) return;

    h->current -= amount;
    if (h->current <= 0) {
        h->current = 0;
        // Death handled by combat system
    }
}

void champion_heal(World* world, Entity entity, float amount) {
    Health* h = world_get_health(world, entity);
    if (!h) return;

    h->current += amount;
    if (h->current > h->max) {
        h->current = h->max;
    }
}

void champion_die(World* world, Entity entity, Entity killer) {
    (void)killer;
    printf("Champion %u died!\n", entity_index(entity));
    // TODO: Start respawn timer, give gold to killer
    (void)world;
}

void champion_respawn(World* world, Entity entity) {
    Health* h = world_get_health(world, entity);
    if (h) {
        h->current = h->max;
    }

    // TODO: Move to spawn point
    Transform* t = world_get_transform(world, entity);
    if (t) {
        t->x = 200.0f;  // Default spawn
        t->y = 360.0f;
    }

    printf("Champion %u respawned!\n", entity_index(entity));
}

// ============================================================================
// Gold
// ============================================================================

void champion_add_gold(World* world, Entity entity, uint32_t amount) {
    (void)world; (void)entity; (void)amount;
    // TODO: Add to Champion component
}

bool champion_spend_gold(World* world, Entity entity, uint32_t amount) {
    (void)world; (void)entity; (void)amount;
    // TODO: Check and subtract from Champion component
    return true;
}

// ============================================================================
// System Update
// ============================================================================

void champion_system_update(World* world, float dt) {
    // Health regeneration for champions (entities with Player component)
    Query query = world_query(world,
        component_mask(COMPONENT_PLAYER) | component_mask(COMPONENT_HEALTH));

    Entity entity;
    while (query_next(&query, &entity)) {
        Health* h = world_get_health(world, entity);
        if (h && h->current > 0 && h->current < h->max) {
            h->current += h->regen * dt;
            if (h->current > h->max) {
                h->current = h->max;
            }
        }
    }
}
