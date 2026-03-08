#ifndef ARENA_SPAWNER_H
#define ARENA_SPAWNER_H

#include "../ecs/ecs.h"
#include "../map/map.h"
#include <stdint.h>

// ============================================================================
// Spawner Configuration
// ============================================================================

#define MINION_WAVE_INTERVAL    30.0f   // Seconds between waves
#define MINIONS_PER_WAVE        3       // Minions per lane per wave

// ============================================================================
// Spawner State
// ============================================================================

typedef struct Spawner {
    float wave_timer;
    uint32_t wave_count;
    bool enabled;
} Spawner;

// ============================================================================
// Spawner API
// ============================================================================

// Initialize spawner
void spawner_init(Spawner* spawner);

// Update spawner (call each tick)
void spawner_update(Spawner* spawner, World* world, const Map* map, float dt);

// Spawn functions
Entity spawn_minion(World* world, float x, float y, uint8_t team);
Entity spawn_tower(World* world, float x, float y, uint8_t team);

// Spawn initial game objects from map
void spawn_initial_objects(World* world, const Map* map);

#endif

