# Steam API Integration for Arena Engine MOBA
## Friends List, Presence, and Overlay System

**Document Version:** 1.0  
**Date:** March 2026  
**Status:** Technical Design Document  
**Target Platform:** Linux (native + Proton), Windows  
**Engine:** Arena Engine (C-based)  
**Renderer:** Vulkan  

---

## 1. Steamworks SDK Overview

### 1.1 SDK Download and Setup

The Steamworks SDK is available from the Steamworks partner portal:
- **Official Source:** https://partner.steamgames.com/doc/sdk
- **Latest Version:** Download from Steamworks partner account
- **Size:** ~500 MB (includes documentation, examples, tools)

### 1.2 Required Library Files

For C-based integration on Linux:

```
steamworks_sdk/
├── public/steam/
│   ├── steam_api.h              # Main C API header
│   ├── steam_api_flat.h         # Flat C-style API (recommended for C)
│   ├── isteamfriends.h          # Friends interface
│   ├── isteamuser.h             # User interface
│   ├── isteamutils.h            # Utilities
│   ├── isteammatchmaking.h      # Lobbies/matchmaking
│   ├── steamtypes.h             # Type definitions
│   └── steamclientpublic.h      # Client types
├── redistributable_bin/
│   ├── linux64/libsteam_api.so  # Main library (64-bit)
│   ├── linux32/libsteam_api.so  # 32-bit (if needed)
│   └── osx64/libsteam_api.dylib # macOS (future support)
└── tools/ContentBuilder/        # For uploading builds
```

### 1.3 CMake Integration

Add to `CMakeLists.txt`:

```cmake
set(STEAMWORKS_SDK_PATH "${CMAKE_SOURCE_DIR}/libs/steamworks_sdk")
include_directories(${STEAMWORKS_SDK_PATH}/public)

if(UNIX AND NOT APPLE)
    link_directories(${STEAMWORKS_SDK_PATH}/redistributable_bin/linux64)
    link_libraries(steam_api)
endif()

add_custom_command(TARGET arena-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${STEAMWORKS_SDK_PATH}/redistributable_bin/linux64/libsteam_api.so
    $<TARGET_FILE_DIR:arena-client>/
)
```

### 1.4 Initialization and Shutdown

**Critical:** Steam must be initialized before any API calls.

```c
#include "steam/steam_api.h"
#include <stdio.h>
#include <stdbool.h>

static bool g_steam_initialized = false;

bool steam_init(uint32_t app_id) {
    if (!SteamAPI_Init()) {
        fprintf(stderr, "Steam: Failed to initialize\n");
        return false;
    }
    g_steam_initialized = true;
    printf("Steam: Initialized (App ID: %u)\n", app_id);
    return true;
}

void steam_shutdown(void) {
    if (!g_steam_initialized) return;
    SteamAPI_Shutdown();
    g_steam_initialized = false;
    printf("Steam: Shutdown\n");
}

bool steam_is_initialized(void) {
    return g_steam_initialized;
}
```

### 1.5 App ID Configuration

The App ID identifies your game on Steam. For development/testing:

```c
#define STEAM_APP_ID 480  // SpaceWar (test app)
// For production: Use your actual Steam App ID from partner portal

// Initialize early in main()
if (!steam_init(STEAM_APP_ID)) {
    fprintf(stderr, "Failed to initialize Steam\n");
    return 1;
}
```

**Important:** Create `steam_appid.txt` in game directory:

```bash
echo "480" > steam_appid.txt  # For testing with SpaceWar
```

### 1.6 License Considerations

- **Free to Use:** Steamworks SDK is free for all developers
- **No Royalties:** Valve takes a 30% cut of game sales (standard Steam revenue share)
- **Redistribution:** `libsteam_api.so` must be distributed with your game
- **Source Code:** SDK headers are provided; you don't need to distribute source
- **Linux Support:** Native Linux support is fully available; Proton is for Windows games

---

## 2. Friends System API

### 2.1 ISteamFriends Interface Overview

