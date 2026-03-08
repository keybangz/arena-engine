# Arena Engine - Technical Specifications

**Version:** 0.8.0
**Last Updated:** 2026-03-08
**Target:** C11, Vulkan 1.3, Linux/Windows

---

## Table of Contents

1. [ECS Architecture](#1-ecs-architecture)
   - 1.7 [3D Components (v0.8.0+)](#17-3d-components-v080)
2. [Network Protocol](#2-network-protocol)
3. [Renderer Pipeline](#3-renderer-pipeline)
   - 3.5 [3D Render Pass Organization (v0.8.0+)](#35-render-pass-organization-3d---v080)
   - 3.6 [3D Mesh Structures (v0.8.0+)](#36-3d-mesh-structures-v080)
   - 3.7 [PBR Material System (v0.8.0+)](#37-pbr-material-system-v080)
   - 3.8 [glTF Loader (v0.8.0+)](#38-gltf-loader-v080)
4. [Asset Formats](#4-asset-formats)
5. [Game State Structure](#5-game-state-structure)

---

## 1. ECS Architecture

### 1.1 Design Philosophy

Arena Engine uses a **Sparse Set ECS** architecture optimized for:
- Cache-friendly iteration over components
- Fast entity lookup by ID
- Safe entity handle validation via generational IDs
- Deterministic system execution for network replay

### 1.2 Entity Representation

Entities are represented as 64-bit handles combining an index and generation counter for ABA problem prevention.

```c
// src/arena/ecs/ecs_types.h

#include <stdint.h>
#include <stdbool.h>

// Entity handle: 32-bit index + 32-bit generation
typedef struct EntityId {
    uint32_t index;      // Slot in entity array
    uint32_t generation; // Incremented on reuse, detects stale handles
} EntityId;

#define ENTITY_NULL ((EntityId){.index = UINT32_MAX, .generation = 0})

static inline bool entity_is_null(EntityId id) {
    return id.index == UINT32_MAX;
}

static inline bool entity_equals(EntityId a, EntityId b) {
    return a.index == b.index && a.generation == b.generation;
}

static inline uint64_t entity_to_u64(EntityId id) {
    return ((uint64_t)id.generation << 32) | id.index;
}

static inline EntityId entity_from_u64(uint64_t packed) {
    return (EntityId){
        .index = (uint32_t)(packed & 0xFFFFFFFF),
        .generation = (uint32_t)(packed >> 32)
    };
}
```

### 1.3 Entity Manager

```c
// src/arena/ecs/entity_manager.h

#define ENTITY_MAX_COUNT 65536

typedef struct EntitySlot {
    uint32_t generation;  // Current generation for this slot
    bool     alive;       // Is entity currently alive?
    uint64_t archetype;   // Bitmask of component types present
} EntitySlot;

typedef struct EntityManager {
    EntitySlot  slots[ENTITY_MAX_COUNT];
    uint32_t    free_indices[ENTITY_MAX_COUNT];
    uint32_t    free_count;
    uint32_t    entity_count;
    uint32_t    high_watermark;  // Highest index ever used
} EntityManager;

// API
void         entity_manager_init(EntityManager* em);
EntityId     entity_create(EntityManager* em);
void         entity_destroy(EntityManager* em, EntityId id);
bool         entity_is_alive(const EntityManager* em, EntityId id);
uint32_t     entity_count(const EntityManager* em);
```

### 1.4 Component Storage (Sparse Set)

Each component type uses a sparse set for O(1) lookup and cache-efficient iteration.

```c
// src/arena/ecs/component_storage.h

#define COMPONENT_TYPE_MAX 64

typedef uint64_t ComponentMask;  // Bitmask for component presence

typedef struct SparseSet {
    uint32_t* sparse;       // Entity index -> dense index (size: ENTITY_MAX_COUNT)
    uint32_t* dense;        // Dense array of entity indices
    void*     components;   // Contiguous component data
    uint32_t  count;        // Number of components stored
    uint32_t  capacity;     // Allocated capacity
    size_t    comp_size;    // Size of single component in bytes
    uint16_t  comp_type_id; // Component type identifier
} SparseSet;

// Sparse set operations
void  sparse_set_init(SparseSet* set, size_t comp_size, uint16_t type_id, Arena* arena);
void  sparse_set_destroy(SparseSet* set);
void* sparse_set_add(SparseSet* set, uint32_t entity_index);
void* sparse_set_get(const SparseSet* set, uint32_t entity_index);
void  sparse_set_remove(SparseSet* set, uint32_t entity_index);
bool  sparse_set_contains(const SparseSet* set, uint32_t entity_index);

// Iteration (returns contiguous array)
void* sparse_set_data(SparseSet* set);
uint32_t sparse_set_count(const SparseSet* set);
```

### 1.5 Component Registry

```c
// src/arena/ecs/component_registry.h

typedef void (*ComponentInitFn)(void* component);
typedef void (*ComponentDestroyFn)(void* component);
typedef void (*ComponentCopyFn)(void* dst, const void* src);

typedef struct ComponentInfo {
    const char*         name;
    size_t              size;
    size_t              alignment;
    ComponentInitFn     init;
    ComponentDestroyFn  destroy;
    ComponentCopyFn     copy;
    bool                networked;     // Synced over network
    bool                serialized;    // Saved to disk
} ComponentInfo;

typedef struct ComponentRegistry {
    ComponentInfo info[COMPONENT_TYPE_MAX];
    SparseSet     storage[COMPONENT_TYPE_MAX];
    uint16_t      type_count;
} ComponentRegistry;

// Registration (call at startup)
uint16_t component_register(ComponentRegistry* reg, const ComponentInfo* info);

// Macros for type-safe component access
#define COMPONENT_ID(name) COMP_ID_##name
#define DECLARE_COMPONENT(name, type) \
    extern uint16_t COMP_ID_##name; \
    static inline type* get_##name(const ComponentRegistry* reg, uint32_t idx) { \
        return (type*)sparse_set_get(&reg->storage[COMP_ID_##name], idx); \
    }
```

### 1.6 Core Game Components

```c
// src/arena/ecs/components.h

typedef struct Transform {
    float position[3];   // World position (x, y, z)
    float rotation;      // Rotation in radians (2D gameplay, Y-up)
    float scale[2];      // Non-uniform 2D scale
} Transform;

typedef struct Velocity {
    float linear[3];     // Linear velocity m/s
    float angular;       // Angular velocity rad/s
} Velocity;

typedef struct Sprite {
    uint32_t texture_id; // Handle to sprite sheet
    uint16_t frame_x;    // Current frame column
    uint16_t frame_y;    // Current frame row
    uint16_t frame_w;    // Frame width in pixels
    uint16_t frame_h;    // Frame height in pixels
    uint32_t color;      // RGBA tint
    int16_t  layer;      // Render layer (higher = on top)
    uint16_t flags;      // SPRITE_FLIP_X, SPRITE_FLIP_Y, etc.
} Sprite;

#define SPRITE_FLIP_X    (1 << 0)
#define SPRITE_FLIP_Y    (1 << 1)
#define SPRITE_VISIBLE   (1 << 2)
#define SPRITE_BILLBOARD (1 << 3)

typedef struct Collider {
    float    half_extents[2]; // AABB half-size
    float    offset[2];       // Offset from transform
    uint16_t layer;           // Collision layer bitmask
    uint16_t mask;            // Layers this collides with
    uint8_t  type;            // COLLIDER_BOX, COLLIDER_CIRCLE
    uint8_t  flags;           // COLLIDER_TRIGGER, COLLIDER_STATIC
} Collider;

typedef struct Health {
    float current;
    float maximum;
    float regen_rate;         // HP per second
    float shield;             // Absorbs damage first
    float shield_max;
} Health;

typedef struct Champion {
    uint32_t champion_id;     // Champion definition ID
    uint8_t  level;
    uint32_t experience;
    uint32_t gold;
    float    ability_cooldowns[4];
    uint8_t  ability_levels[4];
    uint32_t items[6];        // Item IDs in slots
} Champion;

typedef struct AIController {
    uint8_t  state;           // AI state machine state
    EntityId target;          // Current target entity
    float    state_timer;     // Time in current state
    float    aggro_range;
    float    attack_range;
    uint8_t  behavior_tree;   // Behavior tree ID
} AIController;

typedef struct NetworkIdentity {
    uint32_t net_id;          // Unique network ID
    uint8_t  owner_player;    // Owning player slot (0-9)
    uint8_t  authority;       // SERVER_AUTH, CLIENT_AUTH, REPLICATED
    uint16_t update_priority; // Higher = more frequent updates
} NetworkIdentity;

// Component type IDs (registered at startup)
DECLARE_COMPONENT(Transform, Transform)
DECLARE_COMPONENT(Velocity, Velocity)
DECLARE_COMPONENT(Sprite, Sprite)
DECLARE_COMPONENT(Collider, Collider)
DECLARE_COMPONENT(Health, Health)
DECLARE_COMPONENT(Champion, Champion)
DECLARE_COMPONENT(AIController, AIController)
DECLARE_COMPONENT(NetworkIdentity, NetworkIdentity)
```

### 1.7 3D Components (v0.8.0+)

```c
// src/arena/ecs/components_3d.h

// Full 3D transform replacing 2D Transform for 3D entities
typedef struct Transform3D {
    Vec3 position;           // World position
    Quat rotation;           // Quaternion rotation
    Vec3 scale;              // Non-uniform scale
    Mat4 local_matrix;       // Cached T * R * S
    Mat4 world_matrix;       // Parent chain applied
    bool dirty;              // Needs recalculation
} Transform3D;

// Static 3D mesh rendering
typedef struct MeshRenderer {
    uint32_t mesh_id;        // Handle to GPU mesh
    uint32_t material_id;    // Handle to PBR material
    Vec3 bounds_min;         // AABB for culling
    Vec3 bounds_max;
    float bounds_radius;     // Bounding sphere
    uint8_t layer;           // Render layer (opaque, transparent)
    bool cast_shadows;
    bool receive_shadows;
    bool visible;
} MeshRenderer;

// Scene camera
typedef enum CameraProjection {
    CAMERA_PERSPECTIVE,
    CAMERA_ORTHOGRAPHIC
} CameraProjection;

typedef struct Camera {
    CameraProjection projection;
    float fov;               // Vertical FOV (radians)
    float near_plane;
    float far_plane;
    float ortho_size;        // Half-height for orthographic
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection;
    Vec4 frustum_planes[6];  // For culling
    bool is_active;
    uint8_t priority;
} Camera;

// Scene lighting
typedef enum LightType {
    LIGHT_DIRECTIONAL,       // Sun/moon
    LIGHT_POINT,             // Torches, explosions
    LIGHT_SPOT               // Focused beams
} LightType;

typedef struct Light {
    LightType type;
    Vec3 color;              // RGB, HDR allowed
    float intensity;
    float range;             // Point/spot only
    float attenuation;
    float inner_cone;        // Spot only
    float outer_cone;
    bool cast_shadows;
    uint32_t shadow_map_id;
} Light;

// Skeletal animation
#define MAX_BONES_PER_MESH 64

typedef struct SkinnedMesh {
    uint32_t skeleton_id;    // Skeleton asset handle
    Mat4 bone_matrices[MAX_BONES_PER_MESH];
    uint32_t bone_count;
    uint32_t current_anim_id;
    float anim_time;
    float anim_speed;
    bool anim_looping;
    uint32_t blend_from_anim;
    float blend_factor;      // 0 = from, 1 = current
    float blend_duration;
} SkinnedMesh;

// Component type IDs (3D - registered at startup)
DECLARE_COMPONENT(Transform3D, Transform3D)
DECLARE_COMPONENT(MeshRenderer, MeshRenderer)
DECLARE_COMPONENT(Camera, Camera)
DECLARE_COMPONENT(Light, Light)
DECLARE_COMPONENT(SkinnedMesh, SkinnedMesh)
```

### 1.8 System Interface

Systems are functions with declared component dependencies, executed in deterministic order.

```c
// src/arena/ecs/system.h

typedef struct World World;

typedef enum SystemPhase {
    PHASE_INPUT,        // Process input
    PHASE_PRE_UPDATE,   // Pre-physics logic
    PHASE_PHYSICS,      // Physics simulation
    PHASE_POST_PHYSICS, // Post-physics reactions
    PHASE_UPDATE,       // Game logic
    PHASE_RENDER_PREP,  // Prepare render data
    PHASE_COUNT
} SystemPhase;

typedef struct SystemQuery {
    ComponentMask required;   // Must have all these components
    ComponentMask excluded;   // Must NOT have any of these
    ComponentMask write;      // Components modified (for scheduling)
} SystemQuery;

typedef void (*SystemFn)(World* world, float delta_time);

typedef struct SystemInfo {
    const char*  name;
    SystemFn     function;
    SystemPhase  phase;
    SystemQuery  query;
    int32_t      priority;    // Within phase (lower = earlier)
    bool         server_only; // Only runs on server
    bool         client_only; // Only runs on client
} SystemInfo;

typedef struct SystemRegistry {
    SystemInfo systems[256];
    uint16_t   system_count;
    uint16_t   phase_start[PHASE_COUNT + 1]; // Index into sorted array
} SystemRegistry;

void system_register(SystemRegistry* reg, const SystemInfo* info);
void system_sort(SystemRegistry* reg); // Call after all registrations
void system_run_phase(SystemRegistry* reg, World* world, SystemPhase phase, float dt);
```

### 1.8 Query API

```c
// src/arena/ecs/query.h

typedef struct QueryIterator {
    World*        world;
    ComponentMask required;
    ComponentMask excluded;
    uint32_t      current_index;  // Index into dense array
    uint32_t      primary_type;   // Smallest component set to iterate
} QueryIterator;

// Query macros for type-safe iteration
#define ECS_QUERY(world, ...) \
    query_iter_init(world, MASK(__VA_ARGS__), 0)

#define ECS_QUERY_EXCLUDE(world, req, exc) \
    query_iter_init(world, req, exc)

// Low-level query API
QueryIterator query_iter_init(World* world, ComponentMask required, ComponentMask excluded);
bool          query_iter_next(QueryIterator* iter);
EntityId      query_iter_entity(const QueryIterator* iter);

// Bulk access during iteration
void* query_get_component(const QueryIterator* iter, uint16_t comp_type);

// Example usage:
// QueryIterator iter = ECS_QUERY(world, COMP_ID_Transform, COMP_ID_Velocity);
// while (query_iter_next(&iter)) {
//     Transform* t = query_get_component(&iter, COMP_ID_Transform);
//     Velocity* v = query_get_component(&iter, COMP_ID_Velocity);
//     t->position[0] += v->linear[0] * dt;
// }
```

### 1.9 World (ECS Container)

```c
// src/arena/ecs/world.h

typedef struct World {
    EntityManager     entities;
    ComponentRegistry components;
    SystemRegistry    systems;
    Arena*            frame_arena;    // Reset each frame
    Arena*            persistent;     // Lives for match duration
    uint32_t          tick;           // Current simulation tick
    float             tick_rate;      // Ticks per second (default: 60)
    float             accumulator;    // Fixed timestep accumulator
} World;

World*   world_create(Arena* persistent);
void     world_destroy(World* world);
void     world_update(World* world, float delta_time);

// Entity operations
EntityId world_spawn(World* world);
void     world_despawn(World* world, EntityId entity);

// Component operations
void*    world_add_component(World* world, EntityId entity, uint16_t comp_type);
void*    world_get_component(World* world, EntityId entity, uint16_t comp_type);
void     world_remove_component(World* world, EntityId entity, uint16_t comp_type);
bool     world_has_component(World* world, EntityId entity, uint16_t comp_type);
```

---

## 2. Network Protocol

### 2.1 Transport Layer

UDP with application-level reliability for critical packets.

```c
// src/arena/network/transport.h

#define NET_MAX_PACKET_SIZE   1400  // Below MTU
#define NET_MAX_CLIENTS       10
#define NET_TICK_RATE         60    // Server ticks per second
#define NET_SNAPSHOT_RATE     20    // Snapshots per second
#define NET_PROTOCOL_VERSION  1
#define NET_PROTOCOL_MAGIC    0x4152454E  // "AREN"

typedef enum PacketFlags {
    PACKET_RELIABLE   = (1 << 0),  // Requires acknowledgment
    PACKET_ORDERED    = (1 << 1),  // Must be processed in order
    PACKET_COMPRESSED = (1 << 2),  // Payload is LZ4 compressed
    PACKET_ENCRYPTED  = (1 << 3),  // Payload is encrypted
} PacketFlags;
```

### 2.2 Packet Header

```c
// src/arena/network/packet.h

typedef struct PacketHeader {
    uint32_t magic;          // NET_PROTOCOL_MAGIC
    uint16_t protocol_ver;   // Protocol version
    uint16_t packet_type;    // PacketType enum
    uint32_t sequence;       // Packet sequence number
    uint32_t ack;            // Last received sequence from peer
    uint32_t ack_bits;       // Bitfield for previous 32 packets
    uint16_t payload_size;   // Size of payload after header
    uint16_t flags;          // PacketFlags
} PacketHeader;

_Static_assert(sizeof(PacketHeader) == 24, "PacketHeader must be 24 bytes");
```

### 2.3 Packet Types

```c
typedef enum PacketType {
    // Connection management
    PKT_HANDSHAKE_REQUEST   = 0x01,
    PKT_HANDSHAKE_RESPONSE  = 0x02,
    PKT_HANDSHAKE_COMPLETE  = 0x03,
    PKT_DISCONNECT          = 0x04,
    PKT_HEARTBEAT           = 0x05,

    // Game data
    PKT_INPUT               = 0x10,  // Client -> Server
    PKT_STATE_SNAPSHOT      = 0x11,  // Server -> Client
    PKT_STATE_DELTA         = 0x12,  // Server -> Client (delta compressed)
    PKT_EVENT               = 0x13,  // Bidirectional
    PKT_ENTITY_SPAWN        = 0x14,  // Server -> Client
    PKT_ENTITY_DESPAWN      = 0x15,  // Server -> Client

    // Match management
    PKT_MATCH_START         = 0x20,
    PKT_MATCH_END           = 0x21,
    PKT_PLAYER_JOIN         = 0x22,
    PKT_PLAYER_LEAVE        = 0x23,
    PKT_CHAMPION_SELECT     = 0x24,

    // Reliability layer
    PKT_ACK                 = 0xFE,
    PKT_FRAGMENT            = 0xFF,
} PacketType;
```

### 2.4 Handshake Packets

```c
typedef struct HandshakeRequest {
    PacketHeader header;
    uint64_t     client_nonce;
    uint32_t     client_version;
    char         player_name[32];
} HandshakeRequest;

typedef struct HandshakeResponse {
    PacketHeader header;
    uint64_t     client_nonce;      // Echo back
    uint64_t     server_nonce;
    uint8_t      player_slot;       // Assigned player ID (0-9)
    uint8_t      result;            // 0=success, non-zero=error code
    uint16_t     server_tick_rate;
    uint32_t     current_tick;
} HandshakeResponse;

typedef enum HandshakeResult {
    HANDSHAKE_OK              = 0,
    HANDSHAKE_SERVER_FULL     = 1,
    HANDSHAKE_VERSION_MISMATCH = 2,
    HANDSHAKE_BANNED          = 3,
    HANDSHAKE_MATCH_IN_PROGRESS = 4,
} HandshakeResult;
```

### 2.5 Input Packet

Clients send input commands at tick rate. Server buffers and applies deterministically.

```c
typedef struct InputCommand {
    uint32_t tick;            // Target server tick
    uint8_t  buttons;         // INPUT_MOVE, INPUT_ATTACK, etc.
    uint8_t  ability_slot;    // 0-3 for Q/W/E/R abilities
    int16_t  move_x;          // Move direction X (fixed-point, /32767 = -1 to 1)
    int16_t  move_y;          // Move direction Y
    float    target_x;        // World position for abilities
    float    target_y;
    EntityId target_entity;   // Target entity (if ability targets entity)
} InputCommand;

#define INPUT_MOVE       (1 << 0)
#define INPUT_ATTACK     (1 << 1)
#define INPUT_ABILITY    (1 << 2)
#define INPUT_STOP       (1 << 3)
#define INPUT_RECALL     (1 << 4)

typedef struct InputPacket {
    PacketHeader header;
    uint8_t      command_count;     // Number of commands (1-8)
    uint8_t      reserved[3];
    InputCommand commands[8];       // Redundant sends for reliability
} InputPacket;
```

### 2.6 State Snapshot

Server sends periodic world state snapshots.

```c
typedef struct EntitySnapshot {
    uint32_t net_id;
    uint32_t champion_id;     // 0 if not a champion
    float    position[2];
    float    rotation;
    float    velocity[2];
    float    health;
    float    health_max;
    uint8_t  team;
    uint8_t  state_flags;     // ENTITY_DEAD, ENTITY_STUNNED, etc.
    uint16_t animation_state;
} EntitySnapshot;

typedef struct StateSnapshot {
    PacketHeader    header;
    uint32_t        server_tick;
    uint32_t        last_input_tick;  // Last processed input from this client
    uint16_t        entity_count;
    uint16_t        reserved;
    EntitySnapshot  entities[];       // Variable length
} StateSnapshot;

#define SNAPSHOT_MAX_ENTITIES  256
```

### 2.7 Event Packet

Discrete game events (damage dealt, ability cast, etc.)

```c
typedef enum GameEventType {
    EVENT_DAMAGE             = 0x01,
    EVENT_HEAL               = 0x02,
    EVENT_DEATH              = 0x03,
    EVENT_RESPAWN            = 0x04,
    EVENT_ABILITY_CAST       = 0x05,
    EVENT_ABILITY_HIT        = 0x06,
    EVENT_BUFF_APPLY         = 0x07,
    EVENT_BUFF_REMOVE        = 0x08,
    EVENT_ITEM_PURCHASE      = 0x09,
    EVENT_LEVEL_UP           = 0x0A,
    EVENT_OBJECTIVE_CAPTURE  = 0x0B,
    EVENT_TURRET_DESTROY     = 0x0C,
    EVENT_CHAT_MESSAGE       = 0x20,
    EVENT_PING               = 0x21,
} GameEventType;

typedef struct GameEvent {
    uint16_t event_type;
    uint16_t data_size;
    uint32_t tick;
    uint32_t source_entity;
    uint32_t target_entity;
    uint8_t  data[48];        // Event-specific payload
} GameEvent;

typedef struct EventPacket {
    PacketHeader header;
    uint8_t      event_count;
    uint8_t      reserved[3];
    GameEvent    events[];    // Variable length
} EventPacket;
```

### 2.8 Connection State

```c
// src/arena/network/connection.h

typedef enum ConnectionState {
    CONN_DISCONNECTED,
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_DISCONNECTING,
} ConnectionState;

typedef struct Connection {
    ConnectionState state;
    uint32_t        addr;            // IPv4 address
    uint16_t        port;
    uint8_t         player_slot;
    uint8_t         reserved;

    uint32_t        local_sequence;
    uint32_t        remote_sequence;
    uint32_t        ack_bits;

    double          last_send_time;
    double          last_recv_time;
    float           rtt;             // Round-trip time estimate
    float           rtt_variance;

    // Reliability tracking
    struct {
        uint32_t sequence;
        double   send_time;
        uint16_t size;
        uint8_t  data[NET_MAX_PACKET_SIZE];
    } pending_acks[256];
    uint8_t         pending_count;
} Connection;
```

### 2.9 Bandwidth Estimates

| Direction | Data Type | Rate | Size | Bandwidth |
|-----------|-----------|------|------|-----------|
| Client→Server | Input | 60/sec | ~48 bytes | ~3 KB/s |
| Server→Client | Snapshot | 20/sec | ~2 KB | ~40 KB/s |
| Server→Client | Events | Variable | ~100 bytes | ~5 KB/s |
| **Total Client Upload** | | | | **~5 KB/s** |
| **Total Client Download** | | | | **~50 KB/s** |

Delta compression can reduce snapshot bandwidth by ~60%.

---

## 3. Renderer Pipeline

### 3.1 Initialization Sequence

```c
// src/renderer/renderer.h

typedef struct RendererConfig {
    const char* app_name;
    uint32_t    width;
    uint32_t    height;
    bool        vsync;
    bool        validation_layers;
    uint32_t    max_sprites;        // Default: 65536
    uint32_t    max_textures;       // Default: 256
} RendererConfig;

typedef struct Renderer {
    // Vulkan core
    VkInstance               instance;
    VkPhysicalDevice         physical_device;
    VkDevice                 device;
    VkSurfaceKHR             surface;
    VkSwapchainKHR           swapchain;

    // Queues
    VkQueue                  graphics_queue;
    VkQueue                  present_queue;
    uint32_t                 graphics_family;
    uint32_t                 present_family;

    // Swapchain images
    VkImage*                 swapchain_images;
    VkImageView*             swapchain_views;
    uint32_t                 swapchain_image_count;
    VkFormat                 swapchain_format;
    VkExtent2D               swapchain_extent;

    // Render passes
    VkRenderPass             main_pass;
    VkRenderPass             ui_pass;
    VkFramebuffer*           framebuffers;

    // Pipelines
    VkPipelineLayout         sprite_layout;
    VkPipeline               sprite_pipeline;
    VkPipeline               terrain_pipeline;
    VkPipeline               fog_pipeline;
    VkPipeline               ui_pipeline;

    // Descriptors
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    sprite_set_layout;
    VkDescriptorSet          sprite_sets[3];  // Triple buffered

    // Synchronization
    VkSemaphore              image_available[3];
    VkSemaphore              render_finished[3];
    VkFence                  in_flight[3];
    uint32_t                 current_frame;

    // Resources
    SpriteRenderer*          sprites;
    TextureManager*          textures;
    BufferPool*              buffers;

    Arena*                   arena;
} Renderer;

// Initialization
RendererResult renderer_init(Renderer* r, RendererConfig* config, GLFWwindow* window);
void           renderer_shutdown(Renderer* r);
RendererResult renderer_recreate_swapchain(Renderer* r, uint32_t width, uint32_t height);
```

### 3.2 Frame Structure

```c
typedef struct FrameData {
    VkCommandBuffer cmd;
    VkSemaphore     image_available;
    VkSemaphore     render_finished;
    VkFence         in_flight;
    uint32_t        image_index;

    // Per-frame resources (reset each frame)
    VkBuffer        sprite_buffer;
    VkDeviceMemory  sprite_memory;
    void*           sprite_mapped;
    uint32_t        sprite_count;
} FrameData;

// Frame lifecycle
RendererResult renderer_begin_frame(Renderer* r, FrameData* frame);
void           renderer_end_frame(Renderer* r, FrameData* frame);

// Called between begin/end
void renderer_begin_render_pass(Renderer* r, FrameData* frame, VkRenderPass pass);
void renderer_end_render_pass(Renderer* r, FrameData* frame);
```

### 3.3 Sprite Renderer

```c
// src/renderer/sprite_renderer.h

typedef struct SpriteVertex {
    float position[2];
    float uv[2];
    uint32_t color;      // RGBA packed
} SpriteVertex;

typedef struct SpriteInstance {
    float    transform[4]; // 2D transform (position.xy, scale.xy)
    float    rotation;
    uint32_t texture_id;
    float    uv_rect[4];   // UV coordinates (u, v, width, height)
    uint32_t color;
    int16_t  layer;
    uint16_t flags;
} SpriteInstance;

typedef struct SpriteBatch {
    uint32_t        texture_id;
    uint32_t        start_index;
    uint32_t        count;
} SpriteBatch;

typedef struct SpriteRenderer {
    SpriteInstance* instances;
    uint32_t        instance_count;
    uint32_t        instance_capacity;

    SpriteBatch*    batches;
    uint32_t        batch_count;

    VkBuffer        vertex_buffer;
    VkBuffer        instance_buffer;
    VkDeviceMemory  buffer_memory;
} SpriteRenderer;

void sprite_renderer_init(SpriteRenderer* sr, Renderer* r, uint32_t max_sprites);
void sprite_renderer_begin(SpriteRenderer* sr);
void sprite_renderer_submit(SpriteRenderer* sr, const SpriteInstance* sprite);
void sprite_renderer_flush(SpriteRenderer* sr, FrameData* frame, Renderer* r);
```

### 3.4 Render Pass Organization (2D Legacy)

```
Main Pass (2D):
├── Terrain Layer (ground textures, tiled)
├── Floor Decals (ability effects, blood)
├── Entities Layer (sorted by Y, back-to-front)
│   ├── Champions
│   ├── Minions
│   ├── Projectiles
│   └── Effects
├── Fog of War (screen-space, visibility mask)
└── World UI (health bars, nameplates)

UI Pass:
├── HUD (ability bar, minimap, stats)
├── Menus
└── Debug Overlay
```

### 3.5 Render Pass Organization (3D - v0.8.0+)

```
┌─────────────────────────────────────────────────────────────┐
│                    3D RENDER FRAME                           │
├─────────────────────────────────────────────────────────────┤
│ Pass 1: Shadow Map Pass                                      │
│   └── Depth-only render from directional light POV          │
│                                                              │
│ Pass 2: Opaque Pass                                          │
│   ├── 3D Terrain mesh                                        │
│   ├── Static props (depth test on, depth write on)          │
│   └── Characters (skinned meshes)                           │
│                                                              │
│ Pass 3: Transparent Pass                                     │
│   ├── VFX/Particles (depth test on, depth write off)        │
│   └── Transparent meshes (sorted back-to-front)             │
│                                                              │
│ Pass 4: UI Overlay Pass                                      │
│   ├── Health bars (world-space, billboarded)                │
│   └── 2D HUD (existing quad pipeline, depth test off)       │
└─────────────────────────────────────────────────────────────┘
```

### 3.6 3D Mesh Structures (v0.8.0+)

```c
// src/renderer/mesh.h

typedef struct Vertex3D {
    float position[3];
    float normal[3];
    float tangent[4];    // xyz + handedness
    float uv[2];
    uint8_t bone_ids[4];    // For skinned meshes
    float bone_weights[4];
} Vertex3D;

typedef struct Mesh {
    VkBuffer        vertex_buffer;
    VkBuffer        index_buffer;
    VkDeviceMemory  memory;
    uint32_t        vertex_count;
    uint32_t        index_count;
    Vec3            bounds_min;
    Vec3            bounds_max;
    float           bounds_radius;
} Mesh;

typedef struct MeshManager {
    Mesh*           meshes;
    uint32_t        mesh_count;
    uint32_t        mesh_capacity;
    uint32_t*       free_list;
    uint32_t        free_count;
} MeshManager;

MeshHandle mesh_load_gltf(MeshManager* mm, Renderer* r, const char* path);
void       mesh_unload(MeshManager* mm, MeshHandle handle);
```

### 3.7 PBR Material System (v0.8.0+)

```c
// src/renderer/material.h

typedef struct Material {
    Vec4    base_color;          // RGB + alpha
    float   metallic;
    float   roughness;
    float   normal_scale;
    float   ao_strength;

    uint32_t albedo_texture;     // Texture handles
    uint32_t normal_texture;
    uint32_t metallic_roughness_texture;
    uint32_t ao_texture;
    uint32_t emissive_texture;

    Vec3    emissive_color;
    float   emissive_strength;

    bool    double_sided;
    uint8_t alpha_mode;          // OPAQUE, MASK, BLEND
    float   alpha_cutoff;
} Material;

typedef struct MaterialManager {
    Material*       materials;
    uint32_t        material_count;
    VkDescriptorSet descriptor_sets[256]; // Per-material descriptors
} MaterialManager;
```

### 3.8 glTF Loader (v0.8.0+)

```c
// src/renderer/gltf_loader.h

typedef struct GLTFScene {
    Mesh*       meshes;
    uint32_t    mesh_count;
    Material*   materials;
    uint32_t    material_count;
    Skeleton*   skeleton;        // Optional
    Animation*  animations;
    uint32_t    animation_count;
} GLTFScene;

GLTFScene* gltf_load(const char* path, Arena* arena);
void       gltf_free(GLTFScene* scene);
```

### 3.5 Shader Interface

```c
// Uniform buffer for camera/projection
typedef struct CameraUBO {
    float view[16];
    float projection[16];
    float viewport[4];    // x, y, width, height
    float time;
    float reserved[3];
} CameraUBO;

// Push constants for sprite rendering
typedef struct SpritePushConstants {
    float    transform[4]; // position.xy, scale.xy
    float    rotation;
    uint32_t texture_idx;
    float    uv_rect[4];
    uint32_t tint;
} SpritePushConstants;
```

### 3.6 Resource Management

```c
// src/renderer/texture_manager.h

typedef struct TextureHandle {
    uint32_t id;
    uint32_t generation;
} TextureHandle;

typedef struct Texture {
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
    uint32_t       width;
    uint32_t       height;
    uint32_t       generation;
    bool           in_use;
} Texture;

typedef struct TextureManager {
    Texture*     textures;
    uint32_t     texture_count;
    uint32_t     texture_capacity;
    uint32_t*    free_list;
    uint32_t     free_count;
    VkSampler    default_sampler;
    VkSampler    pixel_sampler;  // Nearest-neighbor
} TextureManager;

TextureHandle texture_load(TextureManager* tm, Renderer* r, const char* path);
TextureHandle texture_load_memory(TextureManager* tm, Renderer* r,
                                  const void* data, uint32_t w, uint32_t h);
void          texture_unload(TextureManager* tm, TextureHandle handle);
Texture*      texture_get(TextureManager* tm, TextureHandle handle);
```

---

## 4. Asset Formats

### 4.1 Sprite Sheet Format (.arena_sprite)

Binary format for sprite sheets with embedded frame data.

```c
// tools/asset-packer/sprite_format.h

#define ARENA_SPRITE_MAGIC  0x53505254  // "SPRT"
#define ARENA_SPRITE_VERSION 1

typedef struct ArenaSpriteHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_offset;     // Offset to pixel data
    uint32_t texture_size;       // Compressed size
    uint32_t frame_count;
    uint32_t animation_count;
} ArenaSpriteHeader;

typedef struct SpriteFrame {
    uint16_t x, y;               // Position in texture
    uint16_t width, height;      // Frame dimensions
    int16_t  pivot_x, pivot_y;   // Pivot point offset
    int16_t  offset_x, offset_y; // Draw offset
} SpriteFrame;

typedef struct SpriteAnimation {
    char     name[32];
    uint16_t first_frame;
    uint16_t frame_count;
    float    frame_duration;     // Seconds per frame
    uint8_t  loop_mode;          // LOOP, ONCE, PING_PONG
    uint8_t  reserved[3];
} SpriteAnimation;

// File layout:
// [ArenaSpriteHeader]
// [SpriteFrame * frame_count]
// [SpriteAnimation * animation_count]
// [Compressed RGBA pixel data]
```

### 4.2 Map Format (.arena_map)

```c
// tools/asset-packer/map_format.h

#define ARENA_MAP_MAGIC   0x4D415021  // "MAP!"
#define ARENA_MAP_VERSION 1

typedef struct ArenaMapHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t width;              // Map width in tiles
    uint32_t height;             // Map height in tiles
    uint32_t tile_size;          // Tile size in pixels (e.g., 64)
    uint32_t layer_count;
    uint32_t tileset_count;
    uint32_t spawn_count;
    uint32_t collision_offset;
    uint32_t navmesh_offset;
} ArenaMapHeader;

typedef struct MapLayer {
    char     name[32];
    uint8_t  type;               // TERRAIN, DECORATION, COLLISION
    uint8_t  blend_mode;
    uint16_t reserved;
    uint32_t data_offset;        // Offset to tile indices
    uint32_t data_size;
} MapLayer;

typedef struct MapTileset {
    char     path[64];           // Relative path to .arena_sprite
    uint16_t first_gid;          // First global tile ID
    uint16_t tile_count;
} MapTileset;

typedef struct MapSpawn {
    float    position[2];
    uint8_t  type;               // CHAMPION, MINION, TURRET, etc.
    uint8_t  team;               // 0 = neutral, 1 = blue, 2 = red
    uint16_t entity_id;          // Reference to entity definition
} MapSpawn;

typedef struct CollisionTile {
    uint8_t  walkable   : 1;
    uint8_t  flyable    : 1;
    uint8_t  vision_block : 1;
    uint8_t  brush      : 1;
    uint8_t  reserved   : 4;
} CollisionTile;

// File layout:
// [ArenaMapHeader]
// [MapLayer * layer_count]
// [MapTileset * tileset_count]
// [MapSpawn * spawn_count]
// [Tile data per layer]
// [CollisionTile grid at collision_offset]
// [NavMesh data at navmesh_offset]
```

### 4.3 Champion Definition (.arena_champion)

```c
// tools/asset-packer/champion_format.h

#define ARENA_CHAMPION_MAGIC   0x43484D50  // "CHMP"
#define ARENA_CHAMPION_VERSION 1

typedef struct ArenaChampionHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t champion_id;
    char     name[32];
    char     title[64];
} ArenaChampionHeader;

typedef struct ChampionStats {
    // Base stats (level 1)
    float    health;
    float    health_regen;
    float    mana;
    float    mana_regen;
    float    attack_damage;
    float    ability_power;
    float    armor;
    float    magic_resist;
    float    attack_speed;
    float    movement_speed;
    float    attack_range;

    // Per-level growth
    float    health_growth;
    float    health_regen_growth;
    float    mana_growth;
    float    mana_regen_growth;
    float    attack_damage_growth;
    float    armor_growth;
    float    magic_resist_growth;
    float    attack_speed_growth;
} ChampionStats;

typedef struct ChampionAbilityRef {
    uint32_t ability_id;         // Reference to .arena_ability
    uint8_t  slot;               // 0=Q, 1=W, 2=E, 3=R, 4=Passive
    uint8_t  unlock_level;       // Level to unlock (usually 1,1,1,6)
    uint16_t reserved;
} ChampionAbilityRef;

typedef struct ChampionVisuals {
    char     sprite_path[64];    // Path to .arena_sprite
    char     portrait_path[64];  // Path to portrait image
    uint16_t base_width;
    uint16_t base_height;
    float    collision_radius;
    float    selection_radius;
} ChampionVisuals;

// File layout:
// [ArenaChampionHeader]
// [ChampionStats]
// [ChampionAbilityRef * 5]
// [ChampionVisuals]
```

### 4.4 Ability Definition (.arena_ability)

```c
// tools/asset-packer/ability_format.h

#define ARENA_ABILITY_MAGIC   0x41424C59  // "ABLY"
#define ARENA_ABILITY_VERSION 1

typedef enum AbilityType {
    ABILITY_PASSIVE,
    ABILITY_TARGETED_UNIT,
    ABILITY_TARGETED_POINT,
    ABILITY_SKILLSHOT,
    ABILITY_AREA_OF_EFFECT,
    ABILITY_SELF_BUFF,
    ABILITY_TOGGLE,
    ABILITY_CHANNEL,
} AbilityType;

typedef enum DamageType {
    DAMAGE_PHYSICAL,
    DAMAGE_MAGICAL,
    DAMAGE_TRUE,
} DamageType;

typedef struct ArenaAbilityHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t ability_id;
    char     name[32];
    char     description[256];
} ArenaAbilityHeader;

typedef struct AbilityStats {
    uint8_t  type;               // AbilityType
    uint8_t  damage_type;        // DamageType
    uint16_t flags;              // CAN_CRIT, IGNORES_ARMOR, etc.

    float    cooldown[5];        // Cooldown per rank
    float    mana_cost[5];       // Mana cost per rank
    float    cast_time;          // Seconds
    float    range;
    float    radius;             // For AoE
    float    projectile_speed;   // For skillshots
    float    duration[5];        // For buffs/debuffs

    // Damage formula: base + (ratio * stat)
    float    base_damage[5];
    float    ad_ratio;
    float    ap_ratio;
    float    bonus_ad_ratio;
    float    max_health_ratio;
    float    missing_health_ratio;
} AbilityStats;

typedef struct AbilityEffect {
    uint8_t  effect_type;        // BUFF, DEBUFF, DAMAGE_OVER_TIME, etc.
    uint8_t  target;             // SELF, ENEMY, ALLY, AREA
    uint16_t status_flag;        // STUN, SLOW, ROOT, SILENCE, etc.
    float    duration;
    float    value;              // Effect magnitude
} AbilityEffect;

#define ABILITY_MAX_EFFECTS 8

// File layout:
// [ArenaAbilityHeader]
// [AbilityStats]
// [uint8_t effect_count]
// [AbilityEffect * effect_count]
// [Visual data (sprite refs, particle refs)]
```

---

## 5. Game State Structure

### 5.1 World State

```c
// src/arena/world/game_state.h

#define MAX_PLAYERS      10
#define MAX_TEAMS        2
#define MAX_TURRETS      22
#define MAX_INHIBITORS   6
#define MAX_MINION_WAVES 12

typedef struct GameWorld {
    // ECS
    World*              ecs;

    // Map
    uint32_t            map_id;
    CollisionTile*      collision_grid;
    uint32_t            grid_width;
    uint32_t            grid_height;
    NavMesh*            navmesh;

    // Fog of War (per-team visibility)
    uint8_t*            fog_data[MAX_TEAMS];
    uint32_t            fog_width;
    uint32_t            fog_height;

    // Spatial partitioning
    SpatialHash*        spatial_hash;

    // Physics state
    float               gravity;
} GameWorld;
```

### 5.2 Match State

```c
typedef enum MatchPhase {
    MATCH_LOBBY,
    MATCH_CHAMPION_SELECT,
    MATCH_LOADING,
    MATCH_PLAYING,
    MATCH_PAUSED,
    MATCH_ENDED,
} MatchPhase;

typedef enum ObjectiveType {
    OBJ_TURRET,
    OBJ_INHIBITOR,
    OBJ_NEXUS,
    OBJ_DRAGON,
    OBJ_BARON,
    OBJ_RIFT_HERALD,
} ObjectiveType;

typedef struct ObjectiveState {
    ObjectiveType type;
    uint8_t       team;           // Owner team (0 = neutral)
    uint8_t       alive;
    uint8_t       tier;           // For turrets: 1=outer, 2=inner, 3=inhibitor
    float         health;
    float         respawn_timer;
} ObjectiveState;

typedef struct TeamState {
    uint32_t gold_total;
    uint16_t kills;
    uint16_t deaths;
    uint8_t  turrets_destroyed;
    uint8_t  inhibitors_destroyed;
    uint8_t  dragons_killed;
    uint8_t  barons_killed;
    uint8_t  dragon_souls;        // 0-4
    uint8_t  elder_active;        // Has Elder Dragon buff
} TeamState;

typedef struct MinionWave {
    uint32_t spawn_tick;
    uint8_t  lane;                // 0=top, 1=mid, 2=bot
    uint8_t  team;
    uint8_t  type;                // REGULAR, CANNON, SUPER
    uint8_t  count;
    EntityId minions[7];          // Max minions per wave
} MinionWave;

typedef struct MatchState {
    MatchPhase      phase;
    uint32_t        tick;
    float           game_time;        // Seconds since match start
    uint8_t         winning_team;     // 0=none, 1=blue, 2=red

    TeamState       teams[MAX_TEAMS];
    ObjectiveState  turrets[MAX_TURRETS];
    ObjectiveState  inhibitors[MAX_INHIBITORS];
    ObjectiveState  nexuses[2];
    ObjectiveState  dragon;
    ObjectiveState  baron;
    ObjectiveState  herald;

    MinionWave      minion_waves[MAX_MINION_WAVES];
    uint8_t         minion_wave_count;
    float           next_wave_time;

    // Timers
    float           dragon_respawn;
    float           baron_respawn;
    float           herald_despawn;
} MatchState;
```

### 5.3 Player State

```c
typedef enum PlayerStatus {
    PLAYER_DISCONNECTED,
    PLAYER_CONNECTING,
    PLAYER_CONNECTED,
    PLAYER_LOADING,
    PLAYER_PLAYING,
    PLAYER_AFK,
} PlayerStatus;

typedef struct PlayerState {
    // Identity
    uint8_t         slot;             // 0-9
    uint8_t         team;             // 0 or 1
    PlayerStatus    status;
    char            name[32];

    // Champion
    uint32_t        champion_id;
    EntityId        champion_entity;

    // Stats
    uint16_t        kills;
    uint16_t        deaths;
    uint16_t        assists;
    uint32_t        gold;
    uint32_t        gold_earned;
    uint32_t        cs;               // Creep score
    uint32_t        damage_dealt;
    uint32_t        damage_taken;
    uint32_t        healing_done;

    // Network
    Connection*     connection;
    uint32_t        last_input_tick;
    InputCommand    input_buffer[64];
    uint8_t         input_buffer_head;
    uint8_t         input_buffer_tail;

    // Summoner spells (Flash, Ignite, etc.)
    uint8_t         summoner_spell[2];
    float           summoner_cooldown[2];
} PlayerState;

typedef struct PlayersState {
    PlayerState     players[MAX_PLAYERS];
    uint8_t         player_count;
    uint8_t         players_per_team;
} PlayersState;
```

### 5.4 Server Authoritative State

```c
// src/server/server_state.h

typedef struct ServerState {
    // Core state
    GameWorld       world;
    MatchState      match;
    PlayersState    players;

    // Tick management
    uint32_t        current_tick;
    double          tick_accumulator;
    double          last_tick_time;

    // Event queue
    GameEvent*      event_queue;
    uint32_t        event_count;
    uint32_t        event_capacity;

    // Network
    int             socket;
    Connection      connections[MAX_PLAYERS];

    // Recording (for replay)
    bool            recording;
    FILE*           replay_file;

    // Memory
    Arena*          tick_arena;       // Reset each tick
    Arena*          match_arena;      // Reset each match
    Arena*          persistent;       // Lives for server lifetime
} ServerState;

// Server lifecycle
void server_init(ServerState* s, Arena* persistent);
void server_shutdown(ServerState* s);
void server_tick(ServerState* s, double delta_time);

// Match management
void server_start_match(ServerState* s, uint32_t map_id);
void server_end_match(ServerState* s, uint8_t winning_team);
void server_pause_match(ServerState* s);
void server_resume_match(ServerState* s);
```

### 5.5 Client Predicted State

```c
// src/client/client_state.h

typedef struct ClientState {
    // Local prediction
    GameWorld       predicted_world;
    uint32_t        predicted_tick;

    // Server authoritative (interpolated)
    GameWorld       server_world;
    uint32_t        server_tick;

    // Interpolation buffer
    StateSnapshot*  snapshot_buffer;
    uint32_t        snapshot_count;
    uint32_t        snapshot_capacity;

    // Input
    InputCommand    pending_inputs[128];
    uint32_t        pending_input_count;
    uint8_t         local_player_slot;

    // Network
    Connection      connection;
    double          server_time_offset;

    // Rendering
    Renderer*       renderer;
    CameraState     camera;

    // Memory
    Arena*          frame_arena;
    Arena*          persistent;
} ClientState;

void client_init(ClientState* c, Arena* persistent);
void client_shutdown(ClientState* c);
void client_update(ClientState* c, float delta_time);
void client_render(ClientState* c);

// Prediction & reconciliation
void client_predict_input(ClientState* c, const InputCommand* input);
void client_apply_snapshot(ClientState* c, const StateSnapshot* snapshot);
void client_reconcile(ClientState* c);
```

---

## Appendix A: Constants Summary

```c
// src/arena/constants.h

// Engine limits
#define ENTITY_MAX_COUNT        65536
#define COMPONENT_TYPE_MAX      64
#define SYSTEM_MAX_COUNT        256

// Network
#define NET_MAX_PACKET_SIZE     1400
#define NET_MAX_CLIENTS         10
#define NET_TICK_RATE           60
#define NET_SNAPSHOT_RATE       20
#define NET_INPUT_BUFFER_SIZE   64
#define NET_SNAPSHOT_BUFFER     32

// Rendering
#define RENDER_MAX_SPRITES      65536
#define RENDER_MAX_TEXTURES     256
#define RENDER_FRAMES_IN_FLIGHT 3

// Gameplay
#define MAX_PLAYERS             10
#define MAX_TEAMS               2
#define MAX_ABILITIES           4
#define MAX_ITEMS               6
#define MAX_BUFFS               32
#define CHAMPION_MAX_LEVEL      18

// Map
#define MAP_TILE_SIZE           64
#define MAP_MAX_WIDTH           256
#define MAP_MAX_HEIGHT          256
#define FOG_RESOLUTION          32    // Fog cells per tile
```

---

## Appendix B: Memory Budget

| System | Allocation | Notes |
|--------|------------|-------|
| ECS Entities | 4 MB | 64K entities × 64 bytes |
| Components | 32 MB | All component pools |
| Network Buffers | 4 MB | Send/receive buffers |
| Render State | 16 MB | Sprite instances, vertex buffers |
| Textures | 256 MB | GPU-side texture memory |
| Map Data | 8 MB | Terrain, collision, navmesh |
| **Client Total** | **~320 MB** | |
| **Server Total** | **~64 MB** | No rendering |

---

## Appendix C: File Structure

```
src/
├── arena/
│   ├── ecs/
│   │   ├── ecs_types.h
│   │   ├── entity_manager.h / .c
│   │   ├── component_storage.h / .c
│   │   ├── component_registry.h / .c
│   │   ├── components.h
│   │   ├── system.h / .c
│   │   ├── query.h / .c
│   │   └── world.h / .c
│   ├── network/
│   │   ├── transport.h / .c
│   │   ├── packet.h
│   │   ├── connection.h / .c
│   │   └── reliability.h / .c
│   ├── world/
│   │   ├── game_state.h
│   │   ├── collision.h / .c
│   │   ├── navmesh.h / .c
│   │   └── fog_of_war.h / .c
│   └── ...
├── renderer/
│   ├── renderer.h / .c
│   ├── sprite_renderer.h / .c
│   ├── texture_manager.h / .c
│   └── shaders/
├── client/
│   ├── client_state.h / .c
│   ├── prediction.c
│   └── main.c
└── server/
    ├── server_state.h / .c
    ├── simulation.c
    └── main.c
```
```

