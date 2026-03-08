#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Default Map Creation (Simple 3-lane arena)
// ============================================================================

Map* map_create_default(void) {
    Map* map = calloc(1, sizeof(Map));
    if (!map) return NULL;

    // 20x12 tile map (1280x768 pixels at 64px tiles)
    map->width = 20;
    map->height = 12;
    map->bounds_min_x = 0;
    map->bounds_min_y = 0;
    map->bounds_max_x = map->width * MAP_TILE_SIZE;
    map->bounds_max_y = map->height * MAP_TILE_SIZE;

    // Initialize all tiles as empty (walkable)
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            map->tiles[y][x] = TILE_EMPTY;
        }
    }

    // Add walls around the border
    for (int x = 0; x < map->width; x++) {
        map->tiles[0][x] = TILE_WALL;
        map->tiles[map->height - 1][x] = TILE_WALL;
    }
    for (int y = 0; y < map->height; y++) {
        map->tiles[y][0] = TILE_WALL;
        map->tiles[y][map->width - 1] = TILE_WALL;
    }

    // Blue base (left side)
    for (int y = 4; y <= 7; y++) {
        for (int x = 1; x <= 3; x++) {
            map->tiles[y][x] = TILE_BASE_BLUE;
        }
    }

    // Red base (right side)
    for (int y = 4; y <= 7; y++) {
        for (int x = map->width - 4; x < map->width - 1; x++) {
            map->tiles[y][x] = TILE_BASE_RED;
        }
    }

    // Center lane (middle row)
    for (int x = 4; x < map->width - 4; x++) {
        map->tiles[5][x] = TILE_LANE;
        map->tiles[6][x] = TILE_LANE;
    }

    // Some obstacles in middle
    map->tiles[5][9] = TILE_WALL;
    map->tiles[6][10] = TILE_WALL;

    // Setup spawn points
    // Blue player spawn
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 2.5f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_PLAYER_BLUE, .lane = 1, .active = true
    };

    // Red player spawn
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 17.5f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_PLAYER_RED, .lane = 1, .active = true
    };

    // Blue minion spawn
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 4.0f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_MINION_BLUE, .lane = 1, .active = true
    };

    // Red minion spawn
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 16.0f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_MINION_RED, .lane = 1, .active = true
    };

    // Blue tower
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 5.0f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_TOWER_BLUE, .lane = 1, .active = true
    };

    // Red tower
    map->spawns[map->spawn_count++] = (SpawnPoint){
        .x = 15.0f * MAP_TILE_SIZE, .y = 5.5f * MAP_TILE_SIZE,
        .type = SPAWN_TOWER_RED, .lane = 1, .active = true
    };

    // Setup mid lane path (Blue -> Red)
    Path* mid_blue = &map->paths[map->path_count++];
    mid_blue->team = 0;
    mid_blue->lane = 1;
    mid_blue->waypoint_count = 3;
    mid_blue->waypoints[0][0] = 4.0f * MAP_TILE_SIZE;
    mid_blue->waypoints[0][1] = 5.5f * MAP_TILE_SIZE;
    mid_blue->waypoints[1][0] = 10.0f * MAP_TILE_SIZE;
    mid_blue->waypoints[1][1] = 5.5f * MAP_TILE_SIZE;
    mid_blue->waypoints[2][0] = 16.0f * MAP_TILE_SIZE;
    mid_blue->waypoints[2][1] = 5.5f * MAP_TILE_SIZE;

    // Setup mid lane path (Red -> Blue)
    Path* mid_red = &map->paths[map->path_count++];
    mid_red->team = 1;
    mid_red->lane = 1;
    mid_red->waypoint_count = 3;
    mid_red->waypoints[0][0] = 16.0f * MAP_TILE_SIZE;
    mid_red->waypoints[0][1] = 5.5f * MAP_TILE_SIZE;
    mid_red->waypoints[1][0] = 10.0f * MAP_TILE_SIZE;
    mid_red->waypoints[1][1] = 5.5f * MAP_TILE_SIZE;
    mid_red->waypoints[2][0] = 4.0f * MAP_TILE_SIZE;
    mid_red->waypoints[2][1] = 5.5f * MAP_TILE_SIZE;

    printf("Map: Created default arena (%dx%d tiles, %dx%d pixels)\n",
           map->width, map->height,
           map->width * MAP_TILE_SIZE, map->height * MAP_TILE_SIZE);
    printf("Map: %d spawn points, %d paths\n", map->spawn_count, map->path_count);

    return map;
}

void map_destroy(Map* map) {
    free(map);
}

// ============================================================================
// Tile Queries
// ============================================================================

