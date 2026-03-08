#include "spawner.h"
#include "ai.h"
#include <stdio.h>

// ============================================================================
// Spawner Initialization
// ============================================================================

void spawner_init(Spawner* spawner) {
    spawner->wave_timer = 5.0f;  // First wave in 5 seconds
    spawner->wave_count = 0;
    spawner->enabled = true;
}

// ============================================================================
// Entity Spawning
// ============================================================================

Entity spawn_minion(World* world, float x, float y, uint8_t team) {
    Entity entity = world_spawn(world);
    
    Transform* t = world_add_transform(world, entity);
    t->x = x;
    t->y = y;
    t->scale_x = 1.0f;
    t->scale_y = 1.0f;
    
    Velocity* v = world_add_velocity(world, entity);
    v->x = 0;
    v->y = 0;
    
    Sprite* s = world_add_sprite(world, entity);
    s->width = 20.0f;
    s->height = 20.0f;
    s->color = (team == 0) ? 0xFF0080FF : 0xFF8000FF;  // Blue / Red minion
    s->layer = 5;
    
    Health* h = world_add_health(world, entity);
    h->current = 50.0f;
    h->max = 50.0f;
    
    Collider* c = world_add_collider(world, entity);
    c->radius = 10.0f;
    
    ai_init_minion(world, entity, team, 1);  // Mid lane
    
    return entity;
}

Entity spawn_tower(World* world, float x, float y, uint8_t team) {
    Entity entity = world_spawn(world);
    
    Transform* t = world_add_transform(world, entity);
    t->x = x;
    t->y = y;
    t->scale_x = 1.0f;
    t->scale_y = 1.0f;
    
    // Towers don't move, so no velocity
    
    Sprite* s = world_add_sprite(world, entity);
    s->width = 40.0f;
    s->height = 40.0f;
    s->color = (team == 0) ? 0xFF00FFFF : 0xFFFF00FF;  // Cyan / Magenta tower
    s->layer = 3;
    
    Health* h = world_add_health(world, entity);
    h->current = 500.0f;
    h->max = 500.0f;
    
    Collider* c = world_add_collider(world, entity);
    c->radius = 20.0f;
    
    ai_init_tower(world, entity, team, 40.0f, 200.0f);
    
    return entity;
}

// ============================================================================
// Initial Object Spawning
// ============================================================================

void spawn_initial_objects(World* world, const Map* map) {
    // Spawn towers from map spawn points
    for (int i = 0; i < map->spawn_count; i++) {
        const SpawnPoint* sp = &map->spawns[i];
        
        if (sp->type == SPAWN_TOWER_BLUE) {
            Entity tower = spawn_tower(world, sp->x, sp->y, 0);
            printf("Spawned blue tower at (%.0f, %.0f) entity=%u\n", 
                   sp->x, sp->y, entity_index(tower));
        } else if (sp->type == SPAWN_TOWER_RED) {
            Entity tower = spawn_tower(world, sp->x, sp->y, 1);
            printf("Spawned red tower at (%.0f, %.0f) entity=%u\n", 
                   sp->x, sp->y, entity_index(tower));
        }
    }
}

// ============================================================================
// Spawner Update
// ============================================================================

static void spawn_wave(World* world, const Map* map) {
    // Spawn minions for both teams
    const SpawnPoint* blue_spawn = map_get_spawn(map, SPAWN_MINION_BLUE, 1);
    const SpawnPoint* red_spawn = map_get_spawn(map, SPAWN_MINION_RED, 1);
    
    for (int i = 0; i < MINIONS_PER_WAVE; i++) {
        if (blue_spawn) {
            float offset = (float)(i - 1) * 30.0f;
            spawn_minion(world, blue_spawn->x + offset, blue_spawn->y, 0);
        }
        if (red_spawn) {
            float offset = (float)(i - 1) * 30.0f;
            spawn_minion(world, red_spawn->x - offset, red_spawn->y, 1);
        }
    }
}

void spawner_update(Spawner* spawner, World* world, const Map* map, float dt) {
    if (!spawner->enabled) return;
    
    spawner->wave_timer -= dt;
    
    if (spawner->wave_timer <= 0) {
        spawner->wave_count++;
        printf("Spawning wave %u\n", spawner->wave_count);
        spawn_wave(world, map);
        spawner->wave_timer = MINION_WAVE_INTERVAL;
    }
}

