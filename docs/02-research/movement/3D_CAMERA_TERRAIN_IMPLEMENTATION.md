# 3D Camera & Terrain Implementation for Arena Engine
**Based on League of Legends Isometric Perspective**

---

## 1. CAMERA SYSTEM

### 1.1 Isometric Camera Setup

**Camera Parameters:**
```c
typedef struct {
    Vec3 position;           // Camera world position
    Vec3 target;             // Look-at target (usually champion)
    Vec3 up;                 // Up vector (0, 0, 1)
    
    // Isometric parameters
    float pitch;             // ~55-60 degrees from horizontal
    float yaw;               // 0 degrees (fixed, no rotation)
    float distance;          // Distance from target (~800-1000 units)
    
    // Viewport
    float fov;               // Field of view (45-50 degrees)
    float aspect_ratio;      // 16:9 widescreen
    float near_plane;        // 0.1
    float far_plane;         // 10000.0
} IsometricCamera;
```

**Camera Angle Calculation:**
```c
// Pitch angle: ~55-60 degrees from horizontal
float pitch_rad = 55.0f * (M_PI / 180.0f);  // ~0.96 radians

// Position camera above and behind target
Vec3 camera_offset = {
    0,                                    // No X offset (centered)
    -distance * cosf(pitch_rad),          // Back offset
    distance * sinf(pitch_rad)            // Height offset
};

camera->position = vec3_add(target, camera_offset);
camera->target = target;
camera->up = {0, 0, 1};  // Z is up
```

### 1.2 View Matrix Construction

```c
Mat4 camera_get_view_matrix(IsometricCamera* cam) {
    // Standard look-at matrix
    Vec3 forward = vec3_normalize(vec3_sub(cam->target, cam->position));
    Vec3 right = vec3_normalize(vec3_cross(forward, cam->up));
    Vec3 up = vec3_cross(right, forward);
    
    Mat4 view = mat4_identity();
    
    // Set view matrix columns
    view.m[0][0] = right.x;    view.m[1][0] = right.y;    view.m[2][0] = right.z;
    view.m[0][1] = up.x;       view.m[1][1] = up.y;       view.m[2][1] = up.z;
    view.m[0][2] = -forward.x; view.m[1][2] = -forward.y; view.m[2][2] = -forward.z;
    
    view.m[3][0] = -vec3_dot(right, cam->position);
    view.m[3][1] = -vec3_dot(up, cam->position);
    view.m[3][2] = vec3_dot(forward, cam->position);
    
    return view;
}
```

### 1.3 Projection Matrix

```c
Mat4 camera_get_projection_matrix(IsometricCamera* cam) {
    float f = 1.0f / tanf(cam->fov * 0.5f * (M_PI / 180.0f));
    float aspect = cam->aspect_ratio;
    float near = cam->near_plane;
    float far = cam->far_plane;
    
    Mat4 proj = mat4_zero();
    
    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = (far + near) / (near - far);
    proj.m[2][3] = -1.0f;
    proj.m[3][2] = (2.0f * far * near) / (near - far);
    
    return proj;
}
```

### 1.4 Screen-to-World Conversion

```c
// Convert mouse screen coordinates to world position
Vec3 screen_to_world(IsometricCamera* cam, float screen_x, float screen_y,
                     float viewport_width, float viewport_height) {
    // Normalize screen coordinates to [-1, 1]
    float ndc_x = (2.0f * screen_x) / viewport_width - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_y) / viewport_height;
    
    // Create ray in clip space
    Vec4 ray_clip = {ndc_x, ndc_y, -1.0f, 1.0f};
    
    // Transform to eye space
    Mat4 proj_inv = mat4_inverse(camera_get_projection_matrix(cam));
    Vec4 ray_eye = mat4_mul_vec4(proj_inv, ray_clip);
    ray_eye.z = -1.0f;
    ray_eye.w = 0.0f;
    
    // Transform to world space
    Mat4 view_inv = mat4_inverse(camera_get_view_matrix(cam));
    Vec4 ray_world = mat4_mul_vec4(view_inv, ray_eye);
    
    // Intersect with ground plane (Z = 0)
    Vec3 ray_origin = cam->position;
    Vec3 ray_direction = {ray_world.x, ray_world.y, ray_world.z};
    
    float t = -ray_origin.z / ray_direction.z;
    Vec3 world_pos = {
        ray_origin.x + ray_direction.x * t,
        ray_origin.y + ray_direction.y * t,
        0.0f
    };
    
    return world_pos;
}
```

