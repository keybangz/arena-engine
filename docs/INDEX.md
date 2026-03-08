# Arena Engine Documentation Index

> **Version:** 0.8.0 (3D Foundation)
> **Last Updated:** 2026-03-08
> **Total Documents:** 21
> **Total Lines:** ~25,000+

## Quick Navigation

| Section | Documents | Description |
|---------|-----------|-------------|
| [Project Overview](#1-project-overview) | 4 | Architecture, milestones, technical specs |
| [Research](#2-research) | 10 | Movement mechanics, champions, game analysis |
| [Design](#3-design) | 7 | 3D rendering, UI systems, tools |

---

## 1. Project Overview

Core documents defining the project's architecture, roadmap, and technical foundation.

| Document | Lines | Description |
|----------|-------|-------------|
| [ARCHITECTURE_DECISIONS.md](01-project-overview/ARCHITECTURE_DECISIONS.md) | ~400 | Module boundaries, memory strategy, design patterns |
| [MILESTONES.md](01-project-overview/MILESTONES.md) | ~1,300 | Development roadmap v0.2.0→v1.0.0, progress tracking |
| [TECHNICAL_SPEC.md](01-project-overview/TECHNICAL_SPEC.md) | ~500 | Binary protocol, ECS schema, network architecture |
| [IMPLEMENTATION_PROPOSAL.md](01-project-overview/IMPLEMENTATION_PROPOSAL.md) | ~600 | High-level implementation strategy |

### Key Information
- **Current Version:** v0.7.0
- **Implemented:** ~6,365 LOC
- **3D Transition:** v0.8.0 (Foundation) → v0.9.0 (Materials) → v0.10.0 (Animation) → v1.0.0 (Release)

---

## 2. Research

Research documents covering game mechanics, champion design, and technical analysis.

### 2.1 Movement & Mechanics

Analysis of League of Legends movement, abilities, and 3D implementation.

| Document | Lines | Description |
|----------|-------|-------------|
| [LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md](02-research/movement/LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md) | ~500 | Movement speeds (305-355 MS), CC types, dash/blink mechanics |
| [MOVEMENT_IMPLEMENTATION_GUIDE.md](02-research/movement/MOVEMENT_IMPLEMENTATION_GUIDE.md) | ~300 | Code examples, C structures, implementation guide |
| [3D_CAMERA_TERRAIN_IMPLEMENTATION.md](02-research/movement/3D_CAMERA_TERRAIN_IMPLEMENTATION.md) | ~400 | 55-60° camera, terrain heights, NavMesh integration |
| [QUICK_REFERENCE_LOL_MECHANICS.md](02-research/movement/QUICK_REFERENCE_LOL_MECHANICS.md) | ~200 | Quick lookup tables for common values |

### 2.2 Champions

Detailed champion analysis starting with Jinx as the reference implementation.

| Document | Lines | Description |
|----------|-------|-------------|
| [JINX_REFERENCE_DESIGN.md](02-research/champions/JINX_REFERENCE_DESIGN.md) | ~800 | Full Jinx kit: passive, Q/W/E/R abilities, stats, scaling |
| [JINX_IMPLEMENTATION_SUMMARY.md](02-research/champions/JINX_IMPLEMENTATION_SUMMARY.md) | ~250 | Quick reference, implementation checklist |
| [JINX_VISUAL_REFERENCE.md](02-research/champions/JINX_VISUAL_REFERENCE.md) | ~400 | Art direction, animations (20+), VFX requirements |
| [JINX_CHAMPION_INDEX.md](02-research/champions/JINX_CHAMPION_INDEX.md) | ~100 | Navigation index for Jinx documents |

### 2.3 General Research

Cross-cutting research and summaries.

| Document | Lines | Description |
|----------|-------|-------------|
| [RESEARCH_SUMMARY.md](02-research/general/RESEARCH_SUMMARY.md) | ~300 | Executive summary of all research findings |
| [LOL_RESEARCH_INDEX.md](02-research/general/LOL_RESEARCH_INDEX.md) | ~150 | Navigation guide for LoL research |

---

## 3. Design

Technical design documents for implementation.

### 3.1 3D Rendering

Transition from 2D to full 3D rendering pipeline. This is the current focus of development.

| Document | Lines | Description |
|----------|-------|-------------|
| [3D_IMPLEMENTATION_PROPOSAL.md](03-design/3d-rendering/3D_IMPLEMENTATION_PROPOSAL.md) | ~1,000 | Mesh pipeline, materials, lighting, animation systems |
| [3D_PROPOSAL_EXPANSION.md](03-design/3d-rendering/3D_PROPOSAL_EXPANSION.md) | ~1,066 | Launcher, map editor, replay system, social features |

**3D Transition Phases:**
| Version | Milestone | Focus |
|---------|-----------|-------|
| v0.8.0 | 3D Foundation | glTF loading, mesh pipeline, camera, basic lighting |
| v0.9.0 | 3D Materials | PBR shading, shadow mapping, multi-texture |
| v0.10.0 | 3D Animation | Skeletal animation, GPU skinning, instancing |
| v1.0.0 | Release | Polish, particles, audio, performance |

**Key Architectural Decisions:**
- Forward rendering (simpler, sufficient for MOBA scene complexity)
- glTF 2.0 via cgltf (MIT, header-only)
- Stylized PBR shading (metallic-roughness workflow)
- Parallel 2D/3D pipelines (3D world + 2D UI overlay)

### 3.2 User Interface

Complete UI system design for menus and in-game HUD.

| Document | Lines | Description |
|----------|-------|-------------|
| [UI_MAIN_MENU_DESIGN.md](03-design/ui/UI_MAIN_MENU_DESIGN.md) | 3,037 | 8 screens: Login, Home, Play, Champion Select, Settings |
| [UI_INGAME_HUD_DESIGN.md](03-design/ui/UI_INGAME_HUD_DESIGN.md) | 2,061 | Ability bar, minimap, shop, scoreboard, world-space UI |
| [UI_CHAT_DESIGN.md](03-design/ui/UI_CHAT_DESIGN.md) | 1,627 | Channels, commands, moderation, network protocol |
| [UI_STEAM_FRIENDS_DESIGN.md](03-design/ui/UI_STEAM_FRIENDS_DESIGN.md) | 1,163 | Steamworks API, friends, invites, overlay, presence |

**UI Framework:** Dear ImGui + Vulkan

### 3.3 Tools

Development tools and launcher design.

| Document | Lines | Description |
|----------|-------|-------------|
| [TOOLS_LAUNCHER_DESIGN.md](03-design/tools/TOOLS_LAUNCHER_DESIGN.md) | ~400 | Tool launcher, asset packer, editor architecture |

---

## Document Statistics

| Section | Documents | Total Lines |
|---------|-----------|-------------|
| Project Overview | 4 | ~2,800 |
| Research | 10 | ~3,400 |
| Design | 7 | ~9,350 |
| **Total** | **21** | **~15,550** |

---

## Implementation Timeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        3D TRANSITION ROADMAP                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│ v0.8.0 (2-3 weeks)    v0.9.0 (2-3 weeks)    v0.10.0 (2-3 weeks)            │
│ ├── cgltf integration ├── PBR shaders       ├── Skeletal animation         │
│ ├── Mesh pipeline     ├── Shadow mapping    ├── GPU skinning               │
│ ├── Transform3D       ├── Material system   ├── Hardware instancing        │
│ ├── Camera system     └── Multi-texture     └── Frustum culling            │
│ └── Basic lighting                                                          │
│                                                                              │
│ v1.0.0 (4-6 weeks) - Release                                                │
│ ├── Particle system   ├── UI polish         ├── Performance tuning         │
│ ├── Audio system      ├── Champion assets   └── Bug fixes                  │
│ └── Post-processing   └── Map integration                                   │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick Links

### By Topic
- **Getting Started:** [ARCHITECTURE_DECISIONS.md](01-project-overview/ARCHITECTURE_DECISIONS.md)
- **Current Progress:** [MILESTONES.md](01-project-overview/MILESTONES.md)
- **3D Rendering:** [3D_IMPLEMENTATION_PROPOSAL.md](03-design/3d-rendering/3D_IMPLEMENTATION_PROPOSAL.md)
- **First Champion:** [JINX_REFERENCE_DESIGN.md](02-research/champions/JINX_REFERENCE_DESIGN.md)
- **UI Design:** [UI_MAIN_MENU_DESIGN.md](03-design/ui/UI_MAIN_MENU_DESIGN.md)

### By Implementation Order
1. [3D_IMPLEMENTATION_PROPOSAL.md](03-design/3d-rendering/3D_IMPLEMENTATION_PROPOSAL.md) - 3D foundation
2. [MOVEMENT_IMPLEMENTATION_GUIDE.md](02-research/movement/MOVEMENT_IMPLEMENTATION_GUIDE.md) - Movement system
3. [JINX_REFERENCE_DESIGN.md](02-research/champions/JINX_REFERENCE_DESIGN.md) - First champion
4. [UI_INGAME_HUD_DESIGN.md](03-design/ui/UI_INGAME_HUD_DESIGN.md) - In-game UI
5. [3D_PROPOSAL_EXPANSION.md](03-design/3d-rendering/3D_PROPOSAL_EXPANSION.md) - Tools & systems

