# Arena Engine Architecture Decisions

**Version:** 0.8.0
**Last Updated:** 2026-03-08
**Target Platform:** Linux (Vulkan)

---

## 1. MODULE BOUNDARIES

### 1.1 Core Library (`arena-core`)
**Location:** `src/arena/`  
**Type:** Static library  
**Dependencies:** None (zero external dependencies)

**Responsibilities:**
- Arena allocator and memory management primitives
- Generic collections (array, hashmap, ring buffer)
- Math library (vec2, vec3, vec4, mat4, quat)
- ECS core (entity management, component storage, system scheduling)
- Platform abstraction (time, threads, atomics)

**Public API Boundaries:**
```
arena/alloc/     - Allocators only, no game logic
arena/collections/ - Generic data structures
arena/math/      - Pure math, SIMD where beneficial
arena/ecs/       - Entity-component-system primitives
arena/platform/  - OS abstraction (NEW)
```

**Rationale:** Zero dependencies ensures the core can be used in any context (server, client, tools) without pulling in Vulkan or windowing code.

---

### 1.2 Renderer (`arena-renderer`)
**Location:** `src/renderer/`
**Type:** Static library
**Dependencies:** `arena-core`, Vulkan, cgltf (3D assets), stb_image (textures)

**Responsibilities:**
- Vulkan device/instance management
- Render graph / frame graph
- Pipeline and shader management (2D quad + 3D mesh pipelines)
- GPU resource management (buffers, images, samplers)
- Draw command submission
- Debug rendering (lines, shapes, text)
- **3D mesh rendering (glTF 2.0 models)**
- **Skeletal animation and skinning**
- **PBR material system**
- **Shadow mapping**

