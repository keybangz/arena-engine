# Arena Engine MOBA - Feature Milestones

> **Current Version:** v0.1.0
> **Last Updated:** 2026-03-08

## Executive Summary

This document outlines the development roadmap from the current state (v0.1.0) to a feature-complete MOBA game (v1.0.0). Each milestone builds incrementally on the previous, with clear deliverables and acceptance criteria.

### Current State Assessment (v0.1.0)

| System | Status | Notes |
|--------|--------|-------|
| Arena Allocator | ✅ Complete | Fully functional with custom allocator support |
| HashMap | ✅ Complete | Open addressing with linear probing |
| Array | ❌ Stub only | Header exists, no implementation |
| Vec3/Mat4/Quat | ❌ Stub only | Headers exist, no implementation |
| ECS | ❌ Stub only | Empty header |
| Renderer | ❌ Stub only | Vulkan/GLFW linked, no code |
| World | ❌ Stub only | Empty header |
| Network | ❌ Stub only | Empty header |
| Server | ❌ Stub only | Empty header |

---

## v0.2.0 - "Foundation" (MVP Core Engine)

**Goal:** Render a colored triangle and display FPS counter
**Duration:** 3-4 weeks
**Complexity:** High (Vulkan boilerplate)

### Prerequisites
- Vulkan SDK installed ✅
- GLFW available ✅
- Build system working ✅

### Deliverables

#### 1. Math Library (~400 LOC)

**File: `src/arena/math/vec3.h` + `vec3.c`**
```c
typedef struct { float x, y, z; } Vec3;

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_mul(Vec3 v, float s);
Vec3 vec3_div(Vec3 v, float s);
float vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
float vec3_len(Vec3 v);
float vec3_len_sq(Vec3 v);
Vec3 vec3_normalize(Vec3 v);
Vec3 vec3_lerp(Vec3 a, Vec3 b, float t);
```

**File: `src/arena/math/mat4.h` + `mat4.c`**
```c
typedef struct { float m[16]; } Mat4;

Mat4 mat4_identity(void);
Mat4 mat4_mul(Mat4 a, Mat4 b);
Vec3 mat4_transform_point(Mat4 m, Vec3 v);
Vec3 mat4_transform_dir(Mat4 m, Vec3 v);
Mat4 mat4_translate(Vec3 t);
Mat4 mat4_rotate_x(float radians);
Mat4 mat4_rotate_y(float radians);
Mat4 mat4_rotate_z(float radians);
Mat4 mat4_scale(Vec3 s);
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far);
Mat4 mat4_lookat(Vec3 eye, Vec3 target, Vec3 up);
Mat4 mat4_inverse(Mat4 m);
```

**File: `src/arena/math/quat.h` + `quat.c`**
```c
typedef struct { float x, y, z, w; } Quat;

Quat quat_identity(void);
Quat quat_from_axis_angle(Vec3 axis, float angle);
Quat quat_from_euler(float pitch, float yaw, float roll);
Quat quat_mul(Quat a, Quat b);
Quat quat_normalize(Quat q);
Quat quat_conjugate(Quat q);
Vec3 quat_rotate(Quat q, Vec3 v);
Quat quat_slerp(Quat a, Quat b, float t);
Mat4 quat_to_mat4(Quat q);
```

#### 2. Dynamic Array (~150 LOC)

**File: `src/arena/collections/array.h` + `array.c`**
```c
typedef struct Array Array;

Array* array_create(size_t element_size, size_t initial_capacity);
void array_destroy(Array* arr);
void* array_push(Array* arr, const void* element);
void* array_pop(Array* arr);
void* array_get(Array* arr, size_t index);
void array_set(Array* arr, size_t index, const void* element);
void array_remove(Array* arr, size_t index);
void array_clear(Array* arr);
size_t array_count(const Array* arr);
size_t array_capacity(const Array* arr);
void* array_data(Array* arr);
```

#### 3. Window & Input System (~300 LOC)

**File: `src/renderer/window.h` + `window.c`**
```c
typedef struct Window Window;

Window* window_create(const char* title, int width, int height);
void window_destroy(Window* window);
bool window_should_close(Window* window);
void window_poll_events(Window* window);
void window_get_size(Window* window, int* width, int* height);
double window_get_time(void);
```

**File: `src/renderer/input.h` + `input.c`**
```c
typedef enum { KEY_W, KEY_A, KEY_S, KEY_D, KEY_SPACE, KEY_ESCAPE, ... } KeyCode;

void input_init(Window* window);
void input_update(void);
bool input_key_down(KeyCode key);
bool input_key_pressed(KeyCode key);
bool input_key_released(KeyCode key);
void input_get_mouse_pos(float* x, float* y);
void input_get_mouse_delta(float* dx, float* dy);
bool input_mouse_button_down(int button);
```

#### 4. Vulkan Renderer Bootstrap (~1200 LOC)

**File: `src/renderer/vk_context.h` + `vk_context.c`**
- Instance creation with validation layers
- Physical device selection
- Logical device and queue creation
- Surface and swapchain setup
- Command pool and buffers
- Synchronization primitives (semaphores, fences)

**File: `src/renderer/vk_pipeline.h` + `vk_pipeline.c`**
- Shader module loading (SPIR-V)
- Graphics pipeline creation
- Render pass setup
- Framebuffer management

