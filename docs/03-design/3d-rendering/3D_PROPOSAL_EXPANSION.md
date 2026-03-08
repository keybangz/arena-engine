# Arena Engine 3D Implementation - Research Expansion Document

**Version:** 0.8.0-EXPANSION  
**Date:** 2026-03-08  
**Status:** RESEARCH COMPLETE  
**Related:** [3D_IMPLEMENTATION_PROPOSAL.md](3D_IMPLEMENTATION_PROPOSAL.md)

---

## Executive Summary

This document expands on the 3D Implementation Proposal with detailed research findings covering movement systems, champion implementation, animation pipelines, game launcher architecture, map editor design, and replay systems. The research provides specific data, code structures, and implementation recommendations to guide development.

### Key Research Areas

| Area | Scope | Priority | Est. Effort |
|------|-------|----------|-------------|
| Movement System | LoL-accurate physics & CC | P0 | 2-3 weeks |
| Jinx Champion | First playable character | P0 | 5-6 weeks |
| Animation Pipeline | Free assets & retargeting | P0 | Ongoing |
| Game Launcher | Account, social, matchmaking | P1 | 8-10 weeks |
| 3D Map Editor | Terrain, NavMesh, objects | P1 | 8-12 weeks |
| Replay System | Recording, playback, UI | P2 | 4-6 weeks |

### Success Criteria

- [ ] Movement feels responsive and accurate to LoL standards
- [ ] Jinx is fully playable with all 5 abilities
- [ ] Animation pipeline produces game-ready assets
- [ ] Launcher supports matchmaking and social features
- [ ] Map editor enables rapid level creation
- [ ] Replay system captures and plays back full matches

---

## 1. Movement System Specification

### 1.1 Movement Speed Parameters

Based on League of Legends research (Patch 25.S1.1, 170 champions analyzed):

| Parameter | Value | Notes |
|-----------|-------|-------|
| Minimum Base MS | 305 | Kled (slowest) |
| Maximum Base MS | 355 | Master Yi, Elise |
| Average Base MS | 330-340 | Most champions |
| Boot Bonus | +45-60 | Tier 1-2 boots |
| Max Soft Cap | 415 | 80% efficiency above this |
| Max Hard Cap | 490 | 50% efficiency above this |

### 1.2 Movement Physics

```c
typedef struct MovementComponent {
    float base_speed;           // 305-355 units/sec
    float bonus_flat;           // Flat MS bonuses
    float bonus_percent;        // Percentage MS bonuses
    float slow_percent;         // Current slow amount (0.0-1.0)
    bool grounded;              // Cannot dash/blink
    bool rooted;                // Cannot move
    Vec2 velocity;              // Current velocity (instant changes)
} MovementComponent;

// LoL-style MS calculation with soft/hard caps
float calculate_movement_speed(MovementComponent* move) {
    float raw = move->base_speed + move->bonus_flat;
    raw *= (1.0f + move->bonus_percent);
    raw *= (1.0f - move->slow_percent);
    
    // Soft cap at 415
    if (raw > 415.0f) {
        raw = 415.0f + (raw - 415.0f) * 0.8f;
    }
    // Hard cap at 490
    if (raw > 490.0f) {
        raw = 490.0f + (raw - 490.0f) * 0.5f;
    }
    // Minimum 110
    return fmaxf(raw, 110.0f);
}
```

### 1.3 Crowd Control System

Seven distinct CC types identified:

| CC Type | Blocks Movement | Blocks Abilities | Blocks Attacks | Tenacity Affects |
|---------|-----------------|------------------|----------------|------------------|
| Slow | Reduces % | No | No | Yes |
| Root | Yes | Mobility only | No | Yes |
| Stun | Yes | Yes | Yes | Yes |
| Knockup | Yes | Yes | Yes | **No** |
| Knockback | Forced direction | Yes | Yes | **No** |
| Ground | No (basic) | Mobility only | No | Yes |
| Cripple | Slows attacks | No | Reduces AS | Yes |