**Does NOT handle:**
- Window creation (client's responsibility)
- Input handling
- Game state

**Rationale:** Separating renderer from windowing allows headless rendering for tools and testing. The 3D pipeline coexists with the 2D quad pipeline for UI rendering.

---

### 1.3 Network (`arena-network`)
**Location:** `src/arena/network/` → move to `src/network/` (NEW)  
**Type:** Static library  
**Dependencies:** `arena-core`

**Responsibilities:**
- UDP socket abstraction
- Reliable/unreliable packet channels
- Serialization/deserialization
- Client connection state machine
- Server connection management
- Lag compensation primitives

**Protocol Design:**
- Tick-based synchronization (60 ticks/sec server, variable client)
- Delta compression for game state
- Input prediction with server reconciliation

---

### 1.4 Game (`arena-game`)
**Location:** `src/game/` (NEW)  
**Type:** Static library  
**Dependencies:** `arena-core`, `arena-network`

**Responsibilities:**
- Gameplay ECS components and systems
- Champion/ability definitions
- Combat system (damage, healing, CC)
- Movement and physics
- AI (bots, minions, jungle camps)
- Match state machine

**Does NOT depend on:** Renderer (runs identically on server)

---

### 1.5 Client Executable (`arena-client`)
**Location:** `src/client/`  
**Type:** Executable  
**Dependencies:** `arena-core`, `arena-renderer`, `arena-network`, `arena-game`, GLFW

**Responsibilities:**
- Window creation and management
- Input polling → input commands
- Main loop orchestration
- Render submission
- Audio playback (future)
- UI rendering

---

### 1.6 Server Executable (`arena-server`)
**Location:** `src/server/`  
**Type:** Executable  
**Dependencies:** `arena-core`, `arena-network`, `arena-game`

**Responsibilities:**
- Headless game simulation
- Authoritative game state
- Client connection management
- Match lifecycle
- Anti-cheat validation

---

## 2. DATA FLOW

### 2.1 Client Frame Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│ FRAME START                                                         │
├─────────────────────────────────────────────────────────────────────┤
│ 1. Poll Input (GLFW)                                                │
│    └─► InputBuffer (raw keys, mouse)                                │
│                                                                     │
│ 2. Process Input → GameInput                                        │
│    └─► Movement intent, ability activations, targeting              │
│                                                                     │
│ 3. Client Prediction                                                │
│    ├─► Apply GameInput locally (speculative)                        │
│    └─► Store input in prediction buffer                             │
│                                                                     │
│ 4. Network Send                                                     │
│    └─► Send GameInput to server (unreliable w/ sequence)            │
│                                                                     │
│ 5. Network Receive                                                  │
│    ├─► Receive authoritative state from server                      │
│    └─► Reconcile: compare server state, replay unacked inputs       │
│                                                                     │
│ 6. Interpolation                                                    │
│    └─► Interpolate remote entities between snapshots                │
│                                                                     │
│ 7. Prepare Render Data                                              │
│    └─► Extract visible entities, UI state → RenderWorld             │
│                                                                     │
│ 8. Submit Render Commands                                           │
│    └─► Renderer builds command buffers                              │
│                                                                     │
│ 9. Present                                                          │
│    └─► vkQueuePresentKHR                                            │
├─────────────────────────────────────────────────────────────────────┤
│ FRAME END                                                           │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 Server Tick Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│ TICK START (60 Hz fixed)                                            │
├─────────────────────────────────────────────────────────────────────┤
│ 1. Network Receive                                                  │
│    └─► Collect all pending client inputs                            │
│                                                                     │
│ 2. Input Processing                                                 │
│    └─► Validate & queue inputs per player                           │
│                                                                     │
│ 3. Game Simulation (ECS Systems in order)                           │
│    ├─► AbilitySystem: process ability activations                   │
│    ├─► MovementSystem: apply movement with collision                │
│    ├─► CombatSystem: process damage/healing                         │
│    ├─► StatusSystem: update buffs/debuffs                           │
│    ├─► DeathSystem: handle deaths, respawns                         │
│    └─► ObjectiveSystem: towers, inhibitors, nexus                   │
│                                                                     │
│ 4. Snapshot Creation                                                │
│    └─► Serialize game state delta                                   │
│                                                                     │
│ 5. Network Send                                                     │
│    └─► Broadcast snapshots to all clients                           │
├─────────────────────────────────────────────────────────────────────┤
│ TICK END                                                            │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.3 ECS System Execution Order

Systems execute in deterministic order. Shared state accessed via components only.

```c
// System priorities (lower = earlier)
typedef enum {
    SYSTEM_PRIORITY_INPUT        = 100,   // InputSystem
    SYSTEM_PRIORITY_ABILITY      = 200,   // AbilitySystem
    SYSTEM_PRIORITY_MOVEMENT     = 300,   // MovementSystem
    SYSTEM_PRIORITY_COLLISION    = 400,   // CollisionSystem
    SYSTEM_PRIORITY_COMBAT       = 500,   // CombatSystem
    SYSTEM_PRIORITY_STATUS       = 600,   // StatusEffectSystem
    SYSTEM_PRIORITY_DEATH        = 700,   // DeathSystem
    SYSTEM_PRIORITY_OBJECTIVE    = 800,   // ObjectiveSystem
    SYSTEM_PRIORITY_CLEANUP      = 900,   // CleanupSystem (entity destruction)
    SYSTEM_PRIORITY_RENDER_PREP  = 1000,  // RenderPrepSystem (client only)
} SystemPriority;
```

**Rationale:** Fixed order ensures deterministic simulation; critical for server-client reconciliation.

---

## 3. THREADING MODEL

### 3.1 Decision: Single-Threaded Game Logic + Separate Render Thread

**Game Thread (Main Thread):**
- Input processing
- Network I/O (non-blocking)
- All ECS system execution
- Game state mutation

**Render Thread:**
- Vulkan command buffer recording
- GPU submission and presentation
- Never reads game state directly (works on extracted RenderWorld)

**Network Thread (Optional, Phase 2):**
- Packet receive/send
- Compression/decompression
- Communicates via lock-free queues

### 3.2 Thread Communication

```
┌──────────────┐     RenderWorld      ┌──────────────┐
│  Game Thread │ ──────────────────► │ Render Thread │
│              │   (double-buffered)  │               │
└──────────────┘                      └───────────────┘
       │
       │  NetworkPacketQueue
       ▼  (lock-free SPSC)
┌──────────────┐
│Network Thread│
└──────────────┘
```

### 3.3 Synchronization Primitives

| Primitive | Use Case |
|-----------|----------|
| `atomic_flag` | Simple spinlocks for initialization |
| `sem_t` | Render thread frame synchronization |
| `spsc_queue` | Network packet passing |
| No mutexes in hot path | Game thread owns all game state |

### 3.4 Job System (Phase 2+)

Defer job system until profiling shows clear parallelization opportunities:
- Particle systems
- Animation blending
- Asset loading
- Pathfinding

**Rationale:** Premature threading adds complexity. Start simple, parallelize based on profiler data.

---

## 4. MEMORY MANAGEMENT STRATEGY

### 4.1 Allocator Hierarchy

```
┌─────────────────────────────────────────────────────────────────────┐
│                      System Allocator (malloc)                      │
│                    Used only for arena backing                      │
└──────────────────────────────┬──────────────────────────────────────┘
                               │
          ┌────────────────────┼────────────────────┐
          ▼                    ▼                    ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│  Frame Allocator │ │Persistent Allocator│ │  Temp Allocator  │
│   (per-frame)    │ │ (level lifetime)  │ │  (scratch work)  │
│   Reset: frame   │ │ Reset: level load │ │ Reset: immediate │
│   Size: 16 MB    │ │ Size: 256 MB      │ │ Size: 4 MB       │
└──────────────────┘ └──────────────────┘ └──────────────────┘
```

### 4.2 Frame Allocator
**Purpose:** All per-frame transient data
**Size:** 16 MB (tunable)
**Reset:** Every frame start

**Use cases:**
- Render command lists
- Temporary strings
- Query results
- Interpolation buffers

```c
// Usage pattern
void game_frame(FrameContext* ctx) {
    arena_reset(ctx->frame_alloc);

    // All allocations within frame use frame_alloc
    RenderList* list = arena_alloc(ctx->frame_alloc, sizeof(RenderList), 16);
    // ... frame work ...

    // No explicit free needed - reset at frame start
}
```

### 4.3 Persistent Allocator
**Purpose:** Data that lives for the entire level/match
**Size:** 256 MB (expandable via new arenas)
**Reset:** On level unload

**Use cases:**
- ECS component storage
- Loaded assets (meshes, textures, sounds)
- Map/level data
- AI navigation mesh

### 4.4 Per-System Allocators
Each ECS system receives a sub-arena carved from persistent allocator:

```c
typedef struct System {
    Arena* alloc;           // System-private arena
    size_t alloc_watermark; // High-water mark for tuning
    // ...
} System;
```

**Rationale:** Isolates systems for debugging; enables per-system memory tracking.

### 4.5 GPU Memory Management

```
┌─────────────────────────────────────────────────────────────────────┐
│                        VkDeviceMemory Pools                         │
├─────────────────────────────────────────────────────────────────────┤
│ DeviceLocal Pool (GPU-only)                                         │
│   ├─► Static meshes                                                 │
│   ├─► Textures                                                      │
│   └─► Compute buffers                                               │
├─────────────────────────────────────────────────────────────────────┤
│ HostVisible Pool (Staging)                                          │
│   ├─► Upload staging buffers                                        │
│   └─► Readback buffers                                              │
├─────────────────────────────────────────────────────────────────────┤
│ HostCached Pool (Uniform/Dynamic)                                   │
│   ├─► Uniform buffers (per-frame ring)                              │
│   └─► Dynamic vertex data                                           │
└─────────────────────────────────────────────────────────────────────┘
```

**Ring Buffer Strategy for Dynamic Data:**
```c
#define FRAMES_IN_FLIGHT 2

typedef struct DynamicBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped;               // Persistently mapped
    size_t size;
    size_t frame_offsets[FRAMES_IN_FLIGHT];
    uint32_t current_frame;
} DynamicBuffer;
```

**Rationale:** Avoids per-frame buffer creation; exploits persistent mapping for low-latency updates.

---

## 5. BUILD CONFIGURATION

### 5.1 CMake Presets

Add `CMakePresets.json` for standardized configurations:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "debug",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_C_FLAGS": "-g -O0 -DARENA_DEBUG=1",
        "BUILD_TESTS": "ON"
      }
    },
    {
      "name": "release",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_C_FLAGS": "-O3 -DNDEBUG -DARENA_RELEASE=1 -flto"
      }
    },
    {
      "name": "asan",
      "binaryDir": "${sourceDir}/build/asan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_C_FLAGS": "-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer"
      }
    },
    {
      "name": "profile",
      "binaryDir": "${sourceDir}/build/profile",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CMAKE_C_FLAGS": "-O2 -g -DARENA_PROFILE=1 -fno-omit-frame-pointer"
      }
    }
  ]
}
```

### 5.2 Build Configurations

| Config | Optimization | Debug Info | Asserts | Use Case |
|--------|-------------|------------|---------|----------|
| Debug | -O0 | Full (-g) | Enabled | Development |
| Release | -O3 -flto | None | Disabled | Distribution |
| ASAN | -O1 | Full | Enabled | Memory debugging |
| Profile | -O2 | Full | Enabled | Performance analysis |

### 5.3 Compile-Time Flags

```c
// Core configuration macros
#ifdef ARENA_DEBUG
    #define ARENA_ASSERT(cond) do { if (!(cond)) arena_assert_fail(#cond, __FILE__, __LINE__); } while(0)
    #define ARENA_TRACE_ALLOC 1