void map_world_to_tile(float world_x, float world_y, int* tile_x, int* tile_y) {
    *tile_x = (int)(world_x / MAP_TILE_SIZE);
    *tile_y = (int)(world_y / MAP_TILE_SIZE);
}

void map_tile_to_world(int tile_x, int tile_y, float* world_x, float* world_y) {
    *world_x = (tile_x + 0.5f) * MAP_TILE_SIZE;
    *world_y = (tile_y + 0.5f) * MAP_TILE_SIZE;
}

TileType map_get_tile(const Map* map, int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= map->width || tile_y < 0 || tile_y >= map->height) {
        return TILE_WALL;  // Out of bounds = wall
    }
    return map->tiles[tile_y][tile_x];
}

TileType map_get_tile_at(const Map* map, float world_x, float world_y) {
    int tx, ty;
    map_world_to_tile(world_x, world_y, &tx, &ty);
    return map_get_tile(map, tx, ty);
}

bool map_is_walkable(const Map* map, float world_x, float world_y) {
    TileType tile = map_get_tile_at(map, world_x, world_y);
    return tile != TILE_WALL && tile != TILE_WATER;
}

bool map_is_in_bounds(const Map* map, float world_x, float world_y) {
    return world_x >= map->bounds_min_x && world_x < map->bounds_max_x &&
           world_y >= map->bounds_min_y && world_y < map->bounds_max_y;
}

// ============================================================================
// Spawn Points
// ============================================================================

const SpawnPoint* map_get_spawn(const Map* map, SpawnType type, uint8_t lane) {
    for (int i = 0; i < map->spawn_count; i++) {
        if (map->spawns[i].type == type && map->spawns[i].lane == lane) {
            return &map->spawns[i];
        }
    }
    return NULL;
}

int map_get_spawns_by_type(const Map* map, SpawnType type, SpawnPoint* out, int max) {
    int count = 0;
    for (int i = 0; i < map->spawn_count && count < max; i++) {
        if (map->spawns[i].type == type) {
            out[count++] = map->spawns[i];
        }
    }
    return count;
}

// ============================================================================
// Paths
// ============================================================================

const Path* map_get_path(const Map* map, uint8_t team, uint8_t lane) {
    for (int i = 0; i < map->path_count; i++) {
        if (map->paths[i].team == team && map->paths[i].lane == lane) {
            return &map->paths[i];
        }
    }
    return NULL;
}

// ============================================================================
// Collision
// ============================================================================

bool map_check_collision(const Map* map, float x, float y, float radius) {
    // Check tiles in a bounding box around the circle
    int min_tx = (int)((x - radius) / MAP_TILE_SIZE);
    int max_tx = (int)((x + radius) / MAP_TILE_SIZE);
    int min_ty = (int)((y - radius) / MAP_TILE_SIZE);
    int max_ty = (int)((y + radius) / MAP_TILE_SIZE);

    for (int ty = min_ty; ty <= max_ty; ty++) {
        for (int tx = min_tx; tx <= max_tx; tx++) {
            TileType tile = map_get_tile(map, tx, ty);
            if (tile == TILE_WALL || tile == TILE_WATER) {
                // Simple AABB check for now
                float tile_cx = (tx + 0.5f) * MAP_TILE_SIZE;
                float tile_cy = (ty + 0.5f) * MAP_TILE_SIZE;
                float dx = fabsf(x - tile_cx);
                float dy = fabsf(y - tile_cy);
                float half = MAP_TILE_SIZE * 0.5f;

                if (dx < half + radius && dy < half + radius) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool map_line_of_sight(const Map* map, float x1, float y1, float x2, float y2) {
    // Simple raycast using Bresenham-like stepping
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < 1.0f) return true;

    float step = MAP_TILE_SIZE * 0.5f;
    int steps = (int)(dist / step) + 1;

    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float x = x1 + dx * t;
        float y = y1 + dy * t;

        TileType tile = map_get_tile_at(map, x, y);
        if (tile == TILE_WALL) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Debug
// ============================================================================

void map_print(const Map* map) {
    printf("Map (%dx%d):\n", map->width, map->height);
    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            char c = '.';
            switch (map->tiles[y][x]) {
                case TILE_WALL: c = '#'; break;
                case TILE_WATER: c = '~'; break;
                case TILE_BRUSH: c = '*'; break;
                case TILE_BASE_BLUE: c = 'B'; break;
                case TILE_BASE_RED: c = 'R'; break;
                case TILE_LANE: c = '-'; break;
                case TILE_JUNGLE: c = 'J'; break;
                default: c = '.'; break;
            }
            printf("%c", c);
        }
        printf("\n");
    }
}