```c
typedef enum CrowdControlType {
    CC_NONE = 0,
    CC_SLOW,        // Reduces movement speed
    CC_ROOT,        // Prevents movement, allows abilities
    CC_STUN,        // Prevents all actions
    CC_KNOCKUP,     // Airborne, prevents all actions
    CC_KNOCKBACK,   // Forced movement + prevents actions
    CC_GROUND,      // Prevents dashes/blinks
    CC_CRIPPLE,     // Reduces attack speed
} CrowdControlType;

typedef struct CrowdControl {
    CrowdControlType type;
    float duration;
    float remaining;
    float magnitude;        // Slow/cripple percentage or knockback distance
    Vec2 direction;         // For knockback
    Entity source;          // Who applied CC
    bool tenacity_affected; // Can tenacity reduce duration
} CrowdControl;
```

### 1.4 Dash & Blink Mechanics

**Dash Properties:**
- Travel time: 0.1-0.5 seconds
- Distance: 300-600 units typical
- Terrain interaction: Most traverse walls, some cannot (Vayne Q, Galio Q)
- Interrupted by: Stun, root, ground, silence, knockdown, airborne, stasis

**Blink Properties:**
- Travel time: 0 (instant)
- Terrain interaction: Ignores all terrain
- Cannot be interrupted (already completed)
- Examples: Flash, Ezreal E, Shaco R

```c
typedef struct DashComponent {
    Vec2 start_pos;
    Vec2 end_pos;
    float duration;
    float elapsed;
    bool can_cross_terrain;
    bool can_be_interrupted;
    DashState state;        // WINDING_UP, IN_PROGRESS, COMPLETE, INTERRUPTED
} DashComponent;

typedef struct BlinkComponent {
    Vec2 target_pos;
    bool ignore_terrain;    // Always true for blinks
    BlinkState state;       // CASTING, COMPLETE
} BlinkComponent;
```

### 1.5 Camera System

LoL uses a fixed isometric camera:

| Parameter | Value | Notes |
|-----------|-------|-------|
| Pitch Angle | 55-60° | From horizontal |
| Yaw Angle | 0° (fixed) | No rotation |
| Field of View | 45-50° | Perspective projection |
| Camera Distance | 800-1000 units | From target |
| Edge Scroll Speed | 1500-2000 px/sec | Screen edge panning |
| Zoom Range | 0.8x - 1.2x | Limited zoom |

### 1.6 Terrain & Collision

**Visual Heights vs Gameplay:**
- Heights are VISUAL ONLY (for rendering)
- Gameplay collision is 2D on ground plane
- Walls are vertical collision surfaces
- River is rendered lower but collision unchanged

**Brush Mechanics:**
- Grants stealth when inside
- Blocks enemy vision into brush
- Reveals if enemy enters same brush
- No effect on movement

---

## 2. Jinx Champion Design Document

### 2.1 Overview

Jinx is selected as the first champion implementation due to:
- Diverse ability types (toggle, skillshot, trap, global ultimate)
- Medium complexity (challenging but achievable)
- Clear visual identity (blue/pink, dual weapons)
- Good showcase of engine capabilities

**Estimated Implementation:** 5-6 weeks, 1,600-2,150 LOC

### 2.2 Base Statistics

| Stat | Level 1 | Per Level | Level 18 |
|------|---------|-----------|----------|
| Health | 590 | +95 | 2,205 |
| Mana | 260 | +40 | 940 |
| Attack Damage | 59 | +2.2 | 96.4 |
| Attack Speed | 0.625 | +2.1% | 0.848 |
| Movement Speed | 330 | +0 | 330 |
| Armor | 32 | +4 | 100 |
| Magic Resist | 30 | +1.25 | 51.25 |

### 2.3 Ability Specifications

#### Passive: Get Excited!

```c
typedef struct JinxPassive {
    Entity last_damaged_entity;
    float damage_window;        // 3.0 seconds
    bool active;
    float duration;             // 6.0 seconds
    float ms_bonus;             // 175% (decaying)
    float as_bonus;             // 25%
} JinxPassive;

// Trigger: Kill/assist within 3s of dealing damage
// Effect: +175% MS (decaying), +25% AS for 6 seconds
```

#### Q: Switcheroo! (Toggle)

| Mode | Range | Damage | Attack Speed | Mana Cost |
|------|-------|--------|--------------|-----------|
| Pow-Pow (Minigun) | 525 | 100% AD | +0/55/80/105/130% (stacking) | 0 |
| Fishbones (Rocket) | 625-725 | 110% AD + splash | -10% penalty | 20/shot |