---

## 2. TERRAIN SYSTEM

### 2.1 Terrain Heights

**Terrain Elevation Map:**
```c
typedef struct {
    float heights[MAP_WIDTH][MAP_HEIGHT];  // Height at each tile
    float min_height;
    float max_height;
} TerrainHeightMap;

// Load heights from map data
void terrain_load_heights(TerrainHeightMap* terrain, const Map* map) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            TileType tile = map_get_tile(map, x, y);
            
            switch (tile) {
                case TILE_EMPTY:
                    terrain->heights[y][x] = 0.0f;
                    break;
                case TILE_RIVER:
                    terrain->heights[y][x] = -10.0f;  // Slightly lower
                    break;
                case TILE_JUNGLE:
                    terrain->heights[y][x] = 5.0f;    // Slightly elevated
                    break;
                case TILE_WALL:
                    terrain->heights[y][x] = 0.0f;    // Walls are vertical
                    break;
                default:
                    terrain->heights[y][x] = 0.0f;
            }
        }
    }
}
```

### 2.2 Height Interpolation

```c
// Get height at any world position (bilinear interpolation)
float terrain_get_height_at(TerrainHeightMap* terrain, float world_x, float world_y) {
    // Convert world coordinates to tile coordinates
    float tile_x = world_x / TILE_SIZE;
    float tile_y = world_y / TILE_SIZE;
    
    int x0 = (int)tile_x;
    int y0 = (int)tile_y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    // Clamp to bounds
    x0 = (x0 < 0) ? 0 : (x0 >= MAP_WIDTH) ? MAP_WIDTH - 1 : x0;
    y0 = (y0 < 0) ? 0 : (y0 >= MAP_HEIGHT) ? MAP_HEIGHT - 1 : y0;
    x1 = (x1 >= MAP_WIDTH) ? MAP_WIDTH - 1 : x1;
    y1 = (y1 >= MAP_HEIGHT) ? MAP_HEIGHT - 1 : y1;
    
    // Get fractional parts
    float fx = tile_x - x0;
    float fy = tile_y - y0;
    
    // Bilinear interpolation
    float h00 = terrain->heights[y0][x0];
    float h10 = terrain->heights[y0][x1];
    float h01 = terrain->heights[y1][x0];
    float h11 = terrain->heights[y1][x1];
    
    float h0 = h00 * (1 - fx) + h10 * fx;
    float h1 = h01 * (1 - fx) + h11 * fx;
    
    return h0 * (1 - fy) + h1 * fy;
}
```

### 2.3 Mesh Rendering