The `ISteamFriends` interface provides access to the user's friends list and friend information.

**Key Functions:**

| Function | Purpose | Returns |
|----------|---------|---------|
| `GetFriendCount()` | Get total number of friends | `int` |
| `GetFriendByIndex()` | Get friend Steam ID by index | `CSteamID` |
| `GetFriendPersonaName()` | Get friend's display name | `const char*` |
| `GetFriendPersonaState()` | Get friend's online status | `EPersonaState` |
| `GetFriendGamePlayed()` | Get game friend is playing | `FriendGameInfo_t` |
| `GetFriendAvatar()` | Get friend's avatar image | `int` (handle) |
| `RequestUserInformation()` | Request friend data from Steam | `bool` |

### 2.2 Friend Count and Iteration

```c
#include "steam/steam_api_flat.h"

// Get total friend count
int friend_count = SteamAPI_ISteamFriends_GetFriendCount(
    SteamAPI_SteamFriends(),
    k_EFriendFlagImmediate
);

printf("You have %d friends\n", friend_count);

// Iterate through friends
for (int i = 0; i < friend_count; i++) {
    CSteamID friend_id = SteamAPI_ISteamFriends_GetFriendByIndex(
        SteamAPI_SteamFriends(),
        i,
        k_EFriendFlagImmediate
    );
    
    const char* name = SteamAPI_ISteamFriends_GetFriendPersonaName(
        SteamAPI_SteamFriends(),
        friend_id
    );
    
    printf("Friend %d: %s (ID: %llu)\n", i, name, friend_id);
}
```

### 2.3 Friend Status Enumeration

```c
typedef enum EPersonaState {
    k_EPersonaStateOffline = 0,
    k_EPersonaStateOnline = 1,
    k_EPersonaStateBusy = 2,
    k_EPersonaStateAway = 3,
    k_EPersonaStateSnooze = 4,
    k_EPersonaStateLooking_to_trade = 5,
    k_EPersonaStateLooking_to_play = 6,
    k_EPersonaStateInvisible = 7,
} EPersonaState;

const char* persona_state_to_string(EPersonaState state) {
    switch (state) {
        case k_EPersonaStateOffline: return "Offline";
        case k_EPersonaStateOnline: return "Online";
        case k_EPersonaStateBusy: return "Busy";
        case k_EPersonaStateAway: return "Away";
        case k_EPersonaStateSnooze: return "Snooze";
        case k_EPersonaStateLooking_to_trade: return "Looking to Trade";
        case k_EPersonaStateLooking_to_play: return "Looking to Play";
        case k_EPersonaStateInvisible: return "Invisible";
        default: return "Unknown";
    }
}
```

### 2.4 Friend Information Structure

```c
typedef struct FriendInfo {
    CSteamID steam_id;
    char display_name[128];
    EPersonaState persona_state;
    char status_string[256];
    uint32_t app_id;
    char game_name[128];
    uint64_t last_online;
    bool is_in_game;
    bool is_in_lobby;
} FriendInfo;

FriendInfo get_friend_info(CSteamID friend_id) {
    FriendInfo info = {0};
    info.steam_id = friend_id;
    
    const char* name = SteamAPI_ISteamFriends_GetFriendPersonaName(
        SteamAPI_SteamFriends(),
        friend_id
    );
    strncpy(info.display_name, name, sizeof(info.display_name) - 1);
    
    info.persona_state = SteamAPI_ISteamFriends_GetFriendPersonaState(
        SteamAPI_SteamFriends(),
        friend_id
    );
    
    FriendGameInfo_t game_info = {0};
    bool has_game = SteamAPI_ISteamFriends_GetFriendGamePlayed(
        SteamAPI_SteamFriends(),
        friend_id,
        &game_info
    );
    
    if (has_game && game_info.m_gameID.AppID() != 0) {
        info.is_in_game = true;
        info.app_id = game_info.m_gameID.AppID();
    }
    
    return info;
}
```

### 2.5 Requesting User Information

For friends not in your immediate cache, request their data:

```c
bool request_friend_info(CSteamID friend_id) {
    return SteamAPI_ISteamFriends_RequestUserInformation(
        SteamAPI_SteamFriends(),
        friend_id,
        false
    );
}
```

---

## 3. Friend Presence

### 3.1 Rich Presence Overview

Rich Presence allows you to display detailed status about what a player is doing:
- In queue
- In game (with champion/role)
- In lobby
- Spectating
- Custom status

### 3.2 Setting Rich Presence

```c
#include "steam/steam_api_flat.h"

void set_rich_presence(const char* key, const char* value) {
    SteamAPI_ISteamFriends_SetRichPresence(
        SteamAPI_SteamFriends(),
        key,
        value
    );
}

void clear_rich_presence(void) {
    SteamAPI_ISteamFriends_ClearRichPresence(
        SteamAPI_SteamFriends()
    );
}

void update_presence_in_queue(void) {
    set_rich_presence("status", "In Queue");
    set_rich_presence("queue_type", "Ranked");
    set_rich_presence("estimated_wait", "2:30");
}

void update_presence_in_game(const char* champion, const char* role) {
    set_rich_presence("status", "In Game");
    set_rich_presence("champion", champion);
    set_rich_presence("role", role);
    set_rich_presence("game_mode", "5v5");
}

void update_presence_in_lobby(int player_count) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d/5", player_count);
    set_rich_presence("status", "In Lobby");
    set_rich_presence("party_size", buffer);
}

void update_presence_idle(void) {
    set_rich_presence("status", "Main Menu");
    clear_rich_presence();
}
```

### 3.3 Reading Friend Presence

```c
const char* get_friend_presence(CSteamID friend_id, const char* key) {
    return SteamAPI_ISteamFriends_GetFriendRichPresence(
        SteamAPI_SteamFriends(),
        friend_id,
        key
    );
}

bool is_friend_in_game(CSteamID friend_id) {
    const char* status = get_friend_presence(friend_id, "status");
    return status && strcmp(status, "In Game") == 0;
}

const char* get_friend_champion(CSteamID friend_id) {
    return get_friend_presence(friend_id, "champion");
}
```

### 3.4 Presence Update Frequency

- **Update Interval:** Every 5-10 seconds during gameplay
- **Batch Updates:** Combine multiple `SetRichPresence()` calls
- **Avoid Spam:** Don't update every frame; use a timer

```c
typedef struct PresenceManager {
    double last_update_time;
    double update_interval;
    char current_status[256];
} PresenceManager;

void presence_update(PresenceManager* pm, double dt) {
    pm->last_update_time += dt;
    
    if (pm->last_update_time >= pm->update_interval) {
        set_rich_presence("status", pm->current_status);
        pm->last_update_time = 0.0;
    }
}
```

---

## 4. Game Invites

### 4.1 Inviting Friends to Game

```c
#include "steam/steam_api_flat.h"

void invite_friend_to_game(CSteamID friend_id, const char* connect_string) {
    SteamAPI_ISteamFriends_InviteUserToGame(
        SteamAPI_SteamFriends(),
        friend_id,
        connect_string
    );
}

void invite_friend_to_lobby(CSteamID friend_id, uint64_t lobby_id) {
    char connect_string[256];
    uint32_t app_id = 480;
    uint64_t user_id = SteamAPI_ISteamUser_GetSteamID(
        SteamAPI_SteamUser()
    );
    
    snprintf(connect_string, sizeof(connect_string),
             "steam://joinlobby/%u/%llu/%llu",
             app_id, lobby_id, user_id);
    
    invite_friend_to_game(friend_id, connect_string);
}
```

### 4.2 Receiving Invite Callbacks

Invites are handled via Steam callbacks. Set up a callback listener:

```c
#include "steam/steam_api_flat.h"

typedef struct {
    CSteamID inviter_id;
    uint64_t lobby_id;
    uint32_t game_app_id;
} GameInviteCallback;

static GameInviteCallback g_pending_invite = {0};
static bool g_has_pending_invite = false;

void on_game_lobby_join_requested(GameLobbyJoinRequested_t* callback) {
    printf("Invite received from %llu to lobby %llu\n",
           callback->m_steamIDFriend.ConvertToUint64(),
           callback->m_steamIDLobby.ConvertToUint64());
    
    g_pending_invite.inviter_id = callback->m_steamIDFriend;
    g_pending_invite.lobby_id = callback->m_steamIDLobby.ConvertToUint64();
    g_has_pending_invite = true;
}

bool has_pending_invite(void) {
    return g_has_pending_invite;
}

GameInviteCallback get_pending_invite(void) {
    g_has_pending_invite = false;
    return g_pending_invite;
}
```

### 4.3 Deep Linking (steam:// URLs)

When a user clicks an invite link or joins via Steam overlay:

```c
void handle_steam_launch_parameters(void) {
    const char* cmd_line = SteamAPI_ISteamUtils_GetCommandLineFromOSArgsAndLaunchParams(
        SteamAPI_SteamUtils()
    );
    
    if (cmd_line && strstr(cmd_line, "+connect_lobby")) {
        printf("Joining lobby from launch parameter\n");
    }
}
```

---

## 5. Steam Overlay Integration

### 5.1 Overlay Activation

The Steam overlay is activated by default with **Shift+Tab**. Your game should:
- Pause when overlay opens
- Resume when overlay closes
- Not render UI on top of overlay

### 5.2 Overlay Callbacks

```c
#include "steam/steam_api_flat.h"

static bool g_overlay_active = false;

void on_game_overlay_activated(GameOverlayActivated_t* callback) {
    g_overlay_active = callback->m_bActive;
    
    if (g_overlay_active) {
        printf("Steam overlay activated - pausing game\n");
    } else {
        printf("Steam overlay deactivated - resuming game\n");
    }
}

bool is_overlay_active(void) {
    return g_overlay_active;
}
```

### 5.3 Overlay Position Notification

```c
void on_overlay_position_changed(GameOverlayActivated_t* callback) {
    int position = callback->m_nNotifyPosition;
    printf("Overlay position: %d\n", position);
}
```

### 5.4 Drawing Under Overlay

The Steam overlay renders on top of your game. To draw UI elements that appear under the overlay:

```c
void render_with_overlay_support(void) {
    renderer_begin_frame(g_renderer);
    render_game_world();
    render_ui();
    renderer_end_frame(g_renderer);
}
```

---

## 6. Avatar Handling

### 6.1 Avatar Image Sizes

Steam provides three avatar sizes:

| Size | Dimensions | Use Case |
|------|-----------|----------|
| Small | 32x32 | Friend list, chat |
| Medium | 64x64 | Lobby, team display |
| Large | 184x184 | Profile, detailed view |

### 6.2 Fetching Avatar Images

```c
#include "steam/steam_api_flat.h"

typedef int AvatarHandle;

AvatarHandle get_friend_avatar(CSteamID friend_id, int size) {
    return SteamAPI_ISteamFriends_GetFriendAvatar(
        SteamAPI_SteamFriends(),
        friend_id,
        size
    );
}

bool get_avatar_image_data(AvatarHandle image_handle,
                           uint32_t* width,
                           uint32_t* height,
                           uint8_t** rgba_data) {
    if (!SteamAPI_ISteamUtils_GetImageSize(
            SteamAPI_SteamUtils(),
            image_handle,
            width,
            height)) {
        return false;
    }
    
    uint32_t buffer_size = (*width) * (*height) * 4;
    *rgba_data = malloc(buffer_size);
    if (!*rgba_data) return false;
    
    if (!SteamAPI_ISteamUtils_GetImageRGBA(
            SteamAPI_SteamUtils(),
            image_handle,
            *rgba_data,
            buffer_size)) {
        free(*rgba_data);
        return false;
    }
    
    return true;
}
```

### 6.3 Avatar Texture Loading