**Minigun Stack Mechanics:**
- Max 3 stacks
- Stack duration: 2.5 seconds
- Each attack refreshes duration
- Per-stack AS bonus: +30/55/80/105/130%

#### W: Zap! (Linear Skillshot)

| Property | Value |
|----------|-------|
| Range | 1500 units |
| Width | ~60 units |
| Speed | 2000 units/sec |
| Damage | 10/60/110/160/210 + 140% AD |
| Slow | 40/50/60/70/80% |
| Slow Duration | 2 seconds |
| Reveal Duration | 2 seconds |
| Cooldown | 8/7/6/5/4 seconds |
| Mana Cost | 40/45/50/55/60 |

#### E: Flame Chompers! (Traps)

| Property | Value |
|----------|-------|
| Range | 925 units |
| Number | 3 chompers |
| Arm Time | 0.5 seconds |
| Ground Duration | 5 seconds |
| Trigger Radius | ~100 units |
| Damage | 90/140/190/240/290 + 100% AP |
| Root Duration | 1.5 seconds |
| Explosion Radius | ~300 units |
| Cooldown | 24/20.5/17/13.5/10 seconds |
| Mana Cost | 90 |

#### R: Super Mega Death Rocket! (Global Ultimate)

| Property | Value |
|----------|-------|
| Range | Global |
| Initial Speed | 1500 units/sec |
| Final Speed | 2500 units/sec |
| Acceleration Time | 1.0 seconds |
| Min Damage | 32.5/47.5/62.5 + 16.5% bonus AD |
| Max Damage | 200/350/500 + 120% bonus AD |
| Missing HP Bonus | 25/30/35% |
| Splash Radius | ~500 units |
| Splash Damage | 80% of primary |
| Cooldown | 85/65/45 seconds |
| Mana Cost | 100 |
| Cast Time | 0.6 seconds |

### 2.4 Implementation Phases

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| Phase 1 | Week 1-2 | Q toggle, Passive, attack speed stacking |
| Phase 2 | Week 3 | W skillshot, slow mechanic, reveal |
| Phase 3 | Week 4 | E trap placement, proximity detection |
| Phase 4 | Week 5 | R global projectile, acceleration, execute |
| Phase 5 | Week 6 | Animations, VFX, polish |

### 2.5 Required Animation Set

**Estimated: 20+ animations per weapon mode**

| Animation | Duration | Notes |
|-----------|----------|-------|
| Minigun Idle | Loop | Fidgeting, weapon held |
| Minigun Walk | Loop | Bouncy movement |
| Minigun Attack | 0.4s | 3-frame rapid fire |
| Rocket Idle | Loop | Heavier stance |
| Rocket Walk | Loop | Weighted movement |
| Rocket Attack | 0.6s | Recoil animation |
| Weapon Swap | 0.9s | Holster/draw |
| W Cast | 0.3s | Aim and fire |
| E Cast | 0.3s | Throw motion |
| R Cast | 0.6s | Channel pose |
| Passive Active | Loop | Excited movement |
| Death | 2.0s | Fall/explosion |
| Respawn | 1.0s | Spawn-in |

---

## 3. Animation Pipeline & Sources

### 3.1 Free Animation Resources

| Source | Count | Format | License | Quality |
|--------|-------|--------|---------|---------|
| **Mixamo** | 1000+ | FBX/glTF | Commercial free | ⭐⭐⭐⭐⭐ |
| **Rokoko** | 263 | FBX | Commercial free | ⭐⭐⭐⭐ |
| **CMU Mocap** | 2600+ | BVH | Academic (check) | ⭐⭐⭐ |
| **ActorCore** | Monthly free | FBX | Commercial free | ⭐⭐⭐⭐ |

### 3.2 Recommended Workflow

1. **Use Mixamo rig as standard skeleton**
   - Industry standard, excellent compatibility
   - Free auto-rigging for custom characters
   - Large animation library

2. **Retargeting Pipeline:**
```
Source Animation (FBX/BVH)
        ↓
   Blender Import
        ↓
   Retarget to Mixamo Rig (Auto-Rig Pro or Rokoko)
        ↓
   Export as glTF 2.0
        ↓
   Arena Engine (cgltf loading)
```

3. **Animation Categories Needed:**

