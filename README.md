# Arena Engine

A performant MOBA game engine for Linux, built with C and Vulkan 1.2.

## Project Status

| Component | Status | Version |
|-----------|--------|---------|
| **Core Engine** | ✅ Implemented | v0.7.0 |
| **3D Rendering** | 📋 Designed | - |
| **UI Systems** | 📋 Designed | - |
| **Tools** | 📋 Designed | - |

**Current:** v0.7.0 (Content) — ~6,365 lines of code implemented  
**Next:** v0.8.0 (3D Foundation)

## Project Aim

Build a complete, high-performance MOBA game engine inspired by League of Legends, featuring:

- **3D Isometric Rendering** — Vulkan-based mesh rendering with skeletal animation
- **Multiplayer Networking** — Authoritative server with client prediction (30Hz tick rate)
- **Champion System** — Abilities, stats, scaling, items, and inventory
- **AI Systems** — Minion waves, tower targeting, pathfinding
- **Complete UI** — Main menus, in-game HUD, chat, Steam integration
- **Development Tools** — Map editor, asset pipeline, replay system

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Arena Engine                             │
├─────────────┬─────────────┬─────────────┬─────────────┬─────────┤
│   Renderer  │     ECS     │   Network   │    Game     │  Tools  │
│   (Vulkan)  │ (Sparse-Set)│    (UDP)    │   Logic     │         │
├─────────────┴─────────────┴─────────────┴─────────────┴─────────┤
│                    Core (Allocator, Math, Collections)           │
└─────────────────────────────────────────────────────────────────┘
```

## Implemented Features (v0.7.0)

- **Rendering:** Vulkan 1.2 quad pipeline, SPIR-V shaders
- **ECS:** Sparse-set storage, component queries, 10K+ entities
- **Networking:** UDP client-server, state synchronization, 10 players
- **Combat:** Abilities, projectiles, collision, damage system
- **Map:** Tile-based arena, spawn points, lane paths
- **AI:** Minion state machines, tower targeting, wave spawning
- **Champions:** 3 archetypes (Warrior, Mage, Ranger) with stats
- **Items:** 9 items, 6-slot inventory, gold system

## Development Plan

| Phase | Milestone | Focus | Est. Duration |
|-------|-----------|-------|---------------|
| 1 | v0.8.0 | 3D Foundation (mesh, camera, depth) | 2-3 weeks |
| 2 | v0.9.0 | Materials & Lighting | 2-3 weeks |
| 3 | v0.10.0 | Animation & Champions | 2-3 weeks |
| 4 | v0.11.0 | UI Implementation | 3-4 weeks |
| 5 | v0.12.0 | Tools (Map Editor, Replay) | 4-5 weeks |
| 6 | v1.0.0 | Polish & Release | 3-4 weeks |

**Total Estimated:** 16-22 weeks

## Building

### Requirements

- Linux (primary platform)
- Vulkan SDK 1.2+
- GLFW 3.3+
- CMake 3.16+
- GCC or Clang with C11 support

### Build Commands

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running

```bash
# Start server
./build/src/server/arena-server

# Start client (in another terminal)
./build/src/client/arena-client
```

## Documentation

All technical documentation is organized in the [`docs/`](docs/) directory.

| Section | Description |
|---------|-------------|
| [📋 Index](docs/INDEX.md) | **Start here** — Navigation and summaries |
| [01-Project Overview](docs/01-project-overview/) | Architecture, milestones, specs |
| [02-Research](docs/02-research/) | Movement, champions, game analysis |
| [03-Design](docs/03-design/) | 3D rendering, UI, tools design |

### Key Documents

- [Milestones](docs/01-project-overview/MILESTONES.md) — Development roadmap and progress
- [3D Implementation](docs/03-design/3d-rendering/3D_IMPLEMENTATION_PROPOSAL.md) — 3D rendering design
- [Jinx Champion](docs/02-research/champions/JINX_REFERENCE_DESIGN.md) — First champion reference
- [UI Design](docs/03-design/ui/UI_MAIN_MENU_DESIGN.md) — Complete UI specification

## Project Structure

```
arena-engine/
├── src/
│   ├── arena/           # Core engine library
│   │   ├── alloc/       # Arena allocator
│   │   ├── collections/ # HashMap, Array
│   │   ├── ecs/         # Entity-Component-System
│   │   ├── game/        # Champions, items, combat, AI
│   │   ├── map/         # Tile-based map system
│   │   ├── math/        # Vec3, Mat4, Quat
│   │   └── network/     # UDP protocol, client/server
│   ├── renderer/        # Vulkan rendering
│   ├── client/          # Game client executable
│   └── server/          # Headless server executable
├── docs/                # Technical documentation
└── tools/               # Development tools
```

## License

This project is licensed under the **MIT License**.

```
MIT License

Copyright (c) 2026 Arena Engine Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

See [LICENSE](LICENSE) for the full license text.

### What This Means

The MIT License is a permissive open-source license that allows you to:
- ✅ Use the software for any purpose (commercial or personal)
- ✅ Modify the source code
- ✅ Distribute copies
- ✅ Sublicense to others

**Requirements:**
- Include the original copyright notice and license in any copy or substantial portion of the software

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork the repository** and create your branch from `main`
2. **Follow the code style** — C11 standard, consistent formatting
3. **Test your changes** — Ensure the build passes before submitting
4. **Submit a pull request** with a clear description of your changes

For major changes, please open an issue first to discuss what you would like to change.