```c
#include "renderer/renderer.h"

typedef struct {
    CSteamID friend_id;
    Texture texture;
    uint32_t width;
    uint32_t height;
    bool loaded;
} AvatarTexture;

typedef struct {
    AvatarTexture* avatars;
    uint32_t count;
    uint32_t capacity;
} AvatarCache;

Texture load_avatar_texture(AvatarCache* cache, CSteamID friend_id) {
    for (uint32_t i = 0; i < cache->count; i++) {
        if (cache->avatars[i].friend_id == friend_id && cache->avatars[i].loaded) {
            return cache->avatars[i].texture;
        }
    }
    
    AvatarHandle handle = get_friend_avatar(friend_id, 1);
    if (handle == 0) return TEXTURE_NULL;
    
    uint32_t width, height;
    uint8_t* rgba_data = NULL;
    
    if (!get_avatar_image_data(handle, &width, &height, &rgba_data)) {
        return TEXTURE_NULL;
    }
    
    Texture texture = renderer_create_texture_from_rgba(
        g_renderer,
        rgba_data,
        width,
        height
    );
    
    free(rgba_data);
    
    if (cache->count < cache->capacity) {
        AvatarTexture* av = &cache->avatars[cache->count++];
        av->friend_id = friend_id;
        av->texture = texture;
        av->width = width;
        av->height = height;
        av->loaded = true;
    }
    
    return texture;
}
```

### 6.4 Avatar Loaded Callback

```c
void on_avatar_image_loaded(AvatarImageLoaded_t* callback) {
    printf("Avatar loaded for friend %llu\n",
           callback->m_steamID.ConvertToUint64());
}
```

---

## 7. Steam Lobbies

### 7.1 Lobby System Overview

Lobbies are temporary game sessions for matchmaking and party management.

**Key Functions:**

| Function | Purpose |
|----------|---------|
| `CreateLobby()` | Create a new lobby |
| `JoinLobby()` | Join an existing lobby |
| `LeaveLobby()` | Leave current lobby |
| `GetLobbyMemberCount()` | Get number of players |
| `GetLobbyMemberByIndex()` | Get member Steam ID |
| `SetLobbyData()` | Store lobby settings |
| `GetLobbyData()` | Retrieve lobby settings |
| `SendLobbyChatMsg()` | Send chat message |

### 7.2 Creating a Lobby

```c
#include "steam/steam_api_flat.h"

typedef struct {
    uint64_t lobby_id;
    bool is_valid;
} LobbyHandle;

static LobbyHandle g_current_lobby = {0};

void create_lobby(int max_players) {
    SteamAPI_ISteamMatchmaking_CreateLobby(
        SteamAPI_SteamMatchmaking(),
        k_ELobbyTypePublic,
        max_players
    );
}

void on_lobby_created(LobbyCreated_t* callback) {
    if (callback->m_eResult == k_EResultOK) {
        g_current_lobby.lobby_id = callback->m_ulSteamIDLobby;
        g_current_lobby.is_valid = true;
        printf("Lobby created: %llu\n", g_current_lobby.lobby_id);
    } else {
        printf("Failed to create lobby\n");
    }
}
```

### 7.3 Joining a Lobby

```c
void join_lobby(uint64_t lobby_id) {
    CSteamID lobby_steam_id;
    lobby_steam_id.SetFromUint64(lobby_id);
    
    SteamAPI_ISteamMatchmaking_JoinLobby(
        SteamAPI_SteamMatchmaking(),
        lobby_steam_id
    );
}

void on_lobby_enter(LobbyEnter_t* callback) {
    if (callback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess) {
        g_current_lobby.lobby_id = callback->m_ulSteamIDLobby;
        g_current_lobby.is_valid = true;
        printf("Joined lobby: %llu\n", g_current_lobby.lobby_id);
    } else {
        printf("Failed to join lobby\n");
    }
}
```

### 7.4 Lobby Member Management