| Category | Animations | Source Priority |
|----------|------------|-----------------|
| Locomotion | Idle, Walk, Run, Strafe | Mixamo |
| Combat | Attack, Cast, Hit React | Mixamo + Custom |
| Abilities | Spell casts, channels | Custom |
| Death/Spawn | Death, respawn | Mixamo |
| Emotes | Dance, taunt, joke | Custom/Mixamo |

### 3.3 Retargeting Technical Details

**Skeleton Mapping (Mixamo → Arena):**
```c
typedef struct BoneMapping {
    const char* source_name;
    const char* target_name;
    Vec3 rotation_offset;
} BoneMapping;

static const BoneMapping MIXAMO_TO_ARENA[] = {
    {"mixamorig:Hips", "root", {0, 0, 0}},
    {"mixamorig:Spine", "spine_01", {0, 0, 0}},
    {"mixamorig:Spine1", "spine_02", {0, 0, 0}},
    {"mixamorig:Spine2", "spine_03", {0, 0, 0}},
    {"mixamorig:Neck", "neck", {0, 0, 0}},
    {"mixamorig:Head", "head", {0, 0, 0}},
    {"mixamorig:LeftShoulder", "clavicle_l", {0, 0, 0}},
    {"mixamorig:LeftArm", "upperarm_l", {0, 0, 0}},
    // ... etc
};
```

---

## 4. Game Launcher Architecture

### 4.1 Overview

The launcher is a separate application managing:
- Account authentication
- Social features (friends, chat)
- Matchmaking queue
- Game client launching
- Updates and patching

### 4.2 System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      GAME LAUNCHER                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │    GUI       │  │   Account    │  │   Social     │          │
│  │  (Dear ImGui │  │   System     │  │   System     │          │
│  │   + Vulkan)  │  │              │  │              │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  Matchmaking │  │    Lobby     │  │    Chat      │          │
│  │    Queue     │  │   System     │  │   System     │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                           │                                      │
│                    ┌──────┴──────┐                              │
│                    │  IPC/Network │                              │
│                    └──────┬──────┘                              │
└───────────────────────────┼─────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
     ┌───────────┐   ┌───────────┐   ┌───────────┐
     │   Game    │   │   Auth    │   │  Backend  │
     │  Client   │   │  Server   │   │  Services │
     │  (Child)  │   │           │   │           │
     └───────────┘   └───────────┘   └───────────┘
```

### 4.3 Account System

**Authentication:**
- Password hashing: bcrypt or Argon2id (preferred)
- Session tokens: JWT with 1-hour expiry
- Refresh tokens: 30-day expiry, stored securely
- Database: PostgreSQL

```c
typedef struct AccountSession {
    uint64_t account_id;
    char username[32];
    char display_name[64];
    char jwt_token[512];
    char refresh_token[256];
    time_t expires_at;
    AccountStatus status;
} AccountSession;

typedef enum AccountStatus {
    ACCOUNT_OFFLINE,
    ACCOUNT_ONLINE,
    ACCOUNT_IN_GAME,
    ACCOUNT_IN_QUEUE,
    ACCOUNT_AWAY,
    ACCOUNT_DO_NOT_DISTURB,
} AccountStatus;
```

### 4.4 Friends System

```c
typedef struct FriendEntry {
    uint64_t friend_id;
    char display_name[64];
    AccountStatus status;
    char status_message[128];
    time_t last_online;
    bool is_favorite;
} FriendEntry;

typedef struct FriendsList {
    FriendEntry* friends;
    uint32_t count;
    uint32_t capacity;

    FriendEntry* pending_incoming;
    uint32_t pending_incoming_count;

    FriendEntry* pending_outgoing;
    uint32_t pending_outgoing_count;

    uint64_t* blocked;
    uint32_t blocked_count;
} FriendsList;

// API
void friends_send_request(uint64_t target_id);
void friends_accept_request(uint64_t from_id);
void friends_decline_request(uint64_t from_id);
void friends_remove(uint64_t friend_id);
void friends_block(uint64_t user_id);
void friends_unblock(uint64_t user_id);
```

### 4.5 Lobby System

```c
typedef struct LobbyMember {
    uint64_t account_id;
    char display_name[64];
    uint32_t champion_id;       // 0 = not selected
    bool is_ready;
    bool is_leader;
    uint8_t team;               // For custom games
} LobbyMember;