#else
    #define ARENA_ASSERT(cond) ((void)0)
    #define ARENA_TRACE_ALLOC 0
#endif

#ifdef ARENA_PROFILE
    #define ARENA_PROFILE_SCOPE(name) ProfileScope _scope = profile_begin(name)
    #define ARENA_PROFILE_END() profile_end(&_scope)
#else
    #define ARENA_PROFILE_SCOPE(name) ((void)0)
    #define ARENA_PROFILE_END() ((void)0)
#endif
```

### 5.4 Validation Layers (Vulkan Debug)

```c
#ifdef ARENA_DEBUG
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    #define ENABLE_VALIDATION_LAYERS 1
#else
    #define ENABLE_VALIDATION_LAYERS 0
#endif
```

---

## 6. IMPLEMENTATION PLAN

### Phase 1: Foundation (Current → +2 weeks)
1. ✅ Arena allocator
2. ✅ Basic collections (hashmap)
3. 🔲 Complete math library (vec2, vec3, vec4, mat4, quat)
4. 🔲 ECS core implementation
5. 🔲 Platform layer (time, threads)

### Phase 2: Rendering (+2-4 weeks)
1. 🔲 Vulkan initialization
2. 🔲 Swapchain management
3. 🔲 Basic pipeline (triangle)
4. 🔲 Mesh rendering
5. 🔲 Camera system

### Phase 3: Networking (+4-6 weeks)
1. 🔲 UDP sockets
2. 🔲 Packet serialization
3. 🔲 Connection state machine
4. 🔲 Input prediction
5. 🔲 Server reconciliation

### Phase 4: Gameplay (+6-10 weeks)
1. 🔲 Champion movement
2. 🔲 Basic abilities
3. 🔲 Combat system
4. 🔲 Minions/AI
5. 🔲 Match lifecycle

---

## 7. 3D RENDERING ARCHITECTURE

### 7.1 Rendering Strategy

**Decision: Forward Rendering with Parallel 2D/3D Pipelines**

| Approach | Pros | Cons |
|----------|------|------|
| Forward | Simple, MSAA-friendly, transparent-friendly | More light passes |
| Deferred | Efficient many-lights | Complex, G-buffer overhead, MSAA issues |

**Rationale:** MOBA arenas have limited light sources (sun + ~10 point lights). Forward rendering with light culling is sufficient and simpler to implement.

### 7.2 Render Frame Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                    RENDER FRAME                              │
├─────────────────────────────────────────────────────────────┤
│ Pass 1: Shadow Map (depth-only, light POV)                   │
│ Pass 2: Opaque 3D (depth test on, depth write on)           │
│ Pass 3: Transparent 3D (depth test on, depth write off)     │
│ Pass 4: 2D UI Overlay (depth test off, existing quad pipe)  │
└─────────────────────────────────────────────────────────────┘
```