**Terrain Mesh Generation:**
```c
typedef struct {
    Vec3 vertices[MAX_VERTICES];
    uint32_t vertex_count;
    uint32_t indices[MAX_INDICES];
    uint32_t index_count;
} TerrainMesh;

void terrain_generate_mesh(TerrainMesh* mesh, TerrainHeightMap* terrain) {
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    
    // Generate vertices
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            Vec3 vertex = {
                x * TILE_SIZE,
                y * TILE_SIZE,
                terrain->heights[y][x]
            };
            mesh->vertices[mesh->vertex_count++] = vertex;
        }
    }
    
    // Generate indices (two triangles per tile)
    for (int y = 0; y < MAP_HEIGHT - 1; y++) {
        for (int x = 0; x < MAP_WIDTH - 1; x++) {
            uint32_t v0 = y * MAP_WIDTH + x;
            uint32_t v1 = y * MAP_WIDTH + (x + 1);
            uint32_t v2 = (y + 1) * MAP_WIDTH + x;
            uint32_t v3 = (y + 1) * MAP_WIDTH + (x + 1);
            
            // Triangle 1
            mesh->indices[mesh->index_count++] = v0;
            mesh->indices[mesh->index_count++] = v2;
            mesh->indices[mesh->index_count++] = v1;
            
            // Triangle 2
            mesh->indices[mesh->index_count++] = v1;
            mesh->indices[mesh->index_count++] = v2;
            mesh->indices[mesh->index_count++] = v3;
        }
    }
}
```

---

## 3. BRUSH & FOG OF WAR

### 3.1 Brush System

```c
typedef struct {
    Vec2 position;
    float radius;
    uint8_t team;  // 0 = blue, 1 = red, 2 = neutral
} Brush;

// Check if position is in brush
bool is_in_brush(Vec2 pos, Brush* brush) {
    float dist = vec2_distance(pos, brush->position);
    return dist < brush->radius;
}

// Get visibility at position
bool can_see_position(Vec2 viewer_pos, Vec2 target_pos, uint8_t viewer_team) {
    // Check if target is in brush
    for (int i = 0; i < brush_count; i++) {
        if (is_in_brush(target_pos, &brushes[i])) {
            // In brush - only visible if warded or viewer in brush
            if (!is_in_brush(viewer_pos, &brushes[i])) {
                return false;  // Can't see into brush
            }
        }
    }
    return true;
}
```

### 3.2 Fog of War

```c
typedef struct {
    uint8_t visibility[MAP_WIDTH][MAP_HEIGHT];  // 0-255 visibility
    uint8_t team;
} FogOfWar;

void fog_update(FogOfWar* fog, World* world, uint8_t team) {
    // Clear fog
    memset(fog->visibility, 0, sizeof(fog->visibility));
    
    // Add vision from champions
    Query query = world_query(world, component_mask(COMPONENT_TRANSFORM) | 
                                     component_mask(COMPONENT_TEAM));
    
    Entity entity;
    while (query_next(&query, &entity)) {
        Team* t = (Team*)world_get_component(world, entity, COMPONENT_TEAM);
        if (t->team_id != team) continue;
        
        Transform* tr = world_get_transform(world, entity);
        float vision_radius = 1200.0f;  // Default vision
        
        // Add vision in radius
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                float dist = sqrtf(
                    (x * TILE_SIZE - tr->x) * (x * TILE_SIZE - tr->x) +
                    (y * TILE_SIZE - tr->y) * (y * TILE_SIZE - tr->y)
                );
                
                if (dist < vision_radius) {
                    fog->visibility[y][x] = 255;
                }
            }
        }
    }
}
```

---

## 4. WALL GEOMETRY

### 4.1 Wall Collision

```c
typedef struct {
    Vec2 start;
    Vec2 end;
    float height;  // For 3D rendering
} Wall;

// Check if line intersects wall
bool line_intersects_wall(Vec2 p1, Vec2 p2, Wall* wall) {
    // Line-segment intersection test
    Vec2 d1 = {p2.x - p1.x, p2.y - p1.y};
    Vec2 d2 = {wall->end.x - wall->start.x, wall->end.y - wall->start.y};
    Vec2 d3 = {wall->start.x - p1.x, wall->start.y - p1.y};
    
    float cross = d1.x * d2.y - d1.y * d2.x;
    if (fabsf(cross) < 0.001f) return false;  // Parallel
    
    float t = (d3.x * d2.y - d3.y * d2.x) / cross;
    float u = (d3.x * d1.y - d3.y * d1.x) / cross;
    
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
}

// Find intersection point
Vec2 find_wall_intersection(Vec2 p1, Vec2 p2, Wall* wall) {
    Vec2 d1 = {p2.x - p1.x, p2.y - p1.y};
    Vec2 d2 = {wall->end.x - wall->start.x, wall->end.y - wall->start.y};
    Vec2 d3 = {wall->start.x - p1.x, wall->start.y - p1.y};
    
    float cross = d1.x * d2.y - d1.y * d2.x;
    float t = (d3.x * d2.y - d3.y * d2.x) / cross;
    
    return {p1.x + d1.x * t, p1.y + d1.y * t};
}
```