**File: `src/renderer/renderer.h` + `renderer.c`**
```c
typedef struct Renderer Renderer;

Renderer* renderer_create(Window* window);
void renderer_destroy(Renderer* renderer);
bool renderer_begin_frame(Renderer* renderer);
void renderer_end_frame(Renderer* renderer);
void renderer_draw_triangle(Renderer* renderer);  // MVP test
void renderer_resize(Renderer* renderer, int width, int height);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Math correctness | `vec3_normalize({3,4,0})` returns `{0.6,0.8,0}` | Unit test |
| Math performance | 1M vector operations < 50ms | Benchmark |
| Window creation | Opens 1280x720 window with title | Manual |
| Input detection | WASD keys register correctly | Manual |
| Triangle render | Colored triangle visible on screen | Visual |
| Frame timing | Steady 60+ FPS with vsync | FPS counter |
| Resize handling | Window resize doesn't crash | Manual |
| Clean shutdown | No validation errors on exit | Vulkan validation |

### Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Vulkan complexity | High | High | Start with minimal triangle, add features incrementally |
| Driver compatibility | Medium | Medium | Test on multiple GPUs, use validation layers |
| Memory leaks | Medium | Low | Use arena allocator, run with address sanitizer |

### Estimated LOC: ~2,050

---

## v0.3.0 - "Movement"

**Goal:** Control a textured quad in 2D space with camera following
**Duration:** 2-3 weeks
**Complexity:** Medium

### Prerequisites
- v0.2.0 complete
- Triangle rendering working

### Deliverables

#### 1. ECS Core (~500 LOC)

**File: `src/arena/ecs/ecs.h` + `ecs.c`**
```c
typedef uint64_t Entity;
typedef uint32_t ComponentType;

typedef struct World World;

World* world_create(Arena* arena);
void world_destroy(World* world);

Entity world_spawn(World* world);
void world_despawn(World* world, Entity entity);
bool world_is_alive(World* world, Entity entity);

void world_add_component(World* world, Entity entity, ComponentType type, const void* data);
void* world_get_component(World* world, Entity entity, ComponentType type);
void world_remove_component(World* world, Entity entity, ComponentType type);
bool world_has_component(World* world, Entity entity, ComponentType type);

// Query system
typedef struct Query Query;
Query* world_query(World* world, ComponentType* types, size_t count);
bool query_next(Query* query, Entity* out_entity);
void query_destroy(Query* query);
```

#### 2. Core Components (~200 LOC)

**File: `src/arena/ecs/components.h`**
```c
// Component type IDs
#define COMPONENT_TRANSFORM   0
#define COMPONENT_VELOCITY    1
#define COMPONENT_SPRITE      2
#define COMPONENT_COLLIDER    3
#define COMPONENT_PLAYER      4

typedef struct {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
} TransformComponent;

typedef struct {
    Vec3 linear;
    Vec3 angular;
} VelocityComponent;

typedef struct {
    uint32_t texture_id;
    Vec3 color;
    Vec3 size;
} SpriteComponent;

typedef struct {
    Vec3 half_extents;
    uint32_t layer;
    uint32_t mask;
} ColliderComponent;

typedef struct {
    uint32_t player_id;
    float move_speed;
} PlayerComponent;
```

#### 3. Movement System (~150 LOC)

**File: `src/arena/ecs/systems/movement_system.h` + `.c`**
```c
void movement_system_update(World* world, float dt);
```
- Queries entities with Transform + Velocity
- Applies velocity to position
- Handles acceleration/deceleration

#### 4. Player Control System (~200 LOC)

**File: `src/arena/ecs/systems/player_control_system.h` + `.c`**
```c
void player_control_system_update(World* world, float dt);
```
- Reads WASD input
- Sets velocity on player entity
- Handles click-to-move (optional)

#### 5. Camera System (~150 LOC)

**File: `src/renderer/camera.h` + `camera.c`**
```c
typedef struct {
    Vec3 position;
    Vec3 target;
    float zoom;
    float aspect;
} Camera;

Camera camera_create(void);
Mat4 camera_view_matrix(const Camera* cam);
Mat4 camera_projection_matrix(const Camera* cam);
void camera_follow(Camera* cam, Vec3 target, float smoothing);
Vec3 camera_screen_to_world(const Camera* cam, Vec2 screen_pos);
```

#### 6. Sprite Rendering (~400 LOC)

**File: `src/renderer/sprite_renderer.h` + `sprite_renderer.c`**
```c
typedef struct SpriteRenderer SpriteRenderer;

SpriteRenderer* sprite_renderer_create(Renderer* renderer);
void sprite_renderer_destroy(SpriteRenderer* sr);
void sprite_renderer_begin(SpriteRenderer* sr, const Camera* camera);
void sprite_renderer_draw(SpriteRenderer* sr, Vec3 pos, Vec3 size, Vec3 color);
void sprite_renderer_end(SpriteRenderer* sr);
```

#### 7. Texture Loading (~200 LOC)

**File: `src/renderer/texture.h` + `texture.c`**
```c
typedef struct Texture Texture;