typedef struct Lobby {
    uint64_t lobby_id;
    LobbyType type;             // NORMAL, RANKED, CUSTOM
    LobbyMember members[10];
    uint8_t member_count;
    uint8_t max_members;
    LobbyState state;           // FORMING, CHAMPION_SELECT, IN_GAME
    char password[32];          // For private lobbies
    uint32_t map_id;
    GameSettings settings;
} Lobby;

typedef enum LobbyType {
    LOBBY_NORMAL,
    LOBBY_RANKED,
    LOBBY_CUSTOM,
    LOBBY_PRACTICE,
} LobbyType;
```

### 4.6 Matchmaking System

**Rating System:** ELO or TrueSkill-style MMR
- Starting MMR: 1200
- K-factor: 32 (volatile early, decreases with games)
- Match range: ±100 MMR initially, expands over time

```c
typedef struct MatchmakingEntry {
    uint64_t account_id;
    uint32_t mmr;
    time_t queue_start_time;
    uint32_t search_range;      // Expands over time
    uint32_t preferred_roles;   // Bitmask
} MatchmakingEntry;

typedef struct MatchmakingQueue {
    MatchmakingEntry* entries;
    uint32_t count;
    uint32_t capacity;
    float avg_wait_time;
} MatchmakingQueue;

// Match formation
typedef struct MatchProposal {
    uint64_t match_id;
    MatchmakingEntry team1[5];
    MatchmakingEntry team2[5];
    float quality_score;        // Balance metric
    time_t created_at;
} MatchProposal;
```

### 4.7 Chat System

**Protocol:** WebSocket for real-time messaging

```c
typedef enum ChatChannelType {
    CHAT_WHISPER,           // 1:1 private
    CHAT_PARTY,             // Lobby/party chat
    CHAT_GAME,              // In-game team chat
    CHAT_GAME_ALL,          // In-game all chat
    CHAT_CLUB,              // Club/guild chat
} ChatChannelType;

typedef struct ChatMessage {
    uint64_t message_id;
    uint64_t channel_id;
    uint64_t sender_id;
    char sender_name[64];
    char content[500];
    time_t timestamp;
    bool is_system_message;
} ChatMessage;
```

### 4.8 Technology Stack

| Component | Technology | Notes |
|-----------|------------|-------|
| GUI | Dear ImGui + Vulkan | Consistent with engine |
| HTTP Client | libcurl | REST API calls |
| WebSocket | libwebsockets | Real-time messaging |
| JSON | cJSON | Config/API parsing |
| Database | PostgreSQL | Server-side |
| Auth | JWT + Argon2 | Secure sessions |

**Estimated Implementation:** 8-10 weeks

---

## 5. 3D Map Editor Technical Design

### 5.1 Overview

A standalone tool for creating and editing 3D game maps with:
- Real-time viewport rendering
- Terrain sculpting and painting
- Object placement
- NavMesh generation
- Collision editing

### 5.2 Editor Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                       MAP EDITOR                                 │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   3D VIEWPORT                             │   │
│  │   (Dear ImGui + Vulkan, Orbit Camera, Gizmos)            │   │
│  └──────────────────────────────────────────────────────────┘   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │   Layers    │  │  Properties │  │    Asset Browser        │  │
│  │   Panel     │  │    Panel    │  │    (Drag & Drop)        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    TOOLBAR                                │   │
│  │  [Select] [Move] [Rotate] [Scale] [Terrain] [Objects]    │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 Layer System

| Layer | Description | Edit Tools |
|-------|-------------|------------|
| Terrain | Heightmap + textures | Sculpt, paint, flatten |
| Collision | Walkable/blocked areas | Paint, auto-generate |
| Navigation | NavMesh polygons | Generate, manual edit |
| Objects | Static meshes | Place, transform |
| Spawns | Player/minion spawns | Place, configure |
| Waypoints | AI paths, objectives | Connect, sequence |
| Triggers | Event zones | Draw, configure |

### 5.4 Terrain System (Hybrid)

**Visual Terrain:** Heightmap-based for 3D rendering
**Gameplay Terrain:** Tile-based for deterministic collision

```c
typedef struct TerrainLayer {
    // Visual heightmap
    float* heights;             // [width * height]
    uint32_t heightmap_width;
    uint32_t heightmap_height;
    float height_scale;

    // Texture splatmap
    uint8_t* splat_weights;     // [width * height * 4] (4 textures)
    uint32_t texture_ids[4];

    // Gameplay tiles
    uint8_t* tile_types;        // [tile_width * tile_height]
    uint32_t tile_width;
    uint32_t tile_height;
    float tile_size;
} TerrainLayer;