```c
int get_lobby_member_count(uint64_t lobby_id) {
    CSteamID lobby_steam_id;
    lobby_steam_id.SetFromUint64(lobby_id);
    
    return SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(
        SteamAPI_SteamMatchmaking(),
        lobby_steam_id
    );
}

CSteamID get_lobby_member(uint64_t lobby_id, int index) {
    CSteamID lobby_steam_id;
    lobby_steam_id.SetFromUint64(lobby_id);
    
    return SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(
        SteamAPI_SteamMatchmaking(),
        lobby_steam_id,
        index
    );
}

typedef struct {
    CSteamID* members;
    int count;
} LobbyMembers;

LobbyMembers get_all_lobby_members(uint64_t lobby_id) {
    LobbyMembers result = {0};
    result.count = get_lobby_member_count(lobby_id);
    result.members = malloc(result.count * sizeof(CSteamID));
    
    for (int i = 0; i < result.count; i++) {
        result.members[i] = get_lobby_member(lobby_id, i);
    }
    
    return result;
}
```

### 7.5 Lobby Data (Game Settings)

```c
void set_lobby_data(uint64_t lobby_id, const char* key, const char* value) {
    CSteamID lobby_steam_id;
    lobby_steam_id.SetFromUint64(lobby_id);
    
    SteamAPI_ISteamMatchmaking_SetLobbyData(
        SteamAPI_SteamMatchmaking(),
        lobby_steam_id,
        key,
        value
    );
}

const char* get_lobby_data(uint64_t lobby_id, const char* key) {
    CSteamID lobby_steam_id;
    lobby_steam_id.SetFromUint64(lobby_id);
    
    return SteamAPI_ISteamMatchmaking_GetLobbyData(
        SteamAPI_SteamMatchmaking(),
        lobby_steam_id,
        key
    );
}

void setup_lobby_for_ranked(uint64_t lobby_id) {
    set_lobby_data(lobby_id, "game_mode", "ranked");
    set_lobby_data(lobby_id, "map", "summoners_rift");
    set_lobby_data(lobby_id, "max_players", "5");
}
```

---

## 8. Code Implementation

### 8.1 Steam Manager Module Header

Create `src/arena/steam/steam_manager.h`:

```c
#ifndef ARENA_STEAM_MANAGER_H
#define ARENA_STEAM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "steam/steam_api_flat.h"

typedef struct SteamManager SteamManager;

// Lifecycle
SteamManager* steam_manager_create(uint32_t app_id);
void steam_manager_destroy(SteamManager* manager);

// Update (call every frame)
void steam_manager_update(SteamManager* manager, double dt);

// Friends
int steam_manager_get_friend_count(SteamManager* manager);
CSteamID steam_manager_get_friend_by_index(SteamManager* manager, int index);
const char* steam_manager_get_friend_name(SteamManager* manager, CSteamID friend_id);
EPersonaState steam_manager_get_friend_state(SteamManager* manager, CSteamID friend_id);

// Presence
void steam_manager_set_presence(SteamManager* manager, const char* key, const char* value);
void steam_manager_clear_presence(SteamManager* manager);
const char* steam_manager_get_friend_presence(SteamManager* manager, CSteamID friend_id, const char* key);

// Invites
void steam_manager_invite_friend(SteamManager* manager, CSteamID friend_id, const char* connect_string);
bool steam_manager_has_pending_invite(SteamManager* manager);
void steam_manager_get_pending_invite(SteamManager* manager, CSteamID* out_inviter, uint64_t* out_lobby);

// Lobbies
void steam_manager_create_lobby(SteamManager* manager, int max_players);
void steam_manager_join_lobby(SteamManager* manager, uint64_t lobby_id);
void steam_manager_leave_lobby(SteamManager* manager);
uint64_t steam_manager_get_current_lobby(SteamManager* manager);
int steam_manager_get_lobby_member_count(SteamManager* manager);
CSteamID steam_manager_get_lobby_member(SteamManager* manager, int index);

// Overlay
bool steam_manager_is_overlay_active(SteamManager* manager);

#endif
```

---

## 9. UI Integration

### 9.1 Friends List Panel