Texture* texture_load(Renderer* renderer, const char* path);
void texture_destroy(Texture* texture);
void texture_bind(Texture* texture, uint32_t slot);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Entity spawn/despawn | Create 10K entities in < 10ms | Benchmark |
| Component query | Query 10K entities in < 1ms | Benchmark |
| Movement smoothness | No jitter at 60 FPS | Visual |
| Camera follow | Smooth tracking with configurable lag | Visual |
| Input responsiveness | < 16ms input-to-screen latency | Measurement |
| Texture loading | PNG files load correctly | Visual |

### Estimated LOC: ~1,800

---

## v0.4.0 - "Combat"

**Goal:** Attack enemies, see health bars, abilities with cooldowns
**Duration:** 3-4 weeks
**Complexity:** Medium-High

### Prerequisites
- v0.3.0 complete
- ECS working with basic entities

### Deliverables

#### 1. Combat Components (~250 LOC)

**File: `src/arena/ecs/components/combat_components.h`**
```c
#define COMPONENT_HEALTH      5
#define COMPONENT_COMBAT      6
#define COMPONENT_ABILITY     7
#define COMPONENT_PROJECTILE  8
#define COMPONENT_BUFF        9

typedef struct {
    float current;
    float maximum;
    float regen_rate;
    bool is_dead;
} HealthComponent;

typedef struct {
    float attack_damage;
    float attack_speed;      // attacks per second
    float attack_range;
    float last_attack_time;
    Entity target;
} CombatComponent;

typedef struct {
    uint32_t ability_id;
    float cooldown;
    float cooldown_remaining;
    float mana_cost;
    float range;
    uint32_t slot;           // Q, W, E, R = 0, 1, 2, 3
} AbilityComponent;

typedef struct {
    Entity owner;
    Vec3 direction;
    float speed;
    float damage;
    float lifetime;
} ProjectileComponent;

typedef struct {
    uint32_t buff_type;
    float duration;
    float elapsed;
    float value;             // stat modifier
} BuffComponent;
```

#### 2. Combat System (~400 LOC)

**File: `src/arena/ecs/systems/combat_system.h` + `.c`**
```c
void combat_system_update(World* world, float dt);
void combat_system_attack(World* world, Entity attacker, Entity target);
bool combat_system_in_range(World* world, Entity a, Entity b, float range);
void combat_system_apply_damage(World* world, Entity target, float damage, Entity source);
```
- Auto-attack logic with attack speed timing
- Range checking
- Damage calculation
- Death handling

#### 3. Ability System (~500 LOC)

**File: `src/arena/ecs/systems/ability_system.h` + `.c`**
```c
typedef struct AbilityDef {
    uint32_t id;
    const char* name;
    float base_cooldown;
    float base_damage;
    float mana_cost;
    float range;
    uint32_t flags;          // TARGET_ENEMY, TARGET_POINT, etc.
    void (*on_cast)(World* world, Entity caster, Vec3 target);
} AbilityDef;

void ability_system_update(World* world, float dt);
bool ability_system_can_cast(World* world, Entity caster, uint32_t ability_id);
void ability_system_cast(World* world, Entity caster, uint32_t ability_id, Vec3 target);
void ability_system_register(const AbilityDef* def);
```

#### 4. Projectile System (~200 LOC)

**File: `src/arena/ecs/systems/projectile_system.h` + `.c`**
```c
void projectile_system_update(World* world, float dt);
Entity projectile_spawn(World* world, Vec3 pos, Vec3 dir, float speed, float damage, Entity owner);
```
- Movement along trajectory
- Collision detection with entities
- Lifetime expiration

#### 5. Buff/Debuff System (~200 LOC)

**File: `src/arena/ecs/systems/buff_system.h` + `.c`**
```c
void buff_system_update(World* world, float dt);
void buff_apply(World* world, Entity target, uint32_t buff_type, float duration, float value);
void buff_remove(World* world, Entity target, uint32_t buff_type);
```

#### 6. Health Bar UI (~300 LOC)

**File: `src/renderer/ui/health_bar.h` + `.c`**
```c
void health_bar_render(Renderer* r, Vec3 world_pos, float current, float max, const Camera* cam);
void health_bar_render_batch(Renderer* r, World* world, const Camera* cam);
```

#### 7. AABB Collision (~250 LOC)

**File: `src/arena/physics/collision.h` + `.c`**
```c
typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

bool aabb_intersects(AABB a, AABB b);
bool aabb_contains_point(AABB box, Vec3 point);
AABB aabb_from_center_size(Vec3 center, Vec3 size);
bool circle_intersects(Vec3 a, float ra, Vec3 b, float rb);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Attack timing | Attack speed of 1.0 fires once per second | Unit test |
| Damage calculation | 100 HP - 25 damage = 75 HP | Unit test |
| Ability cooldown | Ability unavailable during cooldown | Manual |
| Projectile hit | Projectile damages target on contact | Visual |
| Health bar | Correctly reflects health percentage | Visual |
| Death handling | Entity despawns at 0 HP | Visual |

### Estimated LOC: ~2,100

---

## v0.5.0 - "Multiplayer"

**Goal:** Two clients controlling separate champions in shared world
**Duration:** 4-6 weeks
**Complexity:** High

### Prerequisites
- v0.4.0 complete
- Combat systems working locally

### Deliverables

#### 1. Network Protocol (~400 LOC)

**File: `src/arena/network/protocol.h`**
```c
typedef enum {
    MSG_CONNECT_REQUEST,
    MSG_CONNECT_RESPONSE,
    MSG_DISCONNECT,
    MSG_PLAYER_INPUT,
    MSG_GAME_STATE,
    MSG_ENTITY_SPAWN,
    MSG_ENTITY_DESPAWN,
    MSG_ENTITY_UPDATE,
    MSG_ABILITY_CAST,
    MSG_DAMAGE_EVENT,
    MSG_PING,
    MSG_PONG,
} MessageType;