### 7.3 Asset Format

**Decision: glTF 2.0 via cgltf**

| Format | Pros | Cons |
|--------|------|------|
| glTF 2.0 | Industry standard, PBR support, animation, binary variant | - |
| FBX | Autodesk standard | Proprietary, complex parser |
| OBJ | Simple | No animation, no materials |

**Library:** cgltf (single-header, MIT license, ~3K LOC)

### 7.4 Shading Model

**Decision: Stylized PBR (Metallic-Roughness Workflow)**

- Physically-based lighting with artistic controls
- Metallic-roughness texture workflow (glTF compatible)
- Environment IBL optional (future enhancement)

### 7.5 New 3D Components

| Component | Purpose | Key Fields |
|-----------|---------|------------|
| Transform3D | Full 3D position, rotation, scale | position, rotation (quat), scale, world_matrix |
| MeshRenderer | Static mesh rendering | mesh_id, material_id, bounds, cast_shadows |
| Camera | View frustum definition | projection, fov, frustum_planes |
| Light | Scene illumination | type, color, intensity, range, cast_shadows |
| SkinnedMesh | Animated characters | skeleton_id, bone_matrices, anim_state |

### 7.6 Animation Pipeline

- Skeletal animation with bone hierarchies (max 64 bones)
- Animation blending for smooth transitions (slerp)
- GPU skinning shader for performance
- Support for glTF animations