typedef enum TileType {
    TILE_WALKABLE,
    TILE_BLOCKED,
    TILE_BRUSH,
    TILE_WATER,
    TILE_SPAWN_BLUE,
    TILE_SPAWN_RED,
} TileType;
```

### 5.5 NavMesh Integration

**Library:** Recast/Detour (MIT license)

```c
typedef struct NavMeshConfig {
    float cell_size;            // 0.3 typical
    float cell_height;          // 0.2 typical
    float agent_height;         // 2.0 (character height)
    float agent_radius;         // 0.6 (collision radius)
    float agent_max_climb;      // 0.9 (step height)
    float agent_max_slope;      // 45.0 (degrees)
    int region_min_size;        // 8
    int region_merge_size;      // 20
    float edge_max_len;         // 12.0
    float edge_max_error;       // 1.3
    int verts_per_poly;         // 6
    float detail_sample_dist;   // 6.0
    float detail_sample_error;  // 1.0
} NavMeshConfig;

// Generation UI exposes these parameters
// Auto-rebuilds on terrain/collision changes
```

### 5.6 Object Placement

```c
typedef struct MapObject {
    uint64_t object_id;
    uint32_t mesh_id;           // Reference to MeshManager
    Transform3D transform;
    uint32_t flags;             // Collision, shadow, etc.
    char name[64];
    AABB bounds;                // For selection/culling
} MapObject;

typedef struct ObjectBrush {
    uint32_t mesh_ids[8];       // Random selection
    uint8_t mesh_count;
    float min_scale;
    float max_scale;
    float min_rotation;
    float max_rotation;
    float density;              // Objects per unit area
    bool align_to_terrain;
    bool random_y_rotation;
} ObjectBrush;
```

### 5.7 Undo/Redo System

**Pattern:** Command pattern with operation stack

```c
typedef struct EditorCommand {
    EditorCommandType type;
    void* data;
    size_t data_size;
    void (*execute)(struct EditorCommand* cmd, EditorState* state);
    void (*undo)(struct EditorCommand* cmd, EditorState* state);
    void (*free_data)(struct EditorCommand* cmd);
} EditorCommand;

typedef struct UndoStack {
    EditorCommand* commands;
    uint32_t count;
    uint32_t capacity;
    uint32_t current;           // Current position in stack
} UndoStack;

// Target: 100+ undo operations
#define UNDO_STACK_CAPACITY 128
```

### 5.8 File Format

**Binary format with compression:**

```c
typedef struct MapHeader {
    char magic[4];              // "AMAP"
    uint32_t version;
    uint32_t flags;
    uint32_t terrain_offset;
    uint32_t collision_offset;
    uint32_t navmesh_offset;
    uint32_t objects_offset;
    uint32_t spawns_offset;
    uint32_t metadata_offset;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
} MapHeader;

// Compression: zstd level 3 (good balance)
// Metadata: JSON for human-readable editing
```

**Estimated Implementation:** 8-12 weeks

---

## 6. Replay System Technical Design

### 6.1 Overview

Record, store, and playback complete game matches for:
- Post-match review
- Spectating (delayed)
- Highlight creation
- Debugging

### 6.2 Recording Architecture

**Hybrid Approach:** Keyframes + Input Recording

```c
typedef struct ReplayFrame {
    uint32_t tick;
    uint32_t data_offset;       // Offset into replay data buffer
    uint32_t data_size;
    ReplayFrameType type;
} ReplayFrame;

typedef enum ReplayFrameType {
    FRAME_KEYFRAME,             // Full world state snapshot
    FRAME_DELTA,                // Changes since last frame
    FRAME_INPUT,                // Player inputs only
    FRAME_EVENT,                // Game events (kills, objectives)
} ReplayFrameType;

