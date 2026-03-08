#ifndef ARENA_MAP_H
#define ARENA_MAP_H

#include "../ecs/ecs.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Map Constants
// ============================================================================

#define MAP_TILE_SIZE       64      // Pixels per tile
#define MAP_MAX_WIDTH       64      // Max tiles wide
#define MAP_MAX_HEIGHT      64      // Max tiles tall
#define MAP_MAX_WAYPOINTS   32      // Max waypoints per path
#define MAP_MAX_PATHS       8       // Max lane paths
#define MAP_MAX_SPAWNS      16      // Max spawn points

// ============================================================================
// Tile Types
// ============================================================================

typedef enum TileType {
    TILE_EMPTY = 0,         // Walkable empty space
    TILE_WALL,              // Impassable wall
    TILE_WATER,             // Impassable water
    TILE_BRUSH,             // Walkable, provides vision cover
    TILE_BASE_BLUE,         // Blue team base area
    TILE_BASE_RED,          // Red team base area
    TILE_LANE,              // Lane path (walkable)
    TILE_JUNGLE,            // Jungle area
    TILE_TYPE_COUNT
} TileType;

// ============================================================================
// Spawn Point
// ============================================================================

typedef enum SpawnType {
    SPAWN_PLAYER_BLUE,
    SPAWN_PLAYER_RED,
    SPAWN_MINION_BLUE,
    SPAWN_MINION_RED,
    SPAWN_TOWER_BLUE,
    SPAWN_TOWER_RED,
    SPAWN_MONSTER,
    SPAWN_TYPE_COUNT
} SpawnType;

typedef struct SpawnPoint {
    float x, y;
    SpawnType type;
    uint8_t lane;           // 0=top, 1=mid, 2=bot for lane spawns
    bool active;
} SpawnPoint;

// ============================================================================
// Path (Lane waypoints)
// ============================================================================

typedef struct Path {
    float waypoints[MAP_MAX_WAYPOINTS][2];  // x, y pairs
    uint8_t waypoint_count;
    uint8_t team;           // 0=blue, 1=red
    uint8_t lane;           // 0=top, 1=mid, 2=bot
} Path;

// ============================================================================
// Map Structure
// ============================================================================

typedef struct Map {
    // Dimensions
    uint16_t width;
    uint16_t height;
    
    // Tile data
    TileType tiles[MAP_MAX_HEIGHT][MAP_MAX_WIDTH];
    
    // Spawn points
    SpawnPoint spawns[MAP_MAX_SPAWNS];
    uint8_t spawn_count;
    
    // Lane paths
    Path paths[MAP_MAX_PATHS];
    uint8_t path_count;
    
    // Bounds (pixel coordinates)
    float bounds_min_x, bounds_min_y;
    float bounds_max_x, bounds_max_y;
} Map;

// ============================================================================
// Map API
// ============================================================================

// Creation
Map* map_create_default(void);
void map_destroy(Map* map);

// Tile queries
TileType map_get_tile(const Map* map, int tile_x, int tile_y);
TileType map_get_tile_at(const Map* map, float world_x, float world_y);
bool map_is_walkable(const Map* map, float world_x, float world_y);
bool map_is_in_bounds(const Map* map, float world_x, float world_y);

// Coordinate conversion
void map_world_to_tile(float world_x, float world_y, int* tile_x, int* tile_y);
void map_tile_to_world(int tile_x, int tile_y, float* world_x, float* world_y);

// Spawn points
const SpawnPoint* map_get_spawn(const Map* map, SpawnType type, uint8_t lane);
int map_get_spawns_by_type(const Map* map, SpawnType type, SpawnPoint* out, int max);

// Paths
const Path* map_get_path(const Map* map, uint8_t team, uint8_t lane);

// Collision
bool map_line_of_sight(const Map* map, float x1, float y1, float x2, float y2);
bool map_check_collision(const Map* map, float x, float y, float radius);

// Debug
void map_print(const Map* map);

#endif