typedef struct {
    uint16_t type;
    uint16_t size;
    uint32_t sequence;
    uint32_t ack;
    uint8_t data[];
} Packet;

size_t packet_serialize(const Packet* pkt, uint8_t* buffer, size_t max);
bool packet_deserialize(const uint8_t* buffer, size_t len, Packet* out);
```

#### 2. UDP Socket Layer (~300 LOC)

**File: `src/arena/network/socket.h` + `.c`**
```c
typedef struct Socket Socket;
typedef struct {
    uint32_t ip;
    uint16_t port;
} Address;

Socket* socket_create(uint16_t port);
void socket_destroy(Socket* socket);
int socket_send(Socket* socket, Address dest, const void* data, size_t len);
int socket_recv(Socket* socket, Address* from, void* buffer, size_t max);
void socket_set_nonblocking(Socket* socket, bool nonblocking);
```

#### 3. Client Network Manager (~500 LOC)

**File: `src/client/network_client.h` + `.c`**
```c
typedef struct NetworkClient NetworkClient;

NetworkClient* network_client_create(void);
void network_client_destroy(NetworkClient* client);
bool network_client_connect(NetworkClient* client, const char* host, uint16_t port);
void network_client_disconnect(NetworkClient* client);
void network_client_update(NetworkClient* client, float dt);
void network_client_send_input(NetworkClient* client, const PlayerInput* input);
bool network_client_is_connected(NetworkClient* client);
float network_client_get_ping(NetworkClient* client);
```

#### 4. Server Network Manager (~600 LOC)

**File: `src/server/network_server.h` + `.c`**
```c
typedef struct NetworkServer NetworkServer;
typedef uint32_t ClientId;

NetworkServer* network_server_create(uint16_t port, int max_clients);
void network_server_destroy(NetworkServer* server);
void network_server_update(NetworkServer* server, float dt);
void network_server_broadcast(NetworkServer* server, const Packet* packet);
void network_server_send(NetworkServer* server, ClientId client, const Packet* packet);
void network_server_kick(NetworkServer* server, ClientId client);
int network_server_get_client_count(NetworkServer* server);
```

#### 5. State Synchronization (~400 LOC)

**File: `src/arena/network/state_sync.h` + `.c`**
```c
// Server-side: serialize world state for network
size_t state_sync_snapshot(World* world, uint8_t* buffer, size_t max);

// Client-side: apply server state
void state_sync_apply(World* world, const uint8_t* buffer, size_t len);

// Delta compression for efficient updates
size_t state_sync_delta(const uint8_t* prev, const uint8_t* curr, uint8_t* out, size_t max);
```

#### 6. Client-Side Prediction (~300 LOC)

**File: `src/client/prediction.h` + `.c`**
```c
void prediction_record_input(uint32_t sequence, const PlayerInput* input);
void prediction_apply(World* world, const PlayerInput* input);
void prediction_reconcile(World* world, uint32_t ack_sequence, Vec3 server_pos);
```

#### 7. Server Game Loop (~400 LOC)

**File: `src/server/game_server.h` + `.c`**
```c
typedef struct GameServer GameServer;

GameServer* game_server_create(uint16_t port);
void game_server_destroy(GameServer* server);
void game_server_run(GameServer* server);  // Main loop
void game_server_tick(GameServer* server, float dt);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Connection | Client connects to server in < 1s | Manual |
| Latency | Ping < 100ms on LAN | Measurement |
| Input sync | Player movement visible to other clients | Manual |
| Combat sync | Damage events replicated correctly | Manual |
| Disconnect | Client disconnect handled gracefully | Manual |
| Reconnect | Client can reconnect to running game | Manual |

### Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Desync bugs | High | High | Checksums, state validation |
| Packet loss | Medium | Medium | Reliable delivery for important messages |
| Cheating | Medium | Low (alpha) | Server authority, input validation |

### Estimated LOC: ~2,900



---

## v0.6.0 - "Arena Alpha"

**Goal:** First playable MOBA: 2v2 with lanes, minions, towers, and one objective
**Duration:** 4-5 weeks
**Complexity:** High

### Prerequisites
- v0.5.0 complete
- Multiplayer working with combat

### Deliverables

#### 1. Map System (~400 LOC)

**File: `src/arena/world/map.h` + `.c`**
```c
typedef struct Map Map;
typedef struct {
    Vec3 position;
    uint32_t type;       // SPAWN, LANE, JUNGLE, OBJECTIVE
    uint32_t team;
} MapNode;

Map* map_load(const char* path);
void map_destroy(Map* map);
Vec3 map_get_spawn(Map* map, uint32_t team);
MapNode* map_get_nearest_node(Map* map, Vec3 pos);
bool map_raycast(Map* map, Vec3 start, Vec3 end, Vec3* hit);
```