typedef struct ReplayRecorder {
    FILE* file;
    uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_used;
    uint32_t keyframe_interval; // Every 10 seconds (600 ticks @ 60Hz)
    uint32_t last_keyframe_tick;
    ReplayFrame* frame_index;
    uint32_t frame_count;
    uint32_t frame_capacity;
} ReplayRecorder;
```

### 6.3 File Format

```c
typedef struct ReplayHeader {
    char magic[4];              // "AREP"
    uint32_t version;
    uint64_t match_id;
    uint32_t duration_ticks;
    float tick_rate;
    uint32_t player_count;
    char map_name[64];
    time_t match_timestamp;
    uint32_t frame_index_offset;
    uint32_t frame_count;
    uint32_t checksum;          // HMAC for integrity
} ReplayHeader;

// File size estimate: ~1.5-2 MB per 30-minute match (with zstd)
```

### 6.4 Seeking & Playback

**Keyframe seeking:** Jump to nearest keyframe, then replay forward

```c
typedef struct ReplayPlayback {
    ReplayHeader header;
    FILE* file;
    World* world;               // Simulation world
    uint32_t current_tick;
    uint32_t target_tick;
    float playback_speed;       // 0.25x to 8x
    bool paused;

    // Frame index for seeking
    ReplayFrame* frames;
    uint32_t frame_count;
} ReplayPlayback;

// Seeking algorithm:
// 1. Find keyframe at or before target tick
// 2. Load keyframe into world state
// 3. Replay inputs/deltas until target tick
// Target seek time: <500ms for any position
```

### 6.5 Playback Controls

| Control | Values | Notes |
|---------|--------|-------|
| Speed | 0.25x, 0.5x, 1x, 2x, 4x, 8x | Smooth transitions |
| Pause/Resume | Toggle | Instant |
| Seek | Any tick | Keyframe-based |
| Event Jump | Next/prev event | Kill, objective, etc. |
| Camera | Free, Follow Player | Unlocked from player |

### 6.6 UI Components

```
┌─────────────────────────────────────────────────────────────────┐
│                      REPLAY VIEWER                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   GAME VIEWPORT                           │   │
│  │          (Free camera, follow player)                     │   │
│  └──────────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  TIMELINE                                                 │   │
│  │  [|----★----●----★----●----★----|]  15:32 / 32:45        │   │
│  │       ★ = Kill    ● = Objective                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│  ┌─────────────┐  ┌─────────────────────────────────────────┐   │
│  │  CONTROLS   │  │           PLAYER STATS                   │   │
│  │ [◀◀][▶][▶▶] │  │  Player1: 5/2/7  |  Player2: 3/4/8     │   │
│  │    [1x]     │  │  Gold: 8,450      |  Gold: 7,200        │   │
│  └─────────────┘  └─────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  MINIMAP                                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 6.7 Security

- **HMAC signatures:** Prevent tampering with replay files
- **Server-side recording:** Authoritative source for competitive
- **Client-side optional:** For casual play/practice

### 6.8 Storage Management

```c
typedef struct ReplayStorage {
    char base_path[256];
    uint64_t max_storage_bytes;     // e.g., 1 GB
    uint32_t max_replay_count;      // e.g., 100
    uint32_t auto_delete_days;      // e.g., 30

    // Bookmarks persist longer
    uint64_t* bookmarked_matches;
    uint32_t bookmark_count;
} ReplayStorage;

// Auto-cleanup: Oldest non-bookmarked replays deleted first
```

**Estimated Implementation:** 4-6 weeks

---

## 7. Combined Implementation Timeline

### Phase Overview

```
Month 1 (Weeks 1-4):
├── Week 1-2: Movement System (CC, dashes, physics)
├── Week 3-4: Jinx Phase 1-2 (Q, Passive, W)
└── Parallel: Animation pipeline setup

Month 2 (Weeks 5-8):
├── Week 5-6: Jinx Phase 3-4 (E, R)
├── Week 7-8: Jinx Phase 5 (Polish, animations)
└── Parallel: Map Editor Phase 1 (Viewport, terrain)

Month 3 (Weeks 9-12):
├── Week 9-10: Map Editor Phase 2 (Objects, NavMesh)
├── Week 11-12: Game Launcher Phase 1 (Auth, UI)
└── Parallel: Replay System Phase 1 (Recording)

Month 4 (Weeks 13-16):
├── Week 13-14: Game Launcher Phase 2 (Social, lobbies)
├── Week 15-16: Game Launcher Phase 3 (Matchmaking)
└── Parallel: Replay System Phase 2 (Playback, UI)

Month 5 (Weeks 17-20):
├── Week 17-18: Map Editor Phase 3 (Undo, file format)
├── Week 19-20: Integration testing, bug fixes
└── Parallel: Second champion development
```

