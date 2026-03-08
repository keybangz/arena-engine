# Arena Engine Tools Launcher Design

## Overview

The Arena Engine tools (arena-editor, asset-packer, entity-editor) will be managed by a unified launcher application. This design provides process isolation, shared resources, and a consistent user experience.

## Goals

1. **Unified Interface**: Single entry point for all development tools
2. **Process Isolation**: Tools run as separate processes for stability
3. **Shared Configuration**: Common settings and project context
4. **Extensibility**: Plugin-like architecture for future tools

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    TOOLS LAUNCHER (Parent)                   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ GUI Window  │  │  Process    │  │   Configuration     │  │
│  │ (Dear ImGui │  │  Manager    │  │   Manager           │  │
│  │  + Vulkan)  │  │             │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│         │                │                    │              │
│         └────────────────┼────────────────────┘              │
│                          │                                   │
│                    IPC Router                                │
│              (Unix Domain Sockets)                           │
└──────────────────────────┬──────────────────────────────────┘
                           │
       ┌───────────────────┼───────────────────┐
       │                   │                   │
       ▼                   ▼                   ▼
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Arena     │     │   Asset     │     │   Entity    │
│   Editor    │     │   Packer    │     │   Editor    │
│  (Child)    │     │  (Child)    │     │  (Child)    │
└─────────────┘     └─────────────┘     └─────────────┘
```

## Directory Structure

```
tools/
├── launcher/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── gui.h / gui.c
│   ├── process_manager.h / process_manager.c
│   └── ipc_router.h / ipc_router.c
├── common/
│   ├── CMakeLists.txt
│   ├── ipc.h / ipc.c
│   ├── config.h / config.c
│   ├── logging.h / logging.c
│   └── tool_interface.h
├── arena-editor/
├── asset-packer/
└── entity-editor/
```

## Process Management API

```c
typedef struct ToolProcess {
    pid_t pid;
    int ipc_fd;
    ToolState state;
    char name[64];
} ToolProcess;

// Core functions
ToolProcess* process_spawn(const char* tool_name, const char* args[]);
void process_terminate(ToolProcess* proc);
ToolState process_get_state(ToolProcess* proc);
bool process_is_running(ToolProcess* proc);
void process_wait_all(void);
```

## IPC Protocol

Message header (8 bytes):
```c
typedef struct IPCHeader {
    uint16_t message_type;
    uint16_t flags;
    uint32_t payload_size;
} IPCHeader;
```

Message types:
- `MSG_HANDSHAKE` (0x0001): Tool registration
- `MSG_HEARTBEAT` (0x0002): Keep-alive
- `MSG_CONFIG_GET` (0x0010): Request config value
- `MSG_CONFIG_SET` (0x0011): Set config value
- `MSG_PROJECT_OPEN` (0x0020): Open project notification
- `MSG_ASSET_CHANGED` (0x0030): Asset modification event
- `MSG_LOG` (0x0040): Log message from tool
- `MSG_SHUTDOWN` (0x00FF): Graceful shutdown request

## Tool Interface Contract

Each tool must implement:

```c
typedef struct ToolInterface {
    const char* name;
    const char* version;
    
    // Lifecycle
    bool (*init)(int argc, char** argv);
    void (*shutdown)(void);
    int (*run)(void);  // Main loop, returns exit code
    
    // IPC (optional)
    void (*on_message)(const IPCMessage* msg);
    void (*on_project_opened)(const char* project_path);
    void (*on_config_changed)(const char* key, const char* value);
} ToolInterface;

// Tools register with this macro
#define TOOL_REGISTER(iface) \
    const ToolInterface* tool_get_interface(void) { return &(iface); }
```

## GUI Framework

Using **Dear ImGui** with Vulkan backend:
- Immediate mode GUI (simple, fast, C++ with C wrapper)
- Vulkan-compatible (shares renderer context)
- Docking support for multi-panel layouts
- Theming via ImGui style configuration

## Build Integration

```cmake
# tools/launcher/CMakeLists.txt
add_executable(arena-launcher
    main.c
    gui.c
    process_manager.c
    ipc_router.c
)

target_link_libraries(arena-launcher
    PRIVATE tools-common
    PRIVATE arena-renderer  # For ImGui+Vulkan
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw
)
```

## Implementation Phases

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| 1 | 1 week | Process manager, basic launcher shell |
| 2 | 1 week | IPC system, message routing |
| 3 | 1.5 weeks | ImGui integration, tool panels |
| 4 | 0.5 weeks | Configuration, polish, testing |

**Total: ~4 weeks**

