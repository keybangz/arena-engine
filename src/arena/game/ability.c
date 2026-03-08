#include "ability.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Ability Registry
// ============================================================================

#define MAX_ABILITY_DEFS 64

static AbilityDef g_ability_defs[MAX_ABILITY_DEFS];
static uint32_t g_ability_count = 0;

void ability_registry_init(void) {
    g_ability_count = 0;
    memset(g_ability_defs, 0, sizeof(g_ability_defs));

    // Register built-in abilities
    ability_register((AbilityDef){
        .id = ABILITY_FIREBALL,
        .name = "Fireball",
        .target_type = ABILITY_TARGET_DIRECTION,
        .range = 600.0f,
        .cooldown = 3.0f,
        .mana_cost = 50.0f,
        .effect_type = ABILITY_EFFECT_PROJECTILE,
        .damage = 80.0f,
        .projectile_speed = 800.0f,
        .projectile_width = 20.0f
    });

    ability_register((AbilityDef){
        .id = ABILITY_HEAL,
        .name = "Heal",
        .target_type = ABILITY_TARGET_SELF,
        .cooldown = 8.0f,
        .mana_cost = 80.0f,
        .effect_type = ABILITY_EFFECT_HEAL,
        .heal = 120.0f
    });

    ability_register((AbilityDef){
        .id = ABILITY_DASH,
        .name = "Dash",
        .target_type = ABILITY_TARGET_DIRECTION,
        .range = 300.0f,
        .cooldown = 10.0f,
        .mana_cost = 0.0f,
        .effect_type = ABILITY_EFFECT_DASH,
        .duration = 0.15f
    });

    printf("Ability registry initialized with %u abilities\n", g_ability_count);
}

const AbilityDef* ability_get_def(uint32_t id) {
    for (uint32_t i = 0; i < g_ability_count; i++) {
        if (g_ability_defs[i].id == id) {
            return &g_ability_defs[i];
        }
    }
    return NULL;
}

uint32_t ability_register(AbilityDef def) {
    if (g_ability_count >= MAX_ABILITY_DEFS) {
        fprintf(stderr, "Ability registry full\n");
        return 0;
    }

    g_ability_defs[g_ability_count++] = def;
    return def.id;
}

// ============================================================================
// Ability Instance Management
// ============================================================================

void abilities_init(Abilities* abilities) {
    memset(abilities, 0, sizeof(Abilities));
}

void abilities_set(Abilities* abilities, uint8_t slot, uint32_t ability_id) {
    if (slot >= MAX_ABILITIES) return;

    abilities->abilities[slot].ability_id = ability_id;
    abilities->abilities[slot].cooldown_remaining = 0.0f;
    abilities->abilities[slot].is_ready = true;

    if (slot >= abilities->ability_count) {
        abilities->ability_count = slot + 1;
    }
}

bool abilities_can_cast(const Abilities* abilities, uint8_t slot) {
    if (slot >= abilities->ability_count) return false;
    return abilities->abilities[slot].is_ready;
}

void abilities_start_cooldown(Abilities* abilities, uint8_t slot) {
    if (slot >= abilities->ability_count) return;

    const AbilityDef* def = ability_get_def(abilities->abilities[slot].ability_id);
    if (def) {
        abilities->abilities[slot].cooldown_remaining = def->cooldown;
        abilities->abilities[slot].is_ready = false;
    }
}

void abilities_update_cooldowns(Abilities* abilities, float dt) {
    for (uint8_t i = 0; i < abilities->ability_count; i++) {
        AbilityInstance* inst = &abilities->abilities[i];
        if (!inst->is_ready && inst->cooldown_remaining > 0.0f) {
            inst->cooldown_remaining -= dt;
            if (inst->cooldown_remaining <= 0.0f) {
                inst->cooldown_remaining = 0.0f;
                inst->is_ready = true;
            }
        }
    }
}

// ============================================================================
// Ability Casting
// ============================================================================

bool ability_try_cast(World* world, Entity caster, uint8_t slot,
                      float target_x, float target_y, Entity target) {
    if (!world_is_alive(world, caster)) return false;

    // Get caster's abilities component (would need to add to ECS)
    // For now, simplified - just spawn projectile for testing

    Transform* caster_t = world_get_transform(world, caster);
    if (!caster_t) return false;

    // Create a projectile toward target
    float dx = target_x - caster_t->x;
    float dy = target_y - caster_t->y;
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.01f) {
        dx /= len;
        dy /= len;

        // Spawn projectile
        Entity proj = world_spawn(world);

        Transform* t = world_add_transform(world, proj);
        t->x = caster_t->x + dx * 20.0f;  // Offset from caster
        t->y = caster_t->y + dy * 20.0f;
        t->scale_x = 1.0f;
        t->scale_y = 1.0f;

        Velocity* v = world_add_velocity(world, proj);
        v->x = dx * 800.0f;  // Projectile speed
        v->y = dy * 800.0f;

        Sprite* s = world_add_sprite(world, proj);
        s->width = 12.0f;
        s->height = 12.0f;
        s->color = 0xFFFF8800;  // Orange
        s->layer = 15;

        // Add collider for hit detection
        Collider* c = world_add_collider(world, proj);
        c->radius = 6.0f;

        return true;
    }

    return false;
}

// ============================================================================
// Ability System Update
// ============================================================================

void ability_system_update(World* world, float dt) {
    (void)world;
    (void)dt;
    // TODO: Process ability cooldowns via ECS query
    // TODO: Handle channeled abilities
    // TODO: Apply ability effects
}