#### 2. Navigation / Pathfinding (~500 LOC)

**File: `src/arena/ai/pathfinding.h` + `.c`**
```c
typedef struct NavMesh NavMesh;
typedef struct Path Path;

NavMesh* navmesh_create(Map* map);
void navmesh_destroy(NavMesh* navmesh);
Path* navmesh_find_path(NavMesh* navmesh, Vec3 start, Vec3 end);
void path_destroy(Path* path);
Vec3 path_get_next(Path* path);
bool path_is_complete(Path* path);
```
- A* pathfinding
- Dynamic obstacle avoidance
- Path smoothing

#### 3. Tower/Structure System (~300 LOC)

**File: `src/arena/ecs/components/structure_components.h`**
```c
#define COMPONENT_TOWER       10
#define COMPONENT_INHIBITOR   11
#define COMPONENT_NEXUS       12

typedef struct {
    uint32_t team;
    float attack_damage;
    float attack_speed;
    float attack_range;
    Entity target;
    float last_attack_time;
    bool is_invulnerable;    // protected by other towers
} TowerComponent;
```

**File: `src/arena/ecs/systems/tower_system.h` + `.c`**
```c
void tower_system_update(World* world, float dt);
```
- Target priority (minions < champions < diving champions)
- Range detection
- Invulnerability rules

#### 4. Minion AI (~400 LOC)

**File: `src/arena/ai/minion_ai.h` + `.c`**
```c
typedef struct {
    uint32_t team;
    uint32_t lane;           // 0=top, 1=mid, 2=bot
    uint32_t wave_id;
} MinionComponent;

void minion_ai_update(World* world, float dt);
void minion_spawn_wave(World* world, uint32_t team, uint32_t lane);
```
- Follow lane waypoints
- Attack nearest enemy
- Aggro switching

#### 5. Team System (~200 LOC)

**File: `src/arena/game/team.h` + `.c`**
```c
typedef struct {
    uint32_t team_id;
} TeamComponent;

bool team_is_enemy(uint32_t team_a, uint32_t team_b);
bool team_is_ally(uint32_t team_a, uint32_t team_b);
Entity* team_get_members(World* world, uint32_t team, size_t* count);
```

#### 6. Game Mode Manager (~400 LOC)

**File: `src/arena/game/game_mode.h` + `.c`**
```c
typedef enum {
    GAME_STATE_LOBBY,
    GAME_STATE_LOADING,
    GAME_STATE_PLAYING,
    GAME_STATE_VICTORY,
    GAME_STATE_DEFEAT,
} GameState;

typedef struct GameMode GameMode;

GameMode* game_mode_create(World* world);
void game_mode_update(GameMode* gm, float dt);
GameState game_mode_get_state(GameMode* gm);
void game_mode_check_victory(GameMode* gm);
float game_mode_get_time(GameMode* gm);
```

#### 7. Fog of War (~400 LOC)

**File: `src/arena/game/fog_of_war.h` + `.c`**
```c
typedef struct FogOfWar FogOfWar;

FogOfWar* fog_create(int width, int height, float cell_size);
void fog_destroy(FogOfWar* fog);
void fog_update(FogOfWar* fog, World* world, uint32_t team);
bool fog_is_visible(FogOfWar* fog, Vec3 pos, uint32_t team);
void fog_reveal_area(FogOfWar* fog, Vec3 center, float radius, uint32_t team);
```

#### 8. Minimap (~300 LOC)

**File: `src/renderer/ui/minimap.h` + `.c`**
```c
void minimap_render(Renderer* r, World* world, FogOfWar* fog, uint32_t team);
Vec3 minimap_click_to_world(Vec2 click_pos);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Map loading | Map loads in < 500ms | Benchmark |
| Pathfinding | Path found in < 5ms for typical distances | Benchmark |
| Minion waves | Spawn every 30s, follow lanes | Visual |
| Tower targeting | Correct priority order | Manual |
| Victory condition | Game ends when Nexus destroyed | Manual |
| Fog of war | Enemies hidden outside vision | Visual |

### Estimated LOC: ~2,900

---

## v0.7.0 - "Content"

**Goal:** 5 playable champions, items, jungle camps, full 5v5 map
**Duration:** 5-6 weeks
**Complexity:** Medium (mostly data, less systems)

### Prerequisites
- v0.6.0 complete
- Core game loop working

### Deliverables

#### 1. Champion System (~400 LOC)

**File: `src/arena/game/champion.h` + `.c`**
```c
typedef struct ChampionDef {
    uint32_t id;
    const char* name;
    float base_health;
    float base_mana;
    float base_ad;
    float base_armor;
    float base_mr;
    float base_speed;
    AbilityDef abilities[4];  // Q, W, E, R
    AbilityDef passive;
} ChampionDef;