### Milestone Summary

| Milestone | Target Week | Deliverables |
|-----------|-------------|--------------|
| M1 | Week 4 | Movement system complete, Jinx W functional |
| M2 | Week 8 | Jinx fully playable |
| M3 | Week 12 | Map Editor MVP, Launcher auth working |
| M4 | Week 16 | Full launcher, replay recording |
| M5 | Week 20 | All systems integrated, second champion |

---

## 8. Dependencies & Libraries

### 8.1 Required Libraries

| Library | Version | License | Purpose |
|---------|---------|---------|---------|
| cgltf | 1.13+ | MIT | glTF loading |
| stb_image | 2.28+ | MIT | Texture loading |
| cJSON | 1.7+ | MIT | JSON parsing |
| libcurl | 8.0+ | MIT-style | HTTP client |
| libwebsockets | 4.3+ | MIT | WebSocket |
| zstd | 1.5+ | BSD | Compression |
| Recast/Detour | 1.6+ | zlib | NavMesh |

### 8.2 Optional Libraries

| Library | Version | License | Purpose | When Needed |
|---------|---------|---------|---------|-------------|
| JoltPhysics | 4.0+ | MIT | 3D physics | If physics required |
| meshoptimizer | 0.19+ | MIT | Mesh optimization | Asset pipeline |
| KTX-Software | 4.0+ | Apache 2.0 | Compressed textures | Memory optimization |
| OpenAL-Soft | 1.23+ | LGPL | 3D audio | Sound system |

### 8.3 Build Dependencies

| Dependency | Platform | Notes |
|------------|----------|-------|
| Vulkan SDK | All | 1.3+ required |
| glslc | All | Shader compilation |
| CMake | All | 3.20+ |
| GCC/Clang | Linux | C11 support |
| MSVC | Windows | C11 support |

---

## 9. Risk Assessment

### 9.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Animation retargeting issues | Medium | High | Early pipeline testing, fallback to Mixamo |
| NavMesh generation quality | Low | Medium | Manual editing tools, parameter tuning |
| Matchmaking complexity | Medium | Medium | Start simple (ELO), iterate |
| Replay file corruption | Low | High | Checksums, auto-save, backups |
| Map Editor performance | Medium | Medium | LOD, culling, async loading |

### 9.2 Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Jinx abilities complex | Medium | High | Simplify if needed, defer polish |
| Map Editor scope creep | High | Medium | Define MVP, defer advanced features |
| Integration issues | Medium | High | Continuous integration, early testing |
| Third-party library issues | Low | Medium | Have fallback options |

### 9.3 Resource Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Animation asset quality | Medium | Medium | Multiple sources, custom if needed |
| Art asset availability | Medium | Medium | Placeholder pipeline, asset store |
| Documentation gaps | Low | Low | Living documents, code comments |

---

## 10. Appendix: Quick Reference Tables

### Movement Speed Examples

| Champion | Base MS | With Boots | Max (typical) |
|----------|---------|------------|---------------|
| Kled | 305 | 350 | 400 |
| Jinx | 330 | 375 | 450+ (passive) |
| Master Yi | 355 | 400 | 600+ (ult) |

### CC Duration Examples

| Ability | CC Type | Duration | Tenacity? |
|---------|---------|----------|-----------|
| Morgana Q | Root | 2-3s | Yes |
| Ashe R | Stun | 1-3.5s | Yes |
| Alistar Q | Knockup | 1s | No |
| Lee Sin R | Knockback | Variable | No |
| Jinx E | Root | 1.5s | Yes |

### Animation Frame Counts

| Animation | Frames | Duration | Loop |
|-----------|--------|----------|------|
| Idle | 60 | 2s | Yes |
| Walk | 30 | 1s | Yes |
| Run | 24 | 0.8s | Yes |
| Attack | 15-20 | 0.4-0.6s | No |
| Spell Cast | 10-20 | 0.3-0.6s | No |
| Death | 60 | 2s | No |

---

*Document compiled from research conducted 2026-03-08.*
*Related documents: 3D_IMPLEMENTATION_PROPOSAL.md, JINX_REFERENCE_DESIGN.md, MOVEMENT_IMPLEMENTATION_GUIDE.md*

