# Arena Engine Implementation Proposal
## League of Legends-Type MOBA Game Engine

**Version:** 1.0  
**Date:** 2026-03-08  
**Project:** Arena Engine  
**Target Platform:** Linux (x86_64)  
**Language:** C11 with minimal C++ for Vulkan  

---

## Executive Summary

This proposal outlines the complete implementation plan for developing Arena Engine into a fully functional MOBA game engine capable of supporting a League of Legends-style game on Linux.

### Current State (v0.1.0)
- ✅ Arena allocator (complete)
- ✅ HashMap collection (complete)
- ⚠️ Math library (stub)
- ⚠️ ECS system (stub)
- ⚠️ Renderer (stub - Vulkan linked but not initialized)
- ⚠️ Network (stub)
- ⚠️ Client/Server (minimal - no game loop)
- ⚠️ Tools (stub - empty executables)

### Target State (v1.0.0)
- Complete MOBA game engine with:
  - 30 Hz authoritative server
  - 60 FPS Vulkan client
  - 5+ playable champions
  - Items, minions, towers, objectives
  - Fog of war, pathfinding, combat
  - Full editor toolchain

### Timeline
- **Estimated Duration:** 30-40 weeks
- **Estimated LOC:** ~22,000 lines of C/C++
- **Team Size:** 1-3 developers

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Module Structure](#2-module-structure)
3. [Milestone Summary](#3-milestone-summary)
4. [Technical Specifications](#4-technical-specifications)
5. [Risk Assessment](#5-risk-assessment)
6. [Resource Requirements](#6-resource-requirements)
7. [Success Criteria](#7-success-criteria)
8. [Supporting Documents](#8-supporting-documents)

---

## 1. Architecture Overview

### System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ARENA ENGINE                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                          arena-core                                  │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │    │
│  │  │ Alloc   │ │ Collect │ │  Math   │ │   ECS   │ │Platform │        │    │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│       ┌────────────────────────────┼────────────────────────────┐           │
│       │                            │                            │           │
│       ▼                            ▼                            ▼           │
│  ┌─────────────┐            ┌─────────────┐            ┌─────────────┐      │
│  │   arena-    │            │   arena-    │            │   arena-    │      │
│  │  renderer   │            │   network   │            │    game     │      │
│  │  (Vulkan)   │            │   (UDP)     │            │  (Systems)  │      │
│  └─────────────┘            └─────────────┘            └─────────────┘      │
│       │                            │                            │           │
│       └────────────────────────────┼────────────────────────────┘           │
│                                    │                                         │
│       ┌────────────────────────────┴────────────────────────────┐           │
│       ▼                                                         ▼           │
│  ┌─────────────┐                                         ┌─────────────┐    │
│  │   arena-    │                                         │   arena-    │    │
│  │   client    │                                         │   server    │    │
│  │ (Windowed)  │                                         │ (Headless)  │    │
│  └─────────────┘                                         └─────────────┘    │
│                                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                              TOOLS SUITE                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        tools-launcher                                │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │    │
│  │  │arena-editor │  │asset-packer │  │entity-editor│                  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Flow

**Client Frame:**
```
Input → Local Prediction → Send to Server → Receive State → Reconcile → Render
```

**Server Tick (30 Hz):**
```
Receive Inputs → Validate → Simulate ECS → Broadcast State → Record Replay
```

### Threading Model

| Thread | Responsibility |
|--------|----------------|
| Main | Input polling, network I/O, game logic, ECS systems |
| Render | Vulkan command recording, GPU submission |
| Network (future) | Packet I/O via lock-free queues |

### Memory Model

| Allocator | Size | Lifetime | Usage |
|-----------|------|----------|-------|
| Frame | 16 MB | Per frame | Transient data, strings, temp buffers |
| Persistent | 256 MB | Level lifetime | Entities, components, textures |
| GPU Pools | 512 MB | Application | Vertex buffers, textures, uniforms |

---

## 2. Module Structure

### Libraries

| Module | Type | Dependencies | Purpose |
|--------|------|--------------|---------|
| `arena-core` | Static | None | Allocator, collections, math, ECS, platform |
| `arena-renderer` | Static | arena-core, Vulkan | Vulkan abstraction, rendering |
| `arena-network` | Static | arena-core | UDP networking, protocol |
| `arena-game` | Static | arena-core, arena-network | Gameplay systems |

### Executables

| Target | Links | Purpose |
|--------|-------|---------|
| `arena-client` | All libraries + GLFW | Game client |
| `arena-server` | arena-core, arena-network, arena-game | Dedicated server |
| `arena-launcher` | arena-renderer, tools-common, GLFW | Tools launcher |
| `arena-editor` | tools-common | Map/level editor |
| `asset-packer` | tools-common | Asset pipeline |
| `entity-editor` | tools-common | Champion/entity editor |

---

## 3. Milestone Summary

| Version | Codename | Weeks | LOC | Key Deliverable |
|---------|----------|-------|-----|-----------------|
| v0.2.0 | Foundation | 3-4 | 2,050 | Vulkan renderer, math library |
| v0.3.0 | Movement | 2-3 | 1,800 | ECS, player control, sprites |
| v0.4.0 | Combat | 3-4 | 2,100 | Abilities, projectiles, damage |
| v0.5.0 | Multiplayer | 4-6 | 2,900 | Client-server networking |
| v0.6.0 | Arena Alpha | 4-5 | 2,900 | Map, AI, towers, minions |
| v0.7.0 | Content | 5-6 | 3,800 | Champions, items, jungle |
| v0.8.0 | Polish | 4-5 | 3,350 | Particles, audio, UI |
| v1.0.0 | Release | 4-6 | 2,800 | Optimization, matchmaking |

**Total: 30-40 weeks, ~21,700 LOC**

See `MILESTONES.md` for detailed deliverables and acceptance criteria.

---

## 4. Technical Specifications

### 4.1 ECS Architecture

**Entity Handle:**
```c
typedef struct EntityId {
    uint32_t index;       // Slot in entity array
    uint32_t generation;  // Validity check
} EntityId;
```

**Core Components:**
- `Transform`: Position, rotation, scale
- `Velocity`: Movement vector
- `Sprite`: Texture, frame, layer
- `Health`: Current/max HP, shields
- `Champion`: Stats, abilities, level
- `AIController`: State machine, target
- `NetworkIdentity`: Net ID, owner

**System Execution Order:**
1. InputSystem (player inputs)
2. AISystem (NPC decisions)
3. AbilitySystem (casts, cooldowns)
4. BuffSystem (apply/expire effects)
5. MovementSystem (physics, pathfinding)
6. CollisionSystem (hit detection)
7. CombatSystem (damage resolution)
8. DeathSystem (kill handling)
9. VisionSystem (fog of war)
10. RenderPrepSystem (extract render data)

### 4.2 Network Protocol

**Transport:** UDP with application-level reliability

**Tick Rate:** 30 Hz server, 60 FPS client

**Packet Types:**
| Type | Direction | Purpose |
|------|-----------|---------|
| Handshake | Both | Connection setup |
| Input | C→S | Player commands |
| StateSnapshot | S→C | Full world state |
| StateDelta | S→C | Incremental update |
| Events | S→C | Abilities, deaths, chat |

**Bandwidth Estimate:**
- Client upload: ~5 KB/s (inputs)
- Client download: ~50 KB/s (state)
- Server total: ~500 KB/s (10 players)

### 4.3 Renderer Pipeline

**Vulkan 1.3** with:
- Triple-buffered swapchain
- Instanced sprite batching
- Two render passes (World + UI)

**Frame Structure:**
```
begin_frame()
├── Update uniform buffers
├── Record command buffer
│   ├── Terrain pass
│   ├── Entity pass (batched sprites)
│   ├── Fog of war pass
│   ├── World UI pass (health bars)
│   └── Screen UI pass
├── Submit to queue
└── Present swapchain image
```

### 4.4 Asset Formats

| Format | Extension | Contents |
|--------|-----------|----------|
| Sprite | `.arena_sprite` | Texture + frames + animations |
| Map | `.arena_map` | Tiles + collision + navmesh + spawns |
| Champion | `.arena_champion` | Stats + abilities + visuals |
| Ability | `.arena_ability` | Damage + effects + targeting |

See `TECHNICAL_SPEC.md` for complete struct definitions.

---

## 5. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Vulkan complexity | High | High | Start simple, iterate. Use validation layers. |
| Network desync | Medium | High | Deterministic simulation, checksums, replay debugging |
| Scope creep | High | Medium | Strict milestone adherence, MVP focus |
| Performance issues | Medium | Medium | Profile early, optimize hot paths only |
| Art/content bottleneck | High | Medium | Programmer art for MVP, placeholders |

---

## 6. Resource Requirements

### Development Environment
- Linux x86_64 (Ubuntu 22.04+ or Arch)
- Vulkan SDK 1.3+
- GCC 11+ or Clang 14+
- CMake 3.20+
- GLFW 3.3+

### Hardware (Minimum)
- CPU: 4 cores
- RAM: 8 GB
- GPU: Vulkan 1.3 compatible

### External Dependencies
| Dependency | Version | Purpose |
|------------|---------|---------|
| Vulkan SDK | 1.3+ | Graphics API |
| GLFW | 3.3+ | Window/input |
| Dear ImGui | 1.89+ | Tool GUI |
| stb_image | Latest | Image loading |

---

## 7. Success Criteria

### v0.6.0 (Alpha) - Minimum Viable Product
- [ ] Two players can connect to a server
- [ ] Players control champions that can move
- [ ] Basic attacks deal damage
- [ ] One ability per champion works
- [ ] Minions spawn and path down lanes
- [ ] Towers attack enemies
- [ ] One team can destroy other's nexus

### v1.0.0 (Release) - Feature Complete
- [ ] 5+ unique champions
- [ ] Item shop with 20+ items
- [ ] Full map with jungle, dragon, baron
- [ ] Fog of war
- [ ] Matchmaking lobby
- [ ] Replay system
- [ ] 60 FPS @ 1080p on mid-range hardware
- [ ] <100ms network latency impact

---

## 8. Supporting Documents

This proposal is accompanied by detailed specifications:

| Document | Contents |
|----------|----------|
| `ARCHITECTURE_DECISIONS.md` | Module boundaries, threading, memory management |
| `MILESTONES.md` | Detailed deliverables, acceptance criteria, LOC estimates |
| `TECHNICAL_SPEC.md` | ECS schema, network protocol, renderer pipeline, asset formats |
| `TOOLS_LAUNCHER_DESIGN.md` | GUI tools architecture, IPC, process management |

---

## Approval

**Prepared by:** Arena Engine Development Team
**Date:** 2026-03-08

---

## Next Steps

Upon approval of this proposal:

1. **Fix critical build issue** - Remove duplicate `entity-editor` target from `tools/asset-packer/CMakeLists.txt`
2. **Begin v0.2.0 (Foundation)** - Implement math library and Vulkan initialization
3. **Commit proposal documents** - Add docs/ to version control
4. **Set up CI** - Automated builds and tests

---

*End of Implementation Proposal*