Entity champion_spawn(World* world, uint32_t champion_id, Vec3 pos, uint32_t team);
const ChampionDef* champion_get_def(uint32_t id);
void champion_level_up(World* world, Entity champion);
```

#### 2. Five Starter Champions (~1500 LOC data)

| Champion | Role | Kit |
|----------|------|-----|
| **Warrior** | Fighter | Q: Dash strike, W: Shield, E: Slow, R: AoE stun |
| **Archer** | Marksman | Q: Piercing shot, W: Trap, E: Roll, R: Rain of arrows |
| **Mage** | Mage | Q: Fireball, W: Teleport, E: Barrier, R: Meteor |
| **Assassin** | Assassin | Q: Blink, W: Backstab, E: Stealth, R: Execute |
| **Guardian** | Tank | Q: Taunt, W: Damage reduction, E: Leap, R: Team shield |

Each champion: ~300 LOC (ability implementations + data)

#### 3. Item System (~500 LOC)

**File: `src/arena/game/item.h` + `.c`**
```c
typedef struct ItemDef {
    uint32_t id;
    const char* name;
    uint32_t cost;
    uint32_t sell_price;
    float stats[STAT_COUNT];      // AD, AP, HP, Armor, MR, etc.
    uint32_t* build_path;         // item IDs
    uint32_t build_path_count;
    void (*on_hit)(World* world, Entity owner, Entity target);
    void (*passive)(World* world, Entity owner, float dt);
} ItemDef;

typedef struct {
    uint32_t items[6];
    uint32_t gold;
} InventoryComponent;

bool inventory_can_buy(World* world, Entity entity, uint32_t item_id);
void inventory_buy(World* world, Entity entity, uint32_t item_id);
void inventory_sell(World* world, Entity entity, uint32_t slot);
```

#### 4. Starter Item Set (~500 LOC data)

- **Weapons:** Long Sword → Serrated Dirk → Duskblade
- **Armor:** Cloth Armor → Chain Vest → Dead Man's Plate
- **Magic:** Amplifying Tome → Fiendish Codex → Rabadon's
- **Boots:** Boots → Berserker's/Sorcerer's/Mobility
- **Consumables:** Health Potion, Mana Potion, Ward

~20 items total, organized into build paths

#### 5. Jungle System (~400 LOC)

**File: `src/arena/game/jungle.h` + `.c`**
```c
typedef struct {
    uint32_t camp_type;       // WOLF, RAPTOR, GROMP, BUFF, DRAGON, BARON
    float respawn_time;
    float respawn_remaining;
    bool is_alive;
    uint32_t buff_type;       // for buff camps
} JungleCampComponent;

void jungle_system_update(World* world, float dt);
void jungle_camp_spawn(World* world, uint32_t camp_type, Vec3 pos);
void jungle_apply_buff(World* world, Entity killer, uint32_t buff_type);
```

#### 6. Gold/Experience System (~300 LOC)

**File: `src/arena/game/economy.h` + `.c`**
```c
typedef struct {
    uint32_t gold;
    uint32_t experience;
    uint32_t level;
    uint32_t kills;
    uint32_t deaths;
    uint32_t assists;
} StatsComponent;

void economy_award_kill(World* world, Entity killer, Entity victim);
void economy_award_assist(World* world, Entity assister, Entity victim);
void economy_passive_gold(World* world, float dt);
uint32_t economy_exp_for_level(uint32_t level);
```

#### 7. 5v5 Map (~200 LOC + data)

- Three lanes with waypoints
- Jungle paths
- River with dragon/baron pits
- Base structures (2 towers per lane, inhibitor, nexus)
- Spawn points and fountain

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Champion variety | 5 distinct playstyles | Manual |
| Ability functionality | All 25 abilities work correctly | Unit tests |
| Item purchases | Buy, sell, build paths work | Manual |
| Jungle clears | Camps respawn on timer | Manual |
| Gold/XP flow | Appropriate progression speed | Balance test |
| Level scaling | Stats increase per level | Unit test |

### Estimated LOC: ~3,800


---

## v0.8.0 - "Polish"

**Goal:** Visual effects, audio, complete UI, smooth gameplay feel
**Duration:** 4-5 weeks
**Complexity:** Medium

### Prerequisites
- v0.7.0 complete
- Core gameplay feature-complete

### Deliverables

#### 1. Particle System (~500 LOC)

**File: `src/renderer/particles.h` + `.c`**
```c
typedef struct ParticleSystem ParticleSystem;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Vec3 color_start;
    Vec3 color_end;
    float size_start;
    float size_end;
    float lifetime;
} ParticleEmitterDef;

ParticleSystem* particles_create(Renderer* renderer, size_t max_particles);
void particles_destroy(ParticleSystem* ps);
void particles_emit(ParticleSystem* ps, const ParticleEmitterDef* def, int count);
void particles_update(ParticleSystem* ps, float dt);
void particles_render(ParticleSystem* ps, const Camera* camera);
```

#### 2. Effect Presets (~300 LOC data)

**File: `src/renderer/effects.h` + `.c`**
```c
void effect_spawn_hit(ParticleSystem* ps, Vec3 pos);
void effect_spawn_death(ParticleSystem* ps, Vec3 pos);
void effect_spawn_levelup(ParticleSystem* ps, Vec3 pos);
void effect_spawn_ability_q(ParticleSystem* ps, Vec3 pos, uint32_t champion_id);
// ... etc for each ability
```

#### 3. Audio System (~400 LOC)

**File: `src/audio/audio.h` + `.c`**
```c
typedef struct AudioSystem AudioSystem;
typedef uint32_t SoundId;