### 4.2 NavMesh Generation

```c
typedef struct {
    Vec2 vertices[MAX_NAVMESH_VERTICES];
    uint32_t vertex_count;
    uint32_t polygons[MAX_NAVMESH_POLYGONS][3];  // Triangle indices
    uint32_t polygon_count;
} NavMesh;

void navmesh_generate(NavMesh* mesh, const Map* map) {
    // Generate walkable area polygons
    // Exclude walls and obstacles
    
    mesh->vertex_count = 0;
    mesh->polygon_count = 0;
    
    // For each walkable tile, create vertices and triangles
    for (int y = 0; y < MAP_HEIGHT - 1; y++) {
        for (int x = 0; x < MAP_WIDTH - 1; x++) {
            if (!map_is_walkable(map, x * TILE_SIZE, y * TILE_SIZE)) {
                continue;
            }
            
            // Create quad vertices
            Vec2 v0 = {x * TILE_SIZE, y * TILE_SIZE};
            Vec2 v1 = {(x + 1) * TILE_SIZE, y * TILE_SIZE};
            Vec2 v2 = {x * TILE_SIZE, (y + 1) * TILE_SIZE};
            Vec2 v3 = {(x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE};
            
            // Add vertices and create triangles
            // (Implementation details omitted for brevity)
        }
    }
}
```

---

## 5. RENDERING PIPELINE

### 5.1 Render Order

```c
void render_frame(Renderer* renderer, World* world, IsometricCamera* cam) {
    // 1. Render terrain
    render_terrain(renderer, cam);
    
    // 2. Render walls
    render_walls(renderer, cam);
    
    // 3. Render entities (sorted by Y for isometric)
    render_entities_sorted(renderer, world, cam);
    
    // 4. Render UI overlay
    render_ui(renderer);
}

// Sort entities by Y position for proper depth ordering
void render_entities_sorted(Renderer* renderer, World* world, IsometricCamera* cam) {
    // Collect all entities with transforms
    Entity entities[MAX_ENTITIES];
    int entity_count = 0;
    
    Query query = world_query(world, component_mask(COMPONENT_TRANSFORM));
    Entity entity;
    while (query_next(&query, &entity)) {
        entities[entity_count++] = entity;
    }
    
    // Sort by Y position (back to front)
    qsort(entities, entity_count, sizeof(Entity), 
          (int (*)(const void*, const void*))compare_entity_y);
    
    // Render in sorted order
    for (int i = 0; i < entity_count; i++) {
        render_entity(renderer, world, entities[i], cam);
    }
}
```

---

## 6. PERFORMANCE OPTIMIZATION

1. **Terrain LOD:** Use lower resolution meshes for distant terrain
2. **Frustum culling:** Only render entities within camera view
3. **Occlusion culling:** Skip rendering behind walls
4. **Fog of war caching:** Update only when vision changes
5. **Mesh batching:** Combine terrain tiles into single mesh

---

## 7. TESTING CHECKLIST

- [ ] Camera positioned at correct isometric angle
- [ ] Screen-to-world conversion accurate
- [ ] Terrain heights render correctly
- [ ] Brush visibility works
- [ ] Fog of war updates properly
- [ ] Wall collision prevents movement
- [ ] Entity depth sorting correct
- [ ] Performance acceptable (60 FPS)