```c
typedef struct FriendsPanel FriendsPanel;

FriendsPanel* friends_panel_create(void);
void friends_panel_destroy(FriendsPanel* panel);
void friends_panel_update(FriendsPanel* panel, SteamManager* steam, double dt);
void friends_panel_render(FriendsPanel* panel, Renderer* renderer);
void friends_panel_on_click(FriendsPanel* panel, int x, int y);
```

### 9.2 Friend Status Indicators

```
┌─────────────────────────────────────┐
│ Friends (12)                    [×] │
├─────────────────────────────────────┤
│ ● Jinx                              │
│   In Game - Jinx (ADC)              │
│   [Invite] [Profile] [Message]      │
├─────────────────────────────────────┤
│ ● Ahri                              │
│   In Queue - Estimated 2:30         │
│   [Invite] [Profile] [Message]      │
├─────────────────────────────────────┤
│ ○ Lux                               │
│   Online - Main Menu                │
│   [Invite] [Profile] [Message]      │
├─────────────────────────────────────┤
│ ◯ Teemo                             │
│   Away                              │
│   [Profile] [Message]               │
├─────────────────────────────────────┤
│ ◯ Garen                             │
│   Offline - Last seen 2 hours ago   │
│   [Profile] [Message]               │
└─────────────────────────────────────┘

Legend:
● = Online
○ = Away/Idle
◯ = Offline
```

### 9.3 Right-Click Context Menu

```
[Invite to Game]
[View Profile]
[Send Message]
[Add to Favorites]
[Block Player]
```

### 9.4 Friend Notifications

```c
typedef enum {
    FRIEND_NOTIFICATION_CAME_ONLINE,
    FRIEND_NOTIFICATION_WENT_OFFLINE,
    FRIEND_NOTIFICATION_INVITE_RECEIVED,
    FRIEND_NOTIFICATION_INVITE_ACCEPTED,
    FRIEND_NOTIFICATION_STARTED_GAME,
} FriendNotificationType;

typedef struct {
    FriendNotificationType type;
    CSteamID friend_id;
    char friend_name[128];
    double display_time;
    double max_display_duration;
} FriendNotification;

void show_friend_notification(FriendNotification* notif) {
    // Display in corner of screen for 5 seconds
}
```

---

## 10. Fallback System

### 10.1 Non-Steam Builds

For development without Steam:

```c
#ifdef ENABLE_STEAM
    #include "steam/steam_manager.h"
    #define STEAM_ENABLED 1
#else
    #define STEAM_ENABLED 0
#endif

SteamManager* steam_manager = NULL;

#if STEAM_ENABLED
    steam_manager = steam_manager_create(STEAM_APP_ID);
    if (!steam_manager) {
        fprintf(stderr, "Warning: Steam not available\n");
    }
#endif
```

### 10.2 Offline Mode Handling

```c
bool is_steam_available(void) {
    return steam_manager != NULL;
}

int get_friend_count(void) {
    if (is_steam_available()) {
        return steam_manager_get_friend_count(steam_manager);
    }
    return 0;
}

typedef struct {
    uint64_t user_id;
    char name[128];
    bool is_online;
} LocalFriend;

typedef struct {
    LocalFriend* friends;
    int count;
} LocalFriendsList;

LocalFriendsList g_offline_friends = {0};

void load_offline_friends(void) {
    // Load from config file or database
}
```

### 10.3 CMake Build Options

```cmake
option(ENABLE_STEAM "Enable Steam integration" ON)

if(ENABLE_STEAM)
    add_definitions(-DENABLE_STEAM=1)
else()
    add_definitions(-DENABLE_STEAM=0)
endif()
```

---

## 11. Testing

### 11.1 Steam Client Requirement

- **Steam Client:** Must be running on the system
- **Test Account:** Use SpaceWar (App ID 480) for testing
- **Steamworks Partner Account:** Required for production App IDs

### 11.2 Test Accounts

Create multiple test accounts on Steam:
1. Account A (main player)
2. Account B (friend)
3. Account C (friend)

Add them as friends in Steam client before testing.