AudioSystem* audio_create(void);
void audio_destroy(AudioSystem* audio);
SoundId audio_load(AudioSystem* audio, const char* path);
void audio_play(AudioSystem* audio, SoundId sound);
void audio_play_at(AudioSystem* audio, SoundId sound, Vec3 position);
void audio_set_listener(AudioSystem* audio, Vec3 pos, Vec3 forward);
void audio_set_volume(AudioSystem* audio, float volume);
void audio_set_music(AudioSystem* audio, const char* path);
```

#### 4. Sound Effects Library (~200 LOC data)

- Attack sounds (per champion type)
- Ability sounds (25 abilities)
- UI sounds (click, buy, ping)
- Ambient sounds (minion march, tower idle)
- Victory/defeat jingles

#### 5. UI Framework (~600 LOC)

**File: `src/renderer/ui/ui.h` + `.c`**
```c
typedef struct UI UI;

UI* ui_create(Renderer* renderer);
void ui_destroy(UI* ui);
void ui_begin(UI* ui);
void ui_end(UI* ui);

// Widgets
bool ui_button(UI* ui, const char* label, Vec2 pos, Vec2 size);
void ui_label(UI* ui, const char* text, Vec2 pos);
void ui_image(UI* ui, Texture* tex, Vec2 pos, Vec2 size);
void ui_progress_bar(UI* ui, float value, Vec2 pos, Vec2 size);
bool ui_tooltip(UI* ui, const char* text);
```

#### 6. Game UI Screens (~800 LOC)

**File: `src/client/ui/hud.h` + `.c`**
- Health/mana bars
- Ability icons with cooldowns
- Item slots
- Gold/KDA display
- Minimap
- Chat box

**File: `src/client/ui/scoreboard.h` + `.c`**
- Player stats (K/D/A, CS, gold)
- Item builds
- Team gold comparison

**File: `src/client/ui/shop.h` + `.c`**
- Item grid
- Search/filter
- Build path visualization
- Recommended items

#### 7. Animation System (~400 LOC)

**File: `src/renderer/animation.h` + `.c`**
```c
typedef struct Animation Animation;
typedef struct Animator Animator;

Animation* animation_load(const char* path);
void animation_destroy(Animation* anim);

Animator* animator_create(void);
void animator_destroy(Animator* anim);
void animator_play(Animator* anim, Animation* animation, bool loop);
void animator_update(Animator* anim, float dt);
Mat4 animator_get_bone_transform(Animator* anim, const char* bone);
```

#### 8. Screen Shake & Juice (~150 LOC)

**File: `src/renderer/juice.h` + `.c`**
```c
void juice_screen_shake(Camera* cam, float intensity, float duration);
void juice_hit_pause(float duration);  // brief freeze frame on big hits
void juice_update(float dt);
```

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Particles | 1000 particles at 60 FPS | Benchmark |
| Audio latency | Sound plays < 50ms after event | Measurement |
| UI responsiveness | Button clicks register immediately | Manual |
| Visual clarity | Abilities clearly visible in teamfight | Visual |
| Shop usability | Can buy item in < 3 clicks | Manual |

### Estimated LOC: ~3,350

---

## v1.0.0 - "Release"

**Goal:** Feature-complete, stable, performant MOBA
**Duration:** 4-6 weeks
**Complexity:** Low (stabilization)

### Prerequisites
- v0.8.0 complete
- All features implemented

### Deliverables

#### 1. Performance Optimization (~400 LOC changes)

- Spatial partitioning for collision (quadtree)
- Frustum culling
- Batch rendering optimization
- Memory pool tuning
- Network bandwidth optimization

**File: `src/arena/physics/quadtree.h` + `.c`**
```c
typedef struct Quadtree Quadtree;

Quadtree* quadtree_create(AABB bounds, int max_depth);
void quadtree_destroy(Quadtree* qt);
void quadtree_insert(Quadtree* qt, Entity entity, AABB bounds);
void quadtree_query(Quadtree* qt, AABB region, Entity* out, size_t* count);
void quadtree_clear(Quadtree* qt);
```

#### 2. Bug Fixes & Edge Cases (~500 LOC changes)

- Reconnection handling
- Disconnect cleanup
- Memory leak fixes
- Race condition fixes
- Input edge cases

#### 3. Cheat Prevention (~300 LOC)

**File: `src/server/anticheat.h` + `.c`**
```c
bool anticheat_validate_input(const PlayerInput* input);
bool anticheat_validate_position(Entity entity, Vec3 claimed_pos);
void anticheat_flag_player(ClientId client, const char* reason);
```

#### 4. Matchmaking System (~400 LOC)

**File: `src/server/matchmaking.h` + `.c`**
```c
typedef struct MatchmakingQueue MatchmakingQueue;

MatchmakingQueue* matchmaking_create(void);
void matchmaking_destroy(MatchmakingQueue* mm);
void matchmaking_enqueue(MatchmakingQueue* mm, ClientId client, uint32_t mmr);
bool matchmaking_try_match(MatchmakingQueue* mm, ClientId* team1, ClientId* team2);
```

#### 5. Replay System (~500 LOC)

**File: `src/arena/replay/replay.h` + `.c`**
```c
typedef struct Replay Replay;