### 7.7 3D Dependencies

| Library | Version | License | Purpose |
|---------|---------|---------|---------|
| cgltf | 1.13+ | MIT | glTF 2.0 loading |
| stb_image | 2.28+ | MIT | Texture loading (extended for PBR) |
| Recast/Detour | 1.6+ | zlib | NavMesh (optional) |

---

## 8. KEY DECISIONS SUMMARY

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Language | C11 | Maximum control, minimal runtime |
| Renderer | Vulkan | Modern, explicit, Linux-native |
| **Rendering Mode** | **Forward** | **Simpler, sufficient for MOBA scene complexity** |
| **Asset Format** | **glTF 2.0** | **Industry standard, free tools, PBR + animation** |
| **Shading** | **Stylized PBR** | **Visual quality with artistic control** |
| **3D Library** | **cgltf** | **Header-only, MIT, minimal footprint** |
| Threading | Main + Render + (Network) | Simple, avoids sync bugs |
| Memory | Arena allocators | Predictable, cache-friendly |
| ECS | Custom sparse-set based | MOBA needs fast iteration |
| Networking | UDP + custom reliability | Low latency critical |
| Tickrate | 60 Hz server | Balance of responsiveness vs bandwidth |

---

## 9. FILE STRUCTURE (Target)

```
arena-engine/
├── CMakeLists.txt
├── CMakePresets.json
├── docs/
│   ├── 01-project-overview/
│   ├── 02-research/
│   └── 03-design/
│       └── 3d-rendering/         # 3D implementation docs
├── libs/
│   ├── cgltf/                    # glTF loader (header-only)
│   └── stb/                      # stb_image for textures
├── src/
│   ├── arena/                    # arena-core library
│   │   ├── alloc/
│   │   │   ├── arena_allocator.c/h
│   │   │   ├── frame_allocator.c/h
│   │   │   └── pool_allocator.c/h
│   │   ├── collections/
│   │   │   ├── array.c/h
│   │   │   ├── hashmap.c/h
│   │   │   ├── ring_buffer.c/h
│   │   │   └── sparse_set.c/h
│   │   ├── math/
│   │   │   ├── vec2.c/h
│   │   │   ├── vec3.c/h
│   │   │   ├── vec4.c/h
│   │   │   ├── mat4.c/h
│   │   │   ├── quat.c/h
│   │   │   └── math.h
│   │   ├── ecs/
│   │   │   ├── entity.c/h
│   │   │   ├── component.c/h
│   │   │   ├── system.c/h
│   │   │   ├── components_3d.h   # NEW: 3D components
│   │   │   └── world.c/h
│   │   ├── game/
│   │   │   ├── animation_system.c/h  # NEW: Animation
│   │   │   └── culling_system.c/h    # NEW: Frustum culling
│   │   ├── platform/
│   │   │   ├── platform.h
│   │   │   ├── time.c/h
│   │   │   ├── thread.c/h
│   │   │   └── atomic.h
│   │   └── arena.h               # Public umbrella header
│   ├── renderer/                 # arena-renderer library
│   │   ├── vk_context.c/h
│   │   ├── vk_swapchain.c/h
│   │   ├── vk_pipeline.c/h
│   │   ├── vk_buffer.c/h
│   │   ├── vk_image.c/h
│   │   ├── render_world.c/h
│   │   ├── mesh.c/h              # NEW: 3D mesh loading
│   │   ├── material.c/h          # NEW: PBR materials
│   │   ├── gltf_loader.c/h       # NEW: glTF import
│   │   ├── instancing.c/h        # NEW: Hardware instancing
│   │   ├── shadow_map.c/h        # NEW: Shadow mapping
│   │   ├── shaders/
│   │   │   ├── quad.vert/frag    # Existing 2D pipeline
│   │   │   ├── mesh.vert/frag    # NEW: 3D static mesh
│   │   │   ├── skinned.vert      # NEW: Skeletal animation
│   │   │   └── shadow.vert       # NEW: Shadow pass
│   │   └── renderer.h
│   ├── network/                  # arena-network library
│   │   ├── socket.c/h
│   │   ├── packet.c/h
│   │   ├── channel.c/h
│   │   ├── client.c/h
│   │   ├── server.c/h
│   │   └── network.h
│   ├── game/                     # arena-game library
│   │   ├── components/
│   │   ├── systems/
│   │   ├── abilities/
│   │   ├── combat/
│   │   └── game.h
│   ├── client/
│   │   └── main.c
│   └── server/
│       └── main.c
└── tools/
    ├── arena-editor/             # 3D Map Editor (future)
    ├── asset-packer/
    └── entity-editor/
```

---

## 10. RELATED DOCUMENTS

| Document | Description |
|----------|-------------|
| [3D_IMPLEMENTATION_PROPOSAL.md](../03-design/3d-rendering/3D_IMPLEMENTATION_PROPOSAL.md) | Detailed 3D transition plan |
| [3D_PROPOSAL_EXPANSION.md](../03-design/3d-rendering/3D_PROPOSAL_EXPANSION.md) | Movement, champions, tools research |
| [MILESTONES.md](MILESTONES.md) | Development roadmap with 3D milestones |
| [TECHNICAL_SPEC.md](TECHNICAL_SPEC.md) | ECS, network, renderer specifications |

---

*Document authored for Arena Engine development team.*