### 11.3 Testing Checklist

```
[ ] Steam initialization succeeds
[ ] Friend list loads correctly
[ ] Friend count matches Steam client
[ ] Friend names display correctly
[ ] Friend status updates in real-time
[ ] Rich presence updates visible to friends
[ ] Invites send successfully
[ ] Invites received and handled
[ ] Lobby creation works
[ ] Lobby joining works
[ ] Overlay activation detected
[ ] Avatar images load correctly
[ ] Avatar caching works
[ ] Offline mode gracefully degrades
[ ] No crashes on Steam disconnect
```

### 11.4 Debugging Steam API

Enable Steam API debugging:

```c
if (SteamAPI_IsSteamRunning()) {
    printf("Steam client is running\n");
} else {
    printf("Warning: Steam client not running\n");
}

if (SteamAPI_GetHSteamUser() == 0) {
    printf("Error: Failed to get Steam user handle\n");
}

void log_steam_callback(const char* callback_name) {
    printf("Steam Callback: %s\n", callback_name);
}
```

### 11.5 Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| "Steam not initialized" | Steam client not running | Start Steam client |
| Friend list empty | Friends not added in Steam | Add test accounts as friends |
| Invites not received | Callback not registered | Ensure `SteamAPI_RunCallbacks()` called |
| Avatar not loading | Image handle invalid | Check `GetImageSize()` return value |
| Overlay not showing | Overlay disabled in settings | Enable in Steam settings |
| Lobby creation fails | App ID mismatch | Verify `steam_appid.txt` |

---

## Appendix A: Complete Integration Example

### A.1 Main Game Loop Integration

```c
#include "arena/steam/steam_manager.h"

static SteamManager* g_steam_manager = NULL;

bool init_steam(void) {
    g_steam_manager = steam_manager_create(STEAM_APP_ID);
    if (!g_steam_manager) {
        fprintf(stderr, "Warning: Steam initialization failed\n");
        return false;
    }
    return true;
}

void shutdown_steam(void) {
    if (g_steam_manager) {
        steam_manager_destroy(g_steam_manager);
        g_steam_manager = NULL;
    }
}

void update_game_loop(double dt) {
    if (g_steam_manager) {
        steam_manager_update(g_steam_manager, dt);
        
        if (steam_manager_has_pending_invite(g_steam_manager)) {
            CSteamID inviter;
            uint64_t lobby_id;
            steam_manager_get_pending_invite(g_steam_manager, &inviter, &lobby_id);
            
            printf("Invite from %s to lobby %llu\n",
                   steam_manager_get_friend_name(g_steam_manager, inviter),
                   lobby_id);
        }
    }
}

int main(int argc, char** argv) {
    if (!init_steam()) {
        fprintf(stderr, "Steam not available\n");
    }
    
    shutdown_steam();
    return 0;
}
```

---

## Appendix B: Linux-Specific Considerations

### B.1 Proton Compatibility

For Windows games running via Proton on Linux:
- Steam API works natively through Proton
- No special handling required
- Performance impact: minimal

### B.2 Library Dependencies

```bash
ldd ./arena-client | grep steam
    libsteam_api.so => ./libsteam_api.so (0x...)

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./arena-client
```

### B.3 Steam Runtime

Steam provides a runtime environment for compatibility:
```bash
~/.steam/steam/steamruntime/run.sh ./arena-client
```

---

## Appendix C: References

- **Steamworks SDK Documentation:** https://partner.steamgames.com/doc/sdk
- **ISteamFriends Interface:** https://partner.steamgames.com/doc/api/isteamfriends
- **ISteamMatchmaking Interface:** https://partner.steamgames.com/doc/api/isteammatchmaking
- **Steam Overlay:** https://partner.steamgames.com/doc/features/overlay
- **Rich Presence:** https://partner.steamgames.com/doc/features/enhancedrichpresence
- **Steam Deck Support:** https://partner.steamgames.com/doc/steamdeck

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-08 | Initial technical design document |

---

**End of Document**