Replay* replay_start_recording(World* world);
void replay_record_frame(Replay* replay, float time);
void replay_stop_recording(Replay* replay);
void replay_save(Replay* replay, const char* path);
Replay* replay_load(const char* path);
void replay_seek(Replay* replay, float time);
void replay_apply_frame(Replay* replay, World* world);
```

#### 6. Settings & Configuration (~300 LOC)

**File: `src/client/settings.h` + `.c`**
```c
typedef struct Settings {
    int resolution_width;
    int resolution_height;
    bool fullscreen;
    bool vsync;
    int graphics_quality;      // 0=low, 1=med, 2=high
    float master_volume;
    float music_volume;
    float sfx_volume;
    KeyBinding bindings[32];
} Settings;

void settings_load(Settings* settings, const char* path);
void settings_save(const Settings* settings, const char* path);
void settings_apply(const Settings* settings);
```

#### 7. Tutorial System (~400 LOC)

**File: `src/client/tutorial.h` + `.c`**
```c
typedef struct Tutorial Tutorial;

Tutorial* tutorial_create(World* world);
void tutorial_update(Tutorial* tut, float dt);
void tutorial_render(Tutorial* tut, UI* ui);
bool tutorial_is_complete(Tutorial* tut);
void tutorial_skip(Tutorial* tut);
```

#### 8. Final Integration Testing

- Full 5v5 game playthrough
- Network stress testing (10+ simultaneous games)
- Memory profiling (no leaks over 1hr session)
- Performance profiling (steady 60 FPS)

### Acceptance Criteria

| Test | Criteria | Method |
|------|----------|--------|
| Performance | 60 FPS in 5v5 teamfight | Benchmark |
| Memory | < 500MB RAM usage | Profiling |
| Stability | No crashes in 10 full games | Testing |
| Network | Handles 100ms latency gracefully | Simulation |
| Cheating | Server rejects invalid inputs | Unit test |
| Matchmaking | Match found in < 60s with players | Manual |

### Estimated LOC: ~2,800

---

## Summary

| Version | Codename | Duration | Est. LOC | Key Deliverable |
|---------|----------|----------|----------|-----------------|
| v0.2.0 | Foundation | 3-4 weeks | 2,050 | Rendered triangle |
| v0.3.0 | Movement | 2-3 weeks | 1,800 | Controllable entity |
| v0.4.0 | Combat | 3-4 weeks | 2,100 | Attacks and abilities |
| v0.5.0 | Multiplayer | 4-6 weeks | 2,900 | Networked gameplay |
| v0.6.0 | Arena Alpha | 4-5 weeks | 2,900 | First playable |
| v0.7.0 | Content | 5-6 weeks | 3,800 | Champions and items |
| v0.8.0 | Polish | 4-5 weeks | 3,350 | VFX/Audio/UI |
| v1.0.0 | Release | 4-6 weeks | 2,800 | Feature complete |
| **TOTAL** | | **29-39 weeks** | **~21,700** | |

### Critical Path

```
v0.2.0 ──► v0.3.0 ──► v0.4.0 ──► v0.5.0 ──► v0.6.0 ──► v0.7.0 ──► v0.8.0 ──► v1.0.0
  │          │          │          │          │          │          │
  └──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
            Each milestone depends on the previous
```

### Parallel Work Opportunities

- **Art assets** can be created alongside any milestone
- **Sound design** can start during v0.5.0
- **Champion design** (on paper) can start during v0.4.0
- **Map design** can start during v0.5.0
- **Testing infrastructure** can be built incrementally

---

## Appendix: File Structure at v1.0.0

```
src/
├── arena/
│   ├── alloc/          # Arena allocator (exists)
│   ├── collections/    # Array, HashMap (partial)
│   ├── math/           # Vec3, Mat4, Quat (v0.2.0)
│   ├── ecs/            # Entity-component system (v0.3.0)
│   │   ├── components/ # All component types
│   │   └── systems/    # All systems
│   ├── physics/        # Collision, quadtree (v0.4.0, v1.0.0)
│   ├── ai/             # Pathfinding, minion AI (v0.6.0)
│   ├── network/        # Protocol, sockets (v0.5.0)
│   ├── world/          # Map, fog of war (v0.6.0)
│   ├── game/           # Champion, item, economy (v0.7.0)
│   └── replay/         # Recording/playback (v1.0.0)
├── renderer/
│   ├── vk_*/           # Vulkan infrastructure (v0.2.0)
│   ├── window/         # GLFW window (v0.2.0)
│   ├── sprite/         # 2D rendering (v0.3.0)
│   ├── particles/      # Particle system (v0.8.0)
│   ├── animation/      # Skeletal animation (v0.8.0)
│   └── ui/             # UI framework (v0.8.0)
├── audio/              # Audio system (v0.8.0)
├── client/
│   ├── main.c          # Client entry (exists)
│   ├── network_client/ # Client networking (v0.5.0)
│   ├── prediction/     # Client-side prediction (v0.5.0)
│   ├── ui/             # Game UI screens (v0.8.0)
│   ├── settings/       # Configuration (v1.0.0)
│   └── tutorial/       # Tutorial (v1.0.0)
└── server/
    ├── main.c          # Server entry (exists)
    ├── network_server/ # Server networking (v0.5.0)
    ├── game_server/    # Game loop (v0.5.0)
    ├── matchmaking/    # Matchmaking (v1.0.0)
    └── anticheat/      # Validation (v1.0.0)
```
