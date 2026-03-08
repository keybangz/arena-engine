# Arena Engine Main Menu UI Design Document

**Version:** 1.0  
**Date:** 2026-03-08  
**Status:** Technical Design Document  
**Target Implementation:** 10-14 weeks  
**Target Platform:** Linux (primary), Windows, macOS  
**Engine:** Arena Engine (C-based)  
**Renderer:** Vulkan 1.2  
**UI Framework:** Dear ImGui (recommended)

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Screen Flow Architecture](#2-screen-flow-architecture)
3. [Screen Specifications](#3-screen-specifications)
4. [Visual Design System](#4-visual-design-system)
5. [UI Framework Integration](#5-ui-framework-integration)
6. [Code Architecture](#6-code-architecture)
7. [Asset Requirements](#7-asset-requirements)
8. [Input Handling](#8-input-handling)
9. [Transitions and Animations](#9-transitions-and-animations)
10. [Localization](#10-localization)
11. [Performance Guidelines](#11-performance-guidelines)
12. [Implementation Phases](#12-implementation-phases)
13. [Testing Strategy](#13-testing-strategy)
14. [Appendices](#appendices)

---

## 1. Executive Summary

### 1.1 Purpose

The Main Menu System serves as the primary user interface for the Arena Engine MOBA, providing players with access to all game features outside of active gameplay. It establishes the game's visual identity, facilitates matchmaking, and serves as the hub for player progression and social features.

### 1.2 Key Screens Overview

| Screen | Purpose | Priority |
|--------|---------|----------|
| **Splash** | Branding, asset loading | Phase 1 |
| **Login** | Authentication, account access | Phase 1 |
| **Home** | Central hub, navigation | Phase 1 |
| **Play** | Game mode selection, queue | Phase 2 |
| **Champion Select** | Draft, team composition | Phase 2 |
| **Loading** | Match preparation | Phase 2 |
| **Profile** | Stats, history, progression | Phase 3 |
| **Settings** | Configuration, preferences | Phase 1 |
| **Store** | Purchases, cosmetics | Phase 3 |
| **Social** | Friends, parties, clubs | Phase 3 |

### 1.3 Technology Stack

| Component | Technology | Rationale |
|-----------|------------|-----------|
| **UI Framework** | Dear ImGui + Custom Styling | Immediate mode, Vulkan-native, rapid iteration |
| **Rendering** | Vulkan 1.2 | Consistent with game renderer |
| **Window** | GLFW | Cross-platform, Vulkan support |
| **Font** | FreeType + stb_truetype | Flexible text rendering |
| **Images** | stb_image | Texture loading |

### 1.4 Design Goals

1. **Visual Cohesion:** Consistent MOBA aesthetic inspired by League of Legends
2. **Responsiveness:** < 50ms input latency, 60 FPS minimum
3. **Accessibility:** Keyboard navigation, screen reader hints, colorblind options
4. **Scalability:** Support 720p to 4K resolutions
5. **Modularity:** Screens as independent, hot-reloadable modules

---

## 2. Screen Flow Architecture

### 2.1 Primary Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ARENA ENGINE SCREEN FLOW                          │
└─────────────────────────────────────────────────────────────────────────────┘

                              ┌──────────┐
                              │  SPLASH  │
                              │  Screen  │
                              └────┬─────┘
                                   │ (Assets loaded)
                                   ▼
                              ┌──────────┐
                              │  LOGIN   │◄─────────────────────┐
                              │  Screen  │                      │
                              └────┬─────┘                      │
                                   │ (Authenticated)            │ (Logout)
                                   ▼                            │
    ┌──────────────────────────────────────────────────────────────────────┐
    │                              HOME SCREEN                              │
    │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐     │
    │  │  PLAY   │  │ PROFILE │  │ SOCIAL  │  │  STORE  │  │SETTINGS │     │
    │  │ Button  │  │ Button  │  │ Button  │  │ Button  │  │ Button  │     │
    │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘     │
    └───────┼────────────┼────────────┼────────────┼────────────┼──────────┘
            │            │            │            │            │
            ▼            ▼            ▼            ▼            ▼
      ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
      │   PLAY   │ │ PROFILE  │ │  SOCIAL  │ │  STORE   │ │ SETTINGS │
      │  Screen  │ │  Screen  │ │  Screen  │ │  Screen  │ │  Screen  │
      └────┬─────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
           │
           │ (Queue pop / Match found)
           ▼
     ┌───────────┐
     │ CHAMPION  │
     │  SELECT   │
     └─────┬─────┘
           │ (All locked in)
           ▼
     ┌───────────┐
     │  LOADING  │
     │  Screen   │
     └─────┬─────┘
           │ (All loaded)
           ▼
     ┌───────────┐
     │   GAME    │
     │  (HUD)    │
     └───────────┘
```

### 2.2 Screen Transition Matrix

| From \ To | Splash | Login | Home | Play | ChampSel | Loading | Profile | Settings | Store | Social |
|-----------|--------|-------|------|------|----------|---------|---------|----------|-------|--------|
| **Splash** | - | ✓ | - | - | - | - | - | - | - | - |
| **Login** | - | - | ✓ | - | - | - | - | - | - | - |
| **Home** | - | ✓ | - | ✓ | - | - | ✓ | ✓ | ✓ | ✓ |
| **Play** | - | - | ✓ | - | ✓ | - | - | - | - | - |
| **ChampSel** | - | - | ✓ | - | - | ✓ | - | - | - | - |
| **Loading** | - | - | - | - | - | - | - | - | - | - |
| **Profile** | - | - | ✓ | - | - | - | - | ✓ | - | - |
| **Settings** | - | - | ✓ | - | - | - | - | - | - | - |
| **Store** | - | - | ✓ | - | - | - | - | - | - | - |
| **Social** | - | - | ✓ | ✓ | - | - | ✓ | - | - | - |

### 2.3 Screen State Transitions

```c
// Screen transition triggers
typedef enum ScreenTransition {
    TRANS_NONE = 0,
    
    // Splash transitions
    TRANS_SPLASH_TO_LOGIN,          // Assets loaded
    
    // Login transitions
    TRANS_LOGIN_TO_HOME,            // Authentication success
    TRANS_LOGIN_ERROR,              // Authentication failed (stay on login)
    
    // Home transitions
    TRANS_HOME_TO_PLAY,
    TRANS_HOME_TO_PROFILE,
    TRANS_HOME_TO_SETTINGS,
    TRANS_HOME_TO_STORE,
    TRANS_HOME_TO_SOCIAL,
    TRANS_HOME_LOGOUT,              // Return to login
    
    // Play transitions
    TRANS_PLAY_TO_HOME,             // Cancel/back
    TRANS_PLAY_TO_CHAMPSELECT,      // Match found
    TRANS_PLAY_QUEUE_UPDATE,        // Queue status change
    
    // Champion Select transitions
    TRANS_CHAMPSELECT_TO_HOME,      // Dodge/cancel
    TRANS_CHAMPSELECT_TO_LOADING,   // All locked in
    
    // Settings/Profile/Store/Social back
    TRANS_BACK_TO_HOME,
    
    TRANS_COUNT
} ScreenTransition;
```

---

## 3. Screen Specifications

### 3.1 Splash Screen

**Purpose:** Display branding, preload critical assets, check for updates

**Duration:** 2-5 seconds (minimum 2s for branding, up to 5s for slow loads)

#### 3.1.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│                                                                          │
│                                                                          │
│                         ┌────────────────────┐                           │
│                         │                    │                           │
│                         │   ARENA ENGINE     │                           │
│                         │       LOGO         │                           │
│                         │    (512x256)       │                           │
│                         │                    │                           │
│                         └────────────────────┘                           │
│                                                                          │
│                              Loading...                                  │
│                                                                          │
│                    ████████████░░░░░░░░░░░░░░░                          │
│                    [===========            ] 45%                         │
│                                                                          │
│                        Initializing shaders...                           │
│                                                                          │
│                                                                          │
│                                                                          │
│─────────────────────────────────────────────────────────────────────────│
│  © 2026 Arena Engine Team          v0.7.0          Build 2026.03.08     │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.1.2 Components

| Component | Position | Size | Description |
|-----------|----------|------|-------------|
| Logo | Center, 30% from top | 512x256 px | Primary branding image |
| Loading Text | Center, below logo | Auto | "Loading..." status |
| Progress Bar | Center, below text | 400x8 px | Asset load progress |
| Status Text | Center, below bar | Auto | Current loading task |
| Version Info | Bottom center | Auto | Build version, copyright |

#### 3.1.3 Loading Tasks

```c
typedef enum SplashLoadStage {
    SPLASH_LOAD_SHADERS,        // Compile/load shaders
    SPLASH_LOAD_TEXTURES,       // Load texture atlases
    SPLASH_LOAD_FONTS,          // Load font files
    SPLASH_LOAD_AUDIO,          // Load audio banks
    SPLASH_LOAD_CONFIG,         // Load user config
    SPLASH_LOAD_NETWORK,        // Initialize network
    SPLASH_LOAD_VALIDATE,       // Validate game files
    SPLASH_LOAD_COMPLETE,
    SPLASH_LOAD_COUNT = SPLASH_LOAD_COMPLETE
} SplashLoadStage;

typedef struct SplashScreen {
    float progress;                 // 0.0 - 1.0
    SplashLoadStage current_stage;
    const char* status_text;
    float min_display_time;         // Minimum 2 seconds
    float elapsed_time;
    bool load_complete;

    // Assets
    TextureID logo_texture;
    float logo_alpha;               // Fade in effect
} SplashScreen;
```

#### 3.1.4 Behavior

1. **Fade In:** Logo fades in over 500ms
2. **Progress:** Bar fills as assets load (weighted by task size)
3. **Minimum Time:** Display for at least 2 seconds even if loading completes faster
4. **Auto-Transition:** When `load_complete && elapsed_time >= min_display_time`
5. **Error Handling:** On load failure, display error modal with retry/exit options

---

### 3.2 Login Screen

**Purpose:** User authentication, account creation, password recovery

#### 3.2.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│  [Background: Animated scene or static art]                              │
│                                                                          │
│                    ┌────────────────────────────────┐                    │
│                    │       ╔═══════════════════╗    │                    │
│                    │       ║   ARENA ENGINE    ║    │                    │
│                    │       ╚═══════════════════╝    │                    │
│                    │                                │                    │
│                    │   ┌──────────────────────┐    │                    │
│                    │   │ Username             │    │                    │
│                    │   └──────────────────────┘    │                    │
│                    │                                │                    │
│                    │   ┌──────────────────────┐    │                    │
│                    │   │ Password        [👁] │    │                    │
│                    │   └──────────────────────┘    │                    │
│                    │                                │                    │
│                    │   [✓] Remember me              │                    │
│                    │                                │                    │
│                    │   ┌──────────────────────┐    │                    │
│                    │   │      SIGN IN         │    │                    │
│                    │   └──────────────────────┘    │                    │
│                    │                                │                    │
│                    │   Forgot Password?             │                    │
│                    │                                │                    │
│                    │   ─────────── or ───────────   │                    │
│                    │                                │                    │
│                    │   [Create New Account]         │                    │
│                    │                                │                    │
│                    └────────────────────────────────┘                    │
│                                                                          │
│  [Settings Gear ⚙]                                   [Region: NA ▼]    │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.2.2 Components

| Component | Type | Properties |
|-----------|------|------------|
| Background | Animated/Static | Full screen, behind login panel |
| Login Panel | Container | 400x500 px, centered, semi-transparent |
| Logo | Image | 256x128 px, top of panel |
| Username Field | Text Input | 320x40 px, placeholder text |
| Password Field | Password Input | 320x40 px, show/hide toggle |
| Remember Me | Checkbox | Persist credentials locally |
| Sign In Button | Primary Button | 320x48 px, gold accent |
| Forgot Password | Link | Opens recovery modal/page |
| Create Account | Secondary Button | 320x40 px, outline style |
| Settings Gear | Icon Button | Bottom-left, opens settings |
| Region Selector | Dropdown | Bottom-right, server region |

#### 3.2.3 Data Structure

```c
typedef struct LoginScreen {
    // Input fields
    char username[64];
    char password[128];
    bool remember_me;
    bool show_password;

    // State
    LoginState state;
    char error_message[256];
    float error_display_timer;

    // Region selection
    ServerRegion selected_region;

    // UI state
    bool username_focused;
    bool password_focused;
    bool attempting_login;
    float login_progress;       // For loading spinner
} LoginScreen;

typedef enum LoginState {
    LOGIN_STATE_IDLE,
    LOGIN_STATE_VALIDATING,     // Client-side validation
    LOGIN_STATE_CONNECTING,     // Connecting to auth server
    LOGIN_STATE_AUTHENTICATING, // Waiting for auth response
    LOGIN_STATE_SUCCESS,
    LOGIN_STATE_ERROR
} LoginState;

typedef enum ServerRegion {
    REGION_NA,      // North America
    REGION_EUW,     // Europe West
    REGION_EUNE,    // Europe Nordic & East
    REGION_KR,      // Korea
    REGION_BR,      // Brazil
    REGION_LAN,     // Latin America North
    REGION_LAS,     // Latin America South
    REGION_OCE,     // Oceania
    REGION_RU,      // Russia
    REGION_TR,      // Turkey
    REGION_JP,      // Japan
    REGION_COUNT
} ServerRegion;
```

#### 3.2.4 Validation Rules

```c
typedef struct LoginValidation {
    bool username_valid;
    bool password_valid;
    const char* username_error;     // NULL if valid
    const char* password_error;     // NULL if valid
} LoginValidation;

// Validation function
LoginValidation validate_login_input(const char* username, const char* password) {
    LoginValidation result = {true, true, NULL, NULL};

    // Username validation
    size_t ulen = strlen(username);
    if (ulen == 0) {
        result.username_valid = false;
        result.username_error = "Username is required";
    } else if (ulen < 3) {
        result.username_valid = false;
        result.username_error = "Username must be at least 3 characters";
    } else if (ulen > 24) {
        result.username_valid = false;
        result.username_error = "Username must be 24 characters or less";
    }

    // Password validation
    size_t plen = strlen(password);
    if (plen == 0) {
        result.password_valid = false;
        result.password_error = "Password is required";
    } else if (plen < 8) {
        result.password_valid = false;
        result.password_error = "Password must be at least 8 characters";
    }

    return result;
}
```

#### 3.2.5 Error Messages

| Error Code | User Message | Recovery Action |
|------------|--------------|-----------------|
| AUTH_INVALID_CREDENTIALS | "Invalid username or password" | Highlight fields, offer reset |
| AUTH_ACCOUNT_LOCKED | "Account locked. Try again in X minutes" | Show timer, offer support |
| AUTH_NETWORK_ERROR | "Unable to connect to server" | Retry button, check connection |
| AUTH_SERVER_MAINTENANCE | "Server maintenance in progress" | Show ETA if available |
| AUTH_VERSION_MISMATCH | "Client update required" | Launch updater |
| AUTH_REGION_UNAVAILABLE | "Region temporarily unavailable" | Suggest alternate region |

---

### 3.3 Home Screen

**Purpose:** Central navigation hub, player status display, news/notifications

#### 3.3.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│ [Logo]  [PLAY] [PROFILE] [SOCIAL] [STORE]           🔔 ⚙ [Player Card] │
├───────────────────────────────────────────────────────────────┬──────────┤
│                                                               │          │
│   ┌─────────────────────────────────────────────────────┐    │  FRIENDS │
│   │                                                     │    │  ──────  │
│   │              FEATURED CONTENT                       │    │  🟢 Alice│
│   │           (News/Event/Promo Banner)                 │    │  🟢 Bob  │
│   │               1920 x 400 px                         │    │  🟡 Carol│
│   │                                                     │    │  ⚫ Dave │
│   │   [◄]                                        [►]    │    │          │
│   └─────────────────────────────────────────────────────┘    │  [+Add]  │
│                                                               │          │
│                      ○  ○  ●  ○  ○                           ├──────────┤
│                                                               │  PARTY   │
│   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐         │  ──────  │
│   │   RANKED     │ │   NORMAL     │ │   EVENTS     │         │  Empty   │
│   │   Season 3   │ │   Quick Play │ │   Clash      │         │          │
│   │   [🏆 Gold 2]│ │   [▶ Play]  │ │   [Mar 15]   │         │ [Invite] │
│   └──────────────┘ └──────────────┘ └──────────────┘         │          │
│                                                               │          │
│                    ┌──────────────────────┐                   │          │
│                    │                      │                   │          │
│                    │     ▶  PLAY          │                   │          │
│                    │                      │                   │          │
│                    └──────────────────────┘                   │          │
│                                                               │          │
├──────────────────────────────────────────────────────────────┴──────────┤
│  [🪙 1,250 RP]  [💎 3,400 BE]  [🎟️ 2 Tokens]                Mission▶│
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.3.2 Component Breakdown

**Header Bar (60px)**

| Element | Position | Description |
|---------|----------|-------------|
| Logo | Left, 16px margin | Small logo (48x48), clickable (returns home) |
| Nav Buttons | Left of center | PLAY, PROFILE, SOCIAL, STORE |
| Notifications | Right side | Bell icon with badge count |
| Settings | Right side | Gear icon |
| Player Card | Far right | Avatar, name, status dropdown |

**Featured Banner (400px height)**

```c
typedef struct FeaturedBanner {
    uint32_t id;
    TextureID image;
    const char* title;
    const char* subtitle;
    const char* cta_text;           // "Learn More", "Play Now", etc.
    const char* cta_link;           // Deep link or URL
    uint64_t start_time;
    uint64_t end_time;
    uint32_t priority;              // Higher = shown first
} FeaturedBanner;

typedef struct BannerCarousel {
    FeaturedBanner* banners;
    uint32_t banner_count;
    uint32_t current_index;
    float auto_rotate_timer;        // Auto-advance every 8 seconds
    float transition_progress;
} BannerCarousel;
```

**Quick Access Cards (3x cards, 300x180px each)**

| Card | Content | Action |
|------|---------|--------|
| Ranked | Current rank, LP, series info | Opens Play > Ranked |
| Normal | Quick play option | Opens Play > Normal |
| Events | Current event, time remaining | Opens event details |

**Play Button (320x64px, centered)**
- Primary CTA
- Large, prominent styling
- Hover: Glow effect
- Click: Navigate to Play screen

**Right Panel (280px width)**
- Friends list (collapsible)
- Party panel (below friends)
- Shows online friends with status
- Quick invite functionality

**Footer Bar (48px)**

| Element | Description |
|---------|-------------|
| Currency Display | RP (premium), BE (earned), Event Tokens |
| Mission Progress | Compact mission tracker, expandable |

#### 3.3.3 Home Screen Data Structure

```c
typedef struct HomeScreen {
    // Player info
    PlayerInfo player;

    // Featured content
    BannerCarousel carousel;

    // Quick access
    RankInfo ranked_info;
    EventInfo current_event;

    // Social
    FriendListEntry* friends;
    uint32_t friend_count;
    uint32_t friends_online;

    PartyInfo party;

    // Notifications
    Notification* notifications;
    uint32_t notification_count;
    uint32_t unread_count;

    // Missions
    MissionProgress* active_missions;
    uint32_t mission_count;

    // Currency
    uint32_t riot_points;
    uint32_t blue_essence;
    uint32_t event_tokens;
} HomeScreen;

typedef struct PlayerInfo {
    uint64_t account_id;
    char summoner_name[32];
    char tagline[8];                // e.g., "#NA1"
    uint32_t summoner_level;
    uint32_t profile_icon_id;
    PlayerStatus status;
} PlayerInfo;

typedef enum PlayerStatus {
    STATUS_ONLINE,
    STATUS_AWAY,
    STATUS_IN_GAME,
    STATUS_IN_QUEUE,
    STATUS_IN_CHAMP_SELECT,
    STATUS_OFFLINE,
    STATUS_DO_NOT_DISTURB
} PlayerStatus;
```

---

### 3.4 Play Screen

**Purpose:** Game mode selection, queue management

#### 3.4.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│ [◄ Back]                    SELECT GAME MODE                     [⚙]   │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌────────────────┐ ┌────────────────┐ ┌────────────────┐              │
│   │    RANKED      │ │    NORMAL      │ │    CUSTOM      │              │
│   │                │ │                │ │                │              │
│   │   [Trophy]     │ │   [Swords]     │ │   [Gear]       │              │
│   │                │ │                │ │                │              │
│   │   Compete for  │ │   Casual play  │ │   Create your  │              │
│   │   rank & glory │ │   with friends │ │   own rules    │              │
│   │                │ │                │ │                │              │
│   │   Gold II      │ │   Map: SR      │ │   [Create]     │              │
│   │   45 LP        │ │                │ │   [Join]       │              │
│   └────────────────┘ └────────────────┘ └────────────────┘              │
│                                                                          │
│   ┌────────────────┐ ┌────────────────┐ ┌────────────────┐              │
│   │   PRACTICE     │ │     ARAM       │ │    EVENT       │              │
│   │                │ │                │ │                │              │
│   │   [Target]     │ │   [Bridge]     │ │   [Star]       │              │
│   │                │ │                │ │                │              │
│   │   Training     │ │   All Random   │ │   Limited Time │              │
│   │   Tool         │ │   All Mid      │ │   Mode         │              │
│   └────────────────┘ └────────────────┘ └────────────────┘              │
│                                                                          │
│   ─────────────────────────────────────────────────────────────────────  │
│                                                                          │
│   Selected: RANKED SOLO/DUO                                              │
│                                                                          │
│   Position Preference:                                                   │
│   [Primary: 🗡️ TOP ▼]  [Secondary: 🏹 ADC ▼]  [Autofill: ✓]            │
│                                                                          │
│                    ┌──────────────────────┐                              │
│                    │    FIND MATCH        │                              │
│                    └──────────────────────┘                              │
│                                                                          │
│   Party: [Solo]  [Invite]                                                │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.4.2 Queue States

```
┌─────────────────────── QUEUE ACTIVE STATE ───────────────────────┐
│                                                                   │
│                         IN QUEUE                                  │
│                                                                   │
│                        ⏱️  2:34                                   │
│                                                                   │
│                   Estimated: ~3:00                                │
│                                                                   │
│              Searching for 9 other players...                     │
│                                                                   │
│                  ┌──────────────────┐                             │
│                  │   CANCEL QUEUE   │                             │
│                  └──────────────────┘                             │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

```
┌─────────────────────── MATCH FOUND STATE ────────────────────────┐
│                                                                   │
│                      MATCH FOUND!                                 │
│                                                                   │
│                   ┌─────────────┐                                 │
│                   │             │                                 │
│                   │    ✓  ✓     │    2/10 Accepted                │
│                   │    ✓  ⏳    │                                 │
│                   │    ⏳ ⏳    │                                 │
│                   │             │                                 │
│                   └─────────────┘                                 │
│                                                                   │
│                        ⏱️  0:08                                   │
│                                                                   │
│   ┌──────────────────┐           ┌──────────────────┐            │
│   │      ACCEPT      │           │      DECLINE     │            │
│   └──────────────────┘           └──────────────────┘            │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

#### 3.4.3 Play Screen Data Structure

```c
typedef enum GameMode {
    GAME_MODE_RANKED_SOLO,
    GAME_MODE_RANKED_FLEX,
    GAME_MODE_NORMAL_DRAFT,
    GAME_MODE_NORMAL_BLIND,
    GAME_MODE_ARAM,
    GAME_MODE_PRACTICE,
    GAME_MODE_CUSTOM,
    GAME_MODE_EVENT,            // Rotating game modes
    GAME_MODE_COUNT
} GameMode;

typedef enum QueueState {
    QUEUE_STATE_IDLE,
    QUEUE_STATE_SEARCHING,
    QUEUE_STATE_MATCH_FOUND,
    QUEUE_STATE_MATCH_ACCEPTED,
    QUEUE_STATE_MATCH_DECLINED,
    QUEUE_STATE_TIMEOUT
} QueueState;

typedef enum Position {
    POS_TOP,
    POS_JUNGLE,
    POS_MID,
    POS_ADC,
    POS_SUPPORT,
    POS_FILL,
    POS_COUNT
} Position;

typedef struct PlayScreen {
    // Mode selection
    GameMode selected_mode;
    GameMode hovered_mode;

    // Queue
    QueueState queue_state;
    float queue_time;               // Seconds in queue
    float estimated_time;           // Server-provided estimate

    // Match found
    uint8_t players_accepted;       // 0-10
    float accept_timer;             // Countdown to auto-decline
    bool player_accepted;

    // Position preference (ranked)
    Position primary_position;
    Position secondary_position;
    bool autofill_enabled;

    // Party
    uint64_t* party_members;
    uint32_t party_size;
    bool is_party_leader;
} PlayScreen;
```

---

### 3.5 Champion Select Screen

**Purpose:** Champion drafting, team composition, loadout selection

#### 3.5.1 Layout (Draft Mode)

```
┌──────────────────────────────────────────────────────────────────────────┐
│  CHAMPION SELECT                                          Timer: 0:28   │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │                         BANNING PHASE                              │  │
│  │  [BAN1] [BAN2] [BAN3] [BAN4] [BAN5]  vs  [BAN1] [BAN2] [BAN3]...   │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                           │
│  │PLAYER│ │PLAYER│ │ YOU  │ │PLAYER│ │PLAYER│     YOUR TEAM (Blue)      │
│  │  1   │ │  2   │ │      │ │  4   │ │  5   │                           │
│  │[Pick]│ │[Pick]│ │[Pick]│ │[----]│ │[----]│                           │
│  │ TOP  │ │ JGL  │ │ MID  │ │ ADC  │ │ SUP  │                           │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘                           │
│                                                                          │
│  ═══════════════════════════════════════════════════════════════════════ │
│                                                                          │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                           │
│  │ENEMY │ │ENEMY │ │ENEMY │ │ENEMY │ │ENEMY │     ENEMY TEAM (Red)      │
│  │  1   │ │  2   │ │  3   │ │  4   │ │  5   │                           │
│  │[????]│ │[????]│ │[????]│ │[????]│ │[????]│                           │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘                           │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  [Search: ________]  [Roles▼] [All▼]  [Owned✓]         Sort: [A-Z ▼]   │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │ [Aatrox] [Ahri] [Akali] [Alistar] [Amumu] [Anivia] [Annie]...   │    │
│  │ [Aphelios] [Ashe] [Aurelion] [Azir] [Bard] [Blitzcrank]...      │    │
│  │ [Brand] [Braum] [Caitlyn] [Camille] [Cassiopeia] [Cho'Gath]...  │    │
│  │ ...                                                              │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐                                                     │
│  │ SELECTED:       │  [Skin: Classic ▼]  [Summoner1: Flash ▼]           │
│  │                 │                      [Summoner2: Ignite ▼]          │
│  │    [JINX]       │                                                     │
│  │                 │  [Runes: Precision ▼]                               │
│  │   "The Loose    │                                                     │
│  │    Cannon"      │  ┌────────────────────────────────────────────────┐ │
│  │                 │  │                    LOCK IN                     │ │
│  └─────────────────┘  └────────────────────────────────────────────────┘ │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│  [Chat]  _________________________________________________  [Send]       │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.5.2 Champion Select Phases

```c
typedef enum ChampSelectPhase {
    CHAMP_SELECT_PLANNING,      // See teammates' intended picks
    CHAMP_SELECT_BAN_1,         // First ban phase (3 bans per team)
    CHAMP_SELECT_BAN_2,         // Second ban phase (2 bans per team)
    CHAMP_SELECT_PICK_1,        // First pick phase
    CHAMP_SELECT_PICK_2,        // Second pick phase
    CHAMP_SELECT_FINALIZATION,  // All picked, final changes
    CHAMP_SELECT_COMPLETE
} ChampSelectPhase;

typedef struct ChampSelectPlayer {
    uint64_t account_id;
    char summoner_name[32];
    Position assigned_position;

    // Selection
    uint32_t champion_id;           // 0 = not selected
    uint32_t skin_id;
    uint32_t summoner_spell_1;      // e.g., Flash
    uint32_t summoner_spell_2;      // e.g., Ignite
    uint32_t rune_page_id;

    // State
    bool is_picking;                // Currently their turn
    bool has_locked_in;
    bool is_local_player;

    // Intent (shown during planning)
    uint32_t intended_champion_id;
} ChampSelectPlayer;

typedef struct ChampSelectScreen {
    // Phase management
    ChampSelectPhase phase;
    float phase_timer;              // Countdown
    float total_phase_time;         // For progress bar

    // Teams
    ChampSelectPlayer blue_team[5];
    ChampSelectPlayer red_team[5];

    // Bans
    uint32_t blue_bans[5];
    uint32_t red_bans[5];

    // Local player
    uint32_t local_player_index;    // 0-4 on blue team
    bool is_our_turn;

    // Champion grid
    ChampionGridFilter filter;
    uint32_t* filtered_champions;
    uint32_t filtered_count;
    uint32_t hovered_champion;
    uint32_t selected_champion;     // Not yet locked in

    // Search
    char search_text[64];

    // Chat
    ChatState champ_select_chat;
} ChampSelectScreen;

typedef struct ChampionGridFilter {
    Position role_filter;           // POS_COUNT = all roles
    bool owned_only;
    bool favorites_only;
    ChampionSortOrder sort_order;
} ChampionGridFilter;

typedef enum ChampionSortOrder {
    SORT_ALPHABETICAL,
    SORT_MASTERY,
    SORT_RECENTLY_PLAYED,
    SORT_PICK_RATE
} ChampionSortOrder;
```

#### 3.5.3 Champion Grid

- Grid of champion portraits (64x64 px each)
- Hover: Highlight border, show name tooltip
- Unavailable (banned/picked): Grayed out, X overlay
- Not owned: Lock icon (can still hover to see info)
- Search filters in real-time
- Role icons filter by assigned position synergies

---

### 3.6 Loading Screen

**Purpose:** Display match loading progress, show team compositions

#### 3.6.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│                                                                          │
│                         [SUMMONER'S RIFT]                                │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐  │
│  │                                                                    │  │
│  │                    [MAP PREVIEW IMAGE]                             │  │
│  │                       (960 x 400)                                  │  │
│  │                                                                    │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ═══════════════════════ BLUE TEAM ══════════════════════════════════   │
│                                                                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐            │
│  │ [JINX]  │ │ [AHRI]  │ │ [DARIUS]│ │[THRESH] │ │ [LEE]   │            │
│  │         │ │         │ │         │ │         │ │         │            │
│  │ Player1 │ │ Player2 │ │ Player3 │ │ Player4 │ │ Player5 │            │
│  │ Gold II │ │ Plat IV │ │ Gold I  │ │ Gold III│ │ Gold II │            │
│  │ ████████│ │ ██████░░│ │ ████████│ │ ███░░░░░│ │ ████████│            │
│  │  100%   │ │   75%   │ │  100%   │ │   40%   │ │  100%   │            │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘            │
│                                                                          │
│  ═══════════════════════ RED TEAM ═══════════════════════════════════   │
│                                                                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐            │
│  │ [YASUO] │ │ [LUX]   │ │ [GAREN] │ │ [LEONA] │ │ [JINX]  │            │
│  │         │ │         │ │         │ │         │ │         │            │
│  │ Enemy1  │ │ Enemy2  │ │ Enemy3  │ │ Enemy4  │ │ Enemy5  │            │
│  │ Gold I  │ │ Gold II │ │ Plat IV │ │ Gold III│ │ Gold II │            │
│  │ ████████│ │ ████████│ │ ███████░│ │ ██████░░│ │ █████░░░│            │
│  │  100%   │ │  100%   │ │   90%   │ │   70%   │ │   60%   │            │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘            │
│                                                                          │
│  ────────────────────────────────────────────────────────────────────── │
│                                                                          │
│  💡 TIP: Jinx's Super Mega Death Rocket deals more damage to             │
│         low-health targets. Use it to execute enemies!                   │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.6.2 Loading Screen Data Structure

```c
typedef struct LoadingPlayerCard {
    // Player info
    uint64_t account_id;
    char summoner_name[32];
    char rank_display[16];          // "Gold II", "Unranked"

    // Champion/cosmetics
    uint32_t champion_id;
    uint32_t skin_id;
    TextureID splash_art;           // Champion loading splash

    // Loading progress
    float load_progress;            // 0.0 - 1.0
    bool load_complete;
    bool connection_lost;           // Red X indicator

    // Position
    Position assigned_position;
} LoadingPlayerCard;

typedef struct LoadingScreen {
    // Map info
    const char* map_name;
    TextureID map_preview;

    // Players
    LoadingPlayerCard blue_team[5];
    LoadingPlayerCard red_team[5];

    // Tips
    const char** tips;
    uint32_t tip_count;
    uint32_t current_tip;
    float tip_rotate_timer;

    // Overall progress
    float min_load_progress;        // Slowest loader
    bool all_loaded;
} LoadingScreen;
```

---

### 3.7 Profile Screen

**Purpose:** Display player statistics, match history, achievements

#### 3.7.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│ [◄ Back]                      PROFILE                                    │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────┐  ┌────────────────────────────────────────────┐   │
│  │                  │  │                                            │   │
│  │   [PROFILE       │  │  PlayerName#NA1                            │   │
│  │    ICON]         │  │  Level 234                                 │   │
│  │                  │  │  ─────────────────────────────────────────  │   │
│  │   128x128        │  │  Ranked Solo/Duo: 🏆 GOLD II  45 LP        │   │
│  │                  │  │  Ranked Flex:     🏆 SILVER I  88 LP        │   │
│  │                  │  │                                            │   │
│  └──────────────────┘  └────────────────────────────────────────────┘   │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │ [Overview] [Match History] [Champions] [Achievements]            │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│  ══════════════════════════ OVERVIEW ═══════════════════════════════   │
│                                                                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │
│  │   GAMES      │ │   WIN RATE   │ │    KDA       │ │   CS/MIN     │   │
│  │    152       │ │    54.3%     │ │   3.2:1      │ │    7.4       │   │
│  │   played     │ │   83W 69L    │ │ 8.2/4.1/7.8  │ │   avg        │   │
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘   │
│                                                                          │
│  ══════════════════════ TOP CHAMPIONS ══════════════════════════════   │
│                                                                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                     │
│  │ [JINX]       │ │ [AHRI]       │ │ [THRESH]     │                     │
│  │ Mastery 7    │ │ Mastery 6    │ │ Mastery 5    │                     │
│  │ 245,320 pts  │ │ 156,000 pts  │ │ 89,200 pts   │                     │
│  │ 67% WR (45g) │ │ 58% WR (32g) │ │ 52% WR (28g) │                     │
│  └──────────────┘ └──────────────┘ └──────────────┘                     │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.7.2 Match History Tab

```
┌──────────────────────────────────────────────────────────────────────────┐
│  Filter: [All Queues ▼]  [All Champions ▼]  [Last 20 Games ▼]           │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─ VICTORY ───────────────────────────────────────────────────── 2h ──┐│
│  │ [JINX] Ranked Solo │ 12/3/8 │ 245 CS │ 32:14 │ ▶ Details            ││
│  └─────────────────────────────────────────────────────────────────────┘│
│                                                                          │
│  ┌─ DEFEAT ────────────────────────────────────────────────────── 5h ──┐│
│  │ [AHRI] Ranked Solo │ 4/7/5  │ 178 CS │ 28:45 │ ▶ Details            ││
│  └─────────────────────────────────────────────────────────────────────┘│
│                                                                          │
│  ┌─ VICTORY ───────────────────────────────────────────────────── 1d ──┐│
│  │ [THRESH] Normal   │ 2/4/22 │  42 CS │ 35:12 │ ▶ Details            ││
│  └─────────────────────────────────────────────────────────────────────┘│
│                                                                          │
│  ... more matches ...                                                    │
│                                                                          │
│  [Load More]                                                             │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.7.3 Profile Data Structures

```c
typedef struct ProfileScreen {
    // Player overview
    PlayerInfo player;
    RankInfo ranked_solo;
    RankInfo ranked_flex;

    // Current tab
    ProfileTab active_tab;

    // Overview stats
    ProfileStats stats;

    // Match history
    MatchHistoryEntry* matches;
    uint32_t match_count;
    MatchHistoryFilter filter;
    bool loading_more;

    // Champion mastery
    ChampionMastery* masteries;
    uint32_t mastery_count;

    // Achievements
    Achievement* achievements;
    uint32_t achievement_count;
} ProfileScreen;

typedef enum ProfileTab {
    PROFILE_TAB_OVERVIEW,
    PROFILE_TAB_MATCH_HISTORY,
    PROFILE_TAB_CHAMPIONS,
    PROFILE_TAB_ACHIEVEMENTS
} ProfileTab;

typedef struct ProfileStats {
    uint32_t games_played;
    uint32_t wins;
    uint32_t losses;
    float win_rate;
    float average_kda;
    float average_kills;
    float average_deaths;
    float average_assists;
    float average_cs_per_min;
} ProfileStats;

typedef struct MatchHistoryEntry {
    uint64_t match_id;
    uint64_t timestamp;
    GameMode game_mode;
    uint32_t champion_id;
    bool victory;

    // Stats
    uint32_t kills;
    uint32_t deaths;
    uint32_t assists;
    uint32_t cs;
    uint32_t duration_seconds;

    // Team (for expanded view)
    uint32_t team_champions[5];
    uint32_t enemy_champions[5];
} MatchHistoryEntry;

typedef struct ChampionMastery {
    uint32_t champion_id;
    uint8_t mastery_level;          // 1-7
    uint32_t mastery_points;
    uint32_t games_played;
    float win_rate;
    float average_kda;
    uint64_t last_played;
} ChampionMastery;
```

---

### 3.8 Settings Screen

**Purpose:** Configure game options across multiple categories

#### 3.8.1 Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│ [◄ Back]                      SETTINGS                         [Reset] │
├─────────────────┬────────────────────────────────────────────────────────┤
│                 │                                                        │
│   [VIDEO]       │  ══════════════════ VIDEO ═══════════════════════════ │
│                 │                                                        │
│   [AUDIO]       │  Resolution:        [1920 x 1080 ▼]                   │
│                 │                                                        │
│   [CONTROLS]    │  Display Mode:      [● Fullscreen] [○ Windowed]       │
│                 │                      [○ Borderless]                    │
│   [INTERFACE]   │                                                        │
│                 │  VSync:             [✓ Enabled]                       │
│   [GAMEPLAY]    │                                                        │
│                 │  Frame Rate Cap:    [▬▬▬▬▬▬▬●▬▬▬] 144 FPS            │
│   [ACCOUNT]     │                                                        │
│                 │  Graphics Quality:  [○ Low] [○ Medium] [● High]       │
│   [HOTKEYS]     │                      [○ Very High] [○ Ultra]          │
│                 │                                                        │
│                 │  ──────────── Advanced ────────────                    │
│                 │                                                        │
│                 │  Shadows:           [High ▼]                           │
│                 │  Texture Quality:   [High ▼]                           │
│                 │  Effects Quality:   [High ▼]                           │
│                 │  Anti-Aliasing:     [MSAA 4x ▼]                        │
│                 │  Ambient Occlusion: [✓ Enabled]                       │
│                 │                                                        │
├─────────────────┴────────────────────────────────────────────────────────┤
│                                [Apply]  [Cancel]                         │
└──────────────────────────────────────────────────────────────────────────┘
```

#### 3.8.2 Settings Categories

**Video Settings**
| Setting | Type | Options | Default |
|---------|------|---------|---------|
| Resolution | Dropdown | System-detected | Native |
| Display Mode | Radio | Fullscreen, Windowed, Borderless | Fullscreen |
| VSync | Toggle | On/Off | On |
| Frame Rate Cap | Slider | 30-240, Unlimited | 60 |
| Graphics Quality | Preset | Low, Medium, High, Very High, Ultra | High |
| Shadows | Dropdown | Off, Low, Medium, High, Ultra | High |
| Textures | Dropdown | Low, Medium, High, Ultra | High |
| Effects | Dropdown | Low, Medium, High, Ultra | High |
| Anti-Aliasing | Dropdown | Off, FXAA, MSAA 2x/4x/8x, TAA | MSAA 4x |
| Ambient Occlusion | Toggle | On/Off | On |

**Audio Settings**
| Setting | Type | Range | Default |
|---------|------|-------|---------|
| Master Volume | Slider | 0-100 | 80 |
| Music Volume | Slider | 0-100 | 50 |
| SFX Volume | Slider | 0-100 | 80 |
| Voice Volume | Slider | 0-100 | 100 |
| Ambient Volume | Slider | 0-100 | 60 |
| Announcer Voice | Dropdown | Default, Off | Default |
| Mute When Minimized | Toggle | On/Off | On |

**Controls Settings**
| Setting | Type | Description |
|---------|------|-------------|
| Mouse Sensitivity | Slider | Camera pan speed |
| Scroll Speed | Slider | Edge scroll / wheel zoom |
| Invert Y-Axis | Toggle | Camera inversion |
| Camera Lock | Toggle | Lock camera to champion |
| Attack Move Click | Keybind | Default: A |
| Quick Cast | Toggle | Cast abilities on keypress |

**Interface Settings**
| Setting | Type | Options | Default |
|---------|------|---------|---------|
| Minimap Scale | Slider | 50-150% | 100% |
| Minimap Position | Radio | Left, Right | Right |
| HUD Scale | Slider | 75-125% | 100% |
| Chat Scale | Slider | 75-150% | 100% |
| Show Timestamps | Toggle | On/Off | Off |
| Show All Chat | Toggle | On/Off | On |
| Colorblind Mode | Dropdown | Off, Deuteranopia, Protanopia, Tritanopia | Off |

**Gameplay Settings**
| Setting | Type | Description |
|---------|------|-------------|
| Auto-Attack | Toggle | Champion auto-attacks nearby enemies |
| Show Spell Costs | Toggle | Display mana/energy costs on abilities |
| Ability Range Indicators | Dropdown | Off, Quick Cast with Indicator, Normal Cast |
| Target Champions Only | Keybind | Hold to only target champions |
| Show Attack Range | Toggle | Display champion attack range |

#### 3.8.3 Settings Data Structure

```c
typedef enum SettingsCategory {
    SETTINGS_VIDEO,
    SETTINGS_AUDIO,
    SETTINGS_CONTROLS,
    SETTINGS_INTERFACE,
    SETTINGS_GAMEPLAY,
    SETTINGS_ACCOUNT,
    SETTINGS_HOTKEYS,
    SETTINGS_COUNT
} SettingsCategory;

typedef struct VideoSettings {
    uint32_t resolution_width;
    uint32_t resolution_height;
    DisplayMode display_mode;
    bool vsync;
    uint32_t frame_rate_cap;        // 0 = unlimited
    GraphicsQuality quality_preset;

    // Advanced
    QualityLevel shadows;
    QualityLevel textures;
    QualityLevel effects;
    AntiAliasingMode anti_aliasing;
    bool ambient_occlusion;
} VideoSettings;

typedef struct AudioSettings {
    float master_volume;            // 0.0 - 1.0
    float music_volume;
    float sfx_volume;
    float voice_volume;
    float ambient_volume;
    bool mute_when_minimized;
    AnnouncerVoice announcer;
} AudioSettings;

typedef struct InterfaceSettings {
    float minimap_scale;            // 0.5 - 1.5
    bool minimap_left_side;
    float hud_scale;
    float chat_scale;
    bool show_timestamps;
    bool show_all_chat;
    ColorblindMode colorblind_mode;
} InterfaceSettings;

typedef struct GameplaySettings {
    bool auto_attack;
    bool show_spell_costs;
    CastMode ability_cast_mode;
    bool show_attack_range;
    bool enable_attack_move;
} GameplaySettings;

typedef struct SettingsScreen {
    SettingsCategory active_category;

    // Current values (modified)
    VideoSettings video;
    AudioSettings audio;
    ControlSettings controls;
    InterfaceSettings interface;
    GameplaySettings gameplay;
    HotkeySettings hotkeys;

    // Saved values (for cancel/revert)
    VideoSettings video_saved;
    AudioSettings audio_saved;
    // ... etc

    bool has_unsaved_changes;
} SettingsScreen;
```

---

## 4. Visual Design System

### 4.1 Color Palette

```c
// Primary UI Colors
#define UI_COLOR_BG_PRIMARY      0x1E2328FF  // Main background (dark blue-gray)
#define UI_COLOR_BG_SECONDARY    0x0A1428FF  // Darker panels
#define UI_COLOR_BG_TERTIARY     0x1E282DFF  // Slightly lighter panels
#define UI_COLOR_BG_OVERLAY      0x010A13CC  // Modal overlay (80% opacity)

// Accent Colors
#define UI_COLOR_GOLD            0xC89B3CFF  // Primary accent (gold)
#define UI_COLOR_GOLD_LIGHT      0xF0E6D2FF  // Lighter gold (hover states)
#define UI_COLOR_GOLD_DARK       0x785A28FF  // Darker gold (pressed states)

// Text Colors
#define UI_COLOR_TEXT_PRIMARY    0xF0E6D2FF  // Main text (light cream)
#define UI_COLOR_TEXT_SECONDARY  0xA09B8CFF  // Secondary text (muted)
#define UI_COLOR_TEXT_DISABLED   0x5B5A56FF  // Disabled text (gray)
#define UI_COLOR_TEXT_LINK       0x0AC8B9FF  // Link text (cyan)

// Team Colors
#define UI_COLOR_BLUE_TEAM       0x0397ABFF  // Blue/Order team
#define UI_COLOR_RED_TEAM        0xBE1E37FF  // Red/Chaos team

// Status Colors
#define UI_COLOR_SUCCESS         0x1FAD72FF  // Green (victory, success)
#define UI_COLOR_ERROR           0xE84057FF  // Red (defeat, error)
#define UI_COLOR_WARNING         0xF5A623FF  // Orange (warning)
#define UI_COLOR_INFO            0x0AC8B9FF  // Cyan (info)

// Rank Colors
#define UI_COLOR_IRON            0x6E6E6EFF
#define UI_COLOR_BRONZE          0x8C5A3CFF
#define UI_COLOR_SILVER          0x9AA4AFFF
#define UI_COLOR_GOLD            0xCD8837FF
#define UI_COLOR_PLATINUM        0x4FA69FFF
#define UI_COLOR_EMERALD         0x3DBE7EFF
#define UI_COLOR_DIAMOND         0x576CCEFF
#define UI_COLOR_MASTER          0x9A48C1FF
#define UI_COLOR_GRANDMASTER     0xE84057FF
#define UI_COLOR_CHALLENGER      0xF4C874FF
```

### 4.2 Color Helper Functions

```c
typedef struct Color {
    uint8_t r, g, b, a;
} Color;

// Convert hex to Color
static inline Color color_from_hex(uint32_t hex) {
    return (Color){
        .r = (hex >> 24) & 0xFF,
        .g = (hex >> 16) & 0xFF,
        .b = (hex >> 8) & 0xFF,
        .a = hex & 0xFF
    };
}

// Convert to ImGui format (ABGR for ImGui)
static inline uint32_t color_to_imgui(Color c) {
    return (c.a << 24) | (c.b << 16) | (c.g << 8) | c.r;
}

// Interpolate colors
static inline Color color_lerp(Color a, Color b, float t) {
    return (Color){
        .r = (uint8_t)(a.r + (b.r - a.r) * t),
        .g = (uint8_t)(a.g + (b.g - a.g) * t),
        .b = (uint8_t)(a.b + (b.b - a.b) * t),
        .a = (uint8_t)(a.a + (b.a - a.a) * t)
    };
}

// Adjust brightness
static inline Color color_brightness(Color c, float factor) {
    return (Color){
        .r = (uint8_t)arena_clampf(c.r * factor, 0, 255),
        .g = (uint8_t)arena_clampf(c.g * factor, 0, 255),
        .b = (uint8_t)arena_clampf(c.b * factor, 0, 255),
        .a = c.a
    };
}
```

### 4.3 Typography

**Font Family:** Spiegel (League of Legends) or open alternatives:
- **Primary:** Spiegel Regular/Bold (if licensed)
- **Alternative:** Inter, Source Sans Pro, or Noto Sans

**Font Sizes:**

| Usage | Size | Weight | Line Height |
|-------|------|--------|-------------|
| Hero Title | 48px | Bold | 56px |
| Screen Title | 32px | Bold | 40px |
| Section Header | 24px | SemiBold | 32px |
| Subheader | 20px | SemiBold | 28px |
| Body Large | 16px | Regular | 24px |
| Body | 14px | Regular | 20px |
| Caption | 12px | Regular | 16px |
| Small | 10px | Regular | 14px |

**Font Data Structure:**

```c
typedef enum FontWeight {
    FONT_WEIGHT_REGULAR,
    FONT_WEIGHT_SEMIBOLD,
    FONT_WEIGHT_BOLD
} FontWeight;

typedef enum FontSize {
    FONT_SIZE_SMALL = 10,
    FONT_SIZE_CAPTION = 12,
    FONT_SIZE_BODY = 14,
    FONT_SIZE_BODY_LARGE = 16,
    FONT_SIZE_SUBHEADER = 20,
    FONT_SIZE_SECTION = 24,
    FONT_SIZE_TITLE = 32,
    FONT_SIZE_HERO = 48
} FontSize;

typedef struct UIFont {
    ImFont* font;
    float size;
    FontWeight weight;
} UIFont;

typedef struct UIFontSet {
    UIFont regular[8];      // One per size
    UIFont semibold[8];
    UIFont bold[8];
} UIFontSet;
```

### 4.4 Button Styles

**Primary Button (Gold)**
```c
typedef struct ButtonStyle {
    Color bg_normal;
    Color bg_hover;
    Color bg_pressed;
    Color bg_disabled;

    Color border_normal;
    Color border_hover;
    Color border_pressed;
    Color border_disabled;

    Color text_normal;
    Color text_hover;
    Color text_pressed;
    Color text_disabled;

    float border_width;
    float corner_radius;
    float padding_x;
    float padding_y;
} ButtonStyle;

// Primary button (gold accent, main CTAs)
static const ButtonStyle BUTTON_STYLE_PRIMARY = {
    .bg_normal      = {0x1E, 0x28, 0x2D, 0xFF},
    .bg_hover       = {0x2D, 0x3A, 0x40, 0xFF},
    .bg_pressed     = {0x15, 0x1E, 0x22, 0xFF},
    .bg_disabled    = {0x1A, 0x1C, 0x21, 0xFF},

    .border_normal  = {0xC8, 0x9B, 0x3C, 0xFF},  // Gold
    .border_hover   = {0xF0, 0xE6, 0xD2, 0xFF},  // Light gold
    .border_pressed = {0x78, 0x5A, 0x28, 0xFF},  // Dark gold
    .border_disabled= {0x5B, 0x5A, 0x56, 0x80},

    .text_normal    = {0xCD, 0xBE, 0x91, 0xFF},
    .text_hover     = {0xF0, 0xE6, 0xD2, 0xFF},
    .text_pressed   = {0xC8, 0x9B, 0x3C, 0xFF},
    .text_disabled  = {0x5B, 0x5A, 0x56, 0xFF},

    .border_width   = 2.0f,
    .corner_radius  = 4.0f,
    .padding_x      = 24.0f,
    .padding_y      = 12.0f
};

// Secondary button (subtle, less prominent)
static const ButtonStyle BUTTON_STYLE_SECONDARY = {
    .bg_normal      = {0x00, 0x00, 0x00, 0x00},  // Transparent
    .bg_hover       = {0x1E, 0x28, 0x2D, 0xFF},
    .bg_pressed     = {0x15, 0x1E, 0x22, 0xFF},
    .bg_disabled    = {0x00, 0x00, 0x00, 0x00},

    .border_normal  = {0x5B, 0x5A, 0x56, 0xFF},  // Gray
    .border_hover   = {0xA0, 0x9B, 0x8C, 0xFF},
    .border_pressed = {0x3C, 0x3C, 0x3C, 0xFF},
    .border_disabled= {0x3C, 0x3C, 0x3C, 0x80},

    .text_normal    = {0xA0, 0x9B, 0x8C, 0xFF},
    .text_hover     = {0xF0, 0xE6, 0xD2, 0xFF},
    .text_pressed   = {0x5B, 0x5A, 0x56, 0xFF},
    .text_disabled  = {0x5B, 0x5A, 0x56, 0x80},

    .border_width   = 1.0f,
    .corner_radius  = 4.0f,
    .padding_x      = 16.0f,
    .padding_y      = 8.0f
};
```

### 4.5 Panel Styles

```c
typedef struct PanelStyle {
    Color background;
    Color border;
    float border_width;
    float corner_radius;
    float padding;
    bool has_shadow;
    Color shadow_color;
    Vec2 shadow_offset;
} PanelStyle;

// Main content panels
static const PanelStyle PANEL_STYLE_DEFAULT = {
    .background     = {0x0A, 0x14, 0x28, 0xE6},  // Dark blue, 90% opacity
    .border         = {0x1E, 0x28, 0x2D, 0xFF},
    .border_width   = 1.0f,
    .corner_radius  = 4.0f,
    .padding        = 16.0f,
    .has_shadow     = true,
    .shadow_color   = {0x00, 0x00, 0x00, 0x80},
    .shadow_offset  = {4.0f, 4.0f}
};

// Modal/popup panels
static const PanelStyle PANEL_STYLE_MODAL = {
    .background     = {0x01, 0x0A, 0x13, 0xF5},  // Very dark, 96% opacity
    .border         = {0xC8, 0x9B, 0x3C, 0xFF},  // Gold border
    .border_width   = 2.0f,
    .corner_radius  = 0.0f,                       // Sharp corners for modals
    .padding        = 24.0f,
    .has_shadow     = true,
    .shadow_color   = {0x00, 0x00, 0x00, 0xCC},
    .shadow_offset  = {8.0f, 8.0f}
};
```

---

## 5. UI Framework Integration

### 5.1 Dear ImGui + Vulkan Setup

```c
// imgui_integration.h

#ifndef ARENA_IMGUI_INTEGRATION_H
#define ARENA_IMGUI_INTEGRATION_H

#include <stdbool.h>
#include <vulkan/vulkan.h>

// Forward declarations
typedef struct GLFWwindow GLFWwindow;
typedef struct Renderer Renderer;

// ImGui context wrapper
typedef struct ImGuiContext ImGuiContext;

// Initialize ImGui with Vulkan
typedef struct ImGuiInitInfo {
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t queue_family;
    VkQueue queue;
    VkDescriptorPool descriptor_pool;
    VkRenderPass render_pass;
    uint32_t image_count;
} ImGuiInitInfo;

bool imgui_init(const ImGuiInitInfo* info);
void imgui_shutdown(void);

// Frame management
void imgui_new_frame(void);
void imgui_render(VkCommandBuffer cmd);

// Input forwarding
void imgui_process_event(GLFWwindow* window);
bool imgui_wants_keyboard(void);
bool imgui_wants_mouse(void);

#endif // ARENA_IMGUI_INTEGRATION_H
```

### 5.2 ImGui Initialization

```c
// imgui_integration.c

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "cimgui/cimgui_impl.h"
#include "imgui_integration.h"

static ImGuiContext* g_imgui_context = NULL;

bool imgui_init(const ImGuiInitInfo* info) {
    // Create ImGui context
    g_imgui_context = igCreateContext(NULL);
    if (!g_imgui_context) {
        return false;
    }

    ImGuiIO* io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initialize platform/renderer backends
    if (!ImGui_ImplGlfw_InitForVulkan(info->window, true)) {
        return false;
    }

    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = info->instance,
        .PhysicalDevice = info->physical_device,
        .Device = info->device,
        .QueueFamily = info->queue_family,
        .Queue = info->queue,
        .DescriptorPool = info->descriptor_pool,
        .RenderPass = info->render_pass,
        .MinImageCount = info->image_count,
        .ImageCount = info->image_count,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        return false;
    }

    // Apply Arena Engine theme
    setup_arena_theme();

    // Load fonts
    load_arena_fonts(io);

    return true;
}
```

### 5.3 Arena Theme Setup

```c
void setup_arena_theme(void) {
    ImGuiStyle* style = igGetStyle();

    // Sizing
    style->WindowPadding = (ImVec2){16.0f, 16.0f};
    style->FramePadding = (ImVec2){8.0f, 4.0f};
    style->ItemSpacing = (ImVec2){8.0f, 8.0f};
    style->ItemInnerSpacing = (ImVec2){4.0f, 4.0f};
    style->ScrollbarSize = 12.0f;
    style->GrabMinSize = 10.0f;

    // Borders
    style->WindowBorderSize = 1.0f;
    style->FrameBorderSize = 1.0f;
    style->PopupBorderSize = 1.0f;

    // Rounding
    style->WindowRounding = 0.0f;       // Sharp window corners
    style->FrameRounding = 4.0f;        // Rounded frames/buttons
    style->PopupRounding = 0.0f;
    style->ScrollbarRounding = 4.0f;
    style->GrabRounding = 4.0f;
    style->TabRounding = 4.0f;

    // Colors
    ImVec4* colors = style->Colors;

    // Window
    colors[ImGuiCol_WindowBg]           = (ImVec4){0.06f, 0.08f, 0.16f, 0.94f};
    colors[ImGuiCol_Border]             = (ImVec4){0.12f, 0.16f, 0.18f, 1.00f};
    colors[ImGuiCol_BorderShadow]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};

    // Title
    colors[ImGuiCol_TitleBg]            = (ImVec4){0.04f, 0.06f, 0.09f, 1.00f};
    colors[ImGuiCol_TitleBgActive]      = (ImVec4){0.04f, 0.06f, 0.09f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]   = (ImVec4){0.04f, 0.06f, 0.09f, 0.50f};

    // Frame
    colors[ImGuiCol_FrameBg]            = (ImVec4){0.04f, 0.06f, 0.09f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]     = (ImVec4){0.12f, 0.16f, 0.22f, 1.00f};
    colors[ImGuiCol_FrameBgActive]      = (ImVec4){0.08f, 0.10f, 0.14f, 1.00f};

    // Button
    colors[ImGuiCol_Button]             = (ImVec4){0.12f, 0.16f, 0.18f, 1.00f};
    colors[ImGuiCol_ButtonHovered]      = (ImVec4){0.18f, 0.23f, 0.25f, 1.00f};
    colors[ImGuiCol_ButtonActive]       = (ImVec4){0.08f, 0.12f, 0.14f, 1.00f};

    // Header (collapsing headers, menu items, selectables)
    colors[ImGuiCol_Header]             = (ImVec4){0.12f, 0.16f, 0.22f, 1.00f};
    colors[ImGuiCol_HeaderHovered]      = (ImVec4){0.78f, 0.61f, 0.24f, 0.50f};  // Gold
    colors[ImGuiCol_HeaderActive]       = (ImVec4){0.78f, 0.61f, 0.24f, 1.00f};

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]        = (ImVec4){0.04f, 0.06f, 0.09f, 1.00f};
    colors[ImGuiCol_ScrollbarGrab]      = (ImVec4){0.20f, 0.24f, 0.28f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered] = (ImVec4){0.28f, 0.32f, 0.36f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]  = (ImVec4){0.36f, 0.40f, 0.44f, 1.00f};

    // Check/Radio
    colors[ImGuiCol_CheckMark]          = (ImVec4){0.78f, 0.61f, 0.24f, 1.00f};  // Gold

    // Slider
    colors[ImGuiCol_SliderGrab]         = (ImVec4){0.78f, 0.61f, 0.24f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]   = (ImVec4){0.94f, 0.75f, 0.31f, 1.00f};

    // Text
    colors[ImGuiCol_Text]               = (ImVec4){0.94f, 0.90f, 0.82f, 1.00f};
    colors[ImGuiCol_TextDisabled]       = (ImVec4){0.36f, 0.35f, 0.34f, 1.00f};

    // Separator
    colors[ImGuiCol_Separator]          = (ImVec4){0.20f, 0.24f, 0.28f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]   = (ImVec4){0.78f, 0.61f, 0.24f, 0.78f};
    colors[ImGuiCol_SeparatorActive]    = (ImVec4){0.78f, 0.61f, 0.24f, 1.00f};

    // Resize grip
    colors[ImGuiCol_ResizeGrip]         = (ImVec4){0.20f, 0.24f, 0.28f, 0.50f};
    colors[ImGuiCol_ResizeGripHovered]  = (ImVec4){0.78f, 0.61f, 0.24f, 0.67f};
    colors[ImGuiCol_ResizeGripActive]   = (ImVec4){0.78f, 0.61f, 0.24f, 0.95f};

    // Tab
    colors[ImGuiCol_Tab]                = (ImVec4){0.08f, 0.10f, 0.14f, 1.00f};
    colors[ImGuiCol_TabHovered]         = (ImVec4){0.78f, 0.61f, 0.24f, 0.80f};
    colors[ImGuiCol_TabActive]          = (ImVec4){0.20f, 0.24f, 0.28f, 1.00f};
    colors[ImGuiCol_TabUnfocused]       = (ImVec4){0.08f, 0.10f, 0.14f, 1.00f};
    colors[ImGuiCol_TabUnfocusedActive] = (ImVec4){0.12f, 0.16f, 0.20f, 1.00f};
}
```

### 5.4 Custom Widgets

```c
// arena_widgets.h

#ifndef ARENA_WIDGETS_H
#define ARENA_WIDGETS_H

#include <stdbool.h>
#include "arena/math/math.h"

// Champion portrait with border and mastery indicator
typedef struct ChampionPortraitParams {
    uint32_t champion_id;
    uint32_t skin_id;
    uint8_t mastery_level;          // 0 = no mastery shown
    bool is_selected;
    bool is_disabled;               // Banned/already picked
    bool is_locked_in;
    float size;                     // Width/height (square)
} ChampionPortraitParams;

bool ui_champion_portrait(const ChampionPortraitParams* params);

// Progress bar with custom style
typedef struct ProgressBarParams {
    float value;                    // 0.0 - 1.0
    Vec2 size;
    uint32_t fill_color;
    uint32_t bg_color;
    const char* overlay_text;       // Optional text overlay
} ProgressBarParams;

void ui_progress_bar(const ProgressBarParams* params);

// Rank badge (icon + text)
typedef struct RankBadgeParams {
    uint8_t tier;                   // 0 = Iron, 9 = Challenger
    uint8_t division;               // 1-4 (0 for Master+)
    int32_t lp;                     // -1 to hide LP
    bool compact;                   // Small or full display
} RankBadgeParams;

void ui_rank_badge(const RankBadgeParams* params);

// Ability icon with cooldown overlay
typedef struct AbilityIconParams {
    uint32_t ability_id;
    float cooldown_remaining;       // 0 = ready
    float cooldown_total;
    float mana_cost;
    float current_mana;
    bool is_active;                 // Toggle ability on
    float size;
} AbilityIconParams;

bool ui_ability_icon(const AbilityIconParams* params);

// Dropdown with search
typedef struct SearchDropdownParams {
    const char* label;
    const char** items;
    uint32_t item_count;
    int32_t* selected_index;        // In/out
    char* search_buffer;
    size_t search_buffer_size;
    float width;
} SearchDropdownParams;

bool ui_search_dropdown(const SearchDropdownParams* params);

// Tooltip with rich content
void ui_begin_tooltip_rich(void);
void ui_end_tooltip_rich(void);
void ui_tooltip_title(const char* title);
void ui_tooltip_separator(void);
void ui_tooltip_stat(const char* label, const char* value);

#endif // ARENA_WIDGETS_H
```

---

## 6. Code Architecture

### 6.1 Screen State Machine

```c
// screen_manager.h

#ifndef ARENA_SCREEN_MANAGER_H
#define ARENA_SCREEN_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// Screen identifiers
typedef enum ScreenID {
    SCREEN_NONE = -1,
    SCREEN_SPLASH,
    SCREEN_LOGIN,
    SCREEN_HOME,
    SCREEN_PLAY,
    SCREEN_CHAMPION_SELECT,
    SCREEN_LOADING,
    SCREEN_SETTINGS,
    SCREEN_PROFILE,
    SCREEN_STORE,
    SCREEN_SOCIAL,
    SCREEN_COUNT
} ScreenID;

// Forward declaration
typedef struct ScreenManager ScreenManager;

// Screen lifecycle callbacks
typedef struct ScreenCallbacks {
    // Called when entering the screen (after transition)
    void (*on_enter)(void* screen_data, void* transition_data);

    // Called when exiting the screen (before transition)
    void (*on_exit)(void* screen_data);

    // Called every frame to update logic
    void (*update)(void* screen_data, float dt);

    // Called every frame to render UI
    void (*render)(void* screen_data);

    // Handle input events (return true if consumed)
    bool (*on_input)(void* screen_data, InputEvent* event);

    // Called when screen is paused (overlaid by modal)
    void (*on_pause)(void* screen_data);

    // Called when screen resumes from pause
    void (*on_resume)(void* screen_data);
} ScreenCallbacks;

// Screen definition
typedef struct Screen {
    ScreenID id;
    const char* name;
    ScreenCallbacks callbacks;
    void* data;                     // Screen-specific state
    size_t data_size;               // For allocation
} Screen;

// Manager creation/destruction
ScreenManager* screen_manager_create(void);
void screen_manager_destroy(ScreenManager* mgr);

// Screen registration
void screen_manager_register(ScreenManager* mgr, const Screen* screen);

// Navigation
void screen_manager_push(ScreenManager* mgr, ScreenID screen, void* transition_data);
void screen_manager_pop(ScreenManager* mgr);
void screen_manager_replace(ScreenManager* mgr, ScreenID screen, void* transition_data);
void screen_manager_go_to(ScreenManager* mgr, ScreenID screen, void* transition_data);

// Query
ScreenID screen_manager_current(const ScreenManager* mgr);
bool screen_manager_is_transitioning(const ScreenManager* mgr);

// Frame update
void screen_manager_update(ScreenManager* mgr, float dt);
void screen_manager_render(ScreenManager* mgr);
void screen_manager_handle_input(ScreenManager* mgr, InputEvent* event);

#endif // ARENA_SCREEN_MANAGER_H
```

### 6.2 Screen Manager Implementation

```c
// screen_manager.c

#include "screen_manager.h"
#include <stdlib.h>
#include <string.h>

#define MAX_SCREEN_STACK 8

struct ScreenManager {
    Screen screens[SCREEN_COUNT];
    bool screens_registered[SCREEN_COUNT];

    // Navigation stack
    ScreenID stack[MAX_SCREEN_STACK];
    uint32_t stack_size;

    // Transitions
    ScreenID transition_target;
    TransitionType transition_type;
    float transition_progress;
    float transition_duration;
    bool is_transitioning;
    void* transition_data;

    // Modals
    ScreenID modal_screen;
    bool modal_active;
};

ScreenManager* screen_manager_create(void) {
    ScreenManager* mgr = calloc(1, sizeof(ScreenManager));
    if (!mgr) return NULL;

    mgr->transition_duration = 0.3f;  // 300ms default
    return mgr;
}

void screen_manager_go_to(ScreenManager* mgr, ScreenID screen, void* data) {
    if (mgr->is_transitioning) return;
    if (!mgr->screens_registered[screen]) return;

    mgr->transition_target = screen;
    mgr->transition_type = TRANSITION_FADE;
    mgr->transition_progress = 0.0f;
    mgr->is_transitioning = true;
    mgr->transition_data = data;
}

void screen_manager_update(ScreenManager* mgr, float dt) {
    // Update transition
    if (mgr->is_transitioning) {
        mgr->transition_progress += dt / mgr->transition_duration;

        if (mgr->transition_progress >= 0.5f &&
            mgr->stack[mgr->stack_size - 1] != mgr->transition_target) {
            // Halfway point: switch screens
            ScreenID old_screen = mgr->stack[mgr->stack_size - 1];

            // Exit old screen
            if (mgr->screens[old_screen].callbacks.on_exit) {
                mgr->screens[old_screen].callbacks.on_exit(
                    mgr->screens[old_screen].data);
            }

            // Enter new screen
            mgr->stack[mgr->stack_size - 1] = mgr->transition_target;
            if (mgr->screens[mgr->transition_target].callbacks.on_enter) {
                mgr->screens[mgr->transition_target].callbacks.on_enter(
                    mgr->screens[mgr->transition_target].data,
                    mgr->transition_data);
            }
        }

        if (mgr->transition_progress >= 1.0f) {
            mgr->is_transitioning = false;
            mgr->transition_data = NULL;
        }
    }

    // Update current screen
    ScreenID current = mgr->stack[mgr->stack_size - 1];
    if (mgr->screens[current].callbacks.update) {
        mgr->screens[current].callbacks.update(
            mgr->screens[current].data, dt);
    }
}

void screen_manager_render(ScreenManager* mgr) {
    ScreenID current = mgr->stack[mgr->stack_size - 1];

    // Render current screen
    if (mgr->screens[current].callbacks.render) {
        mgr->screens[current].callbacks.render(mgr->screens[current].data);
    }

    // Apply transition effect
    if (mgr->is_transitioning) {
        float alpha;
        if (mgr->transition_progress < 0.5f) {
            alpha = mgr->transition_progress * 2.0f;  // Fade out
        } else {
            alpha = (1.0f - mgr->transition_progress) * 2.0f;  // Fade in
        }

        // Draw full-screen fade overlay
        render_fade_overlay(alpha);
    }
}
```

### 6.3 UI Element Base

```c
// ui_element.h

#ifndef ARENA_UI_ELEMENT_H
#define ARENA_UI_ELEMENT_H

#include <stdbool.h>
#include <stdint.h>
#include "arena/math/math.h"

// Base UI element
typedef struct UIElement {
    uint32_t id;                    // Unique identifier
    Vec2 position;                  // Screen position (pixels)
    Vec2 size;                      // Width/height (pixels)
    Vec2 anchor;                    // Anchor point (0-1, for layout)
    Vec2 pivot;                     // Pivot point (0-1, for rotation)

    // State
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;

    // Animation
    float opacity;                  // 0.0 - 1.0
    float scale;                    // Transform scale

    // Hierarchy
    struct UIElement* parent;
    struct UIElement* children;
    struct UIElement* next_sibling;
    uint32_t child_count;
} UIElement;

// Layout helpers
typedef enum Anchor {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_CENTER_LEFT,
    ANCHOR_CENTER,
    ANCHOR_CENTER_RIGHT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT
} Anchor;

// Get world position (accounting for parent transforms)
Vec2 ui_element_world_position(const UIElement* elem);

// Hit testing
bool ui_element_contains_point(const UIElement* elem, Vec2 point);
UIElement* ui_element_hit_test(UIElement* root, Vec2 point);

// Hierarchy
void ui_element_add_child(UIElement* parent, UIElement* child);
void ui_element_remove_child(UIElement* parent, UIElement* child);

// Layout
void ui_element_set_anchor(UIElement* elem, Anchor anchor);
void ui_element_layout(UIElement* elem, Vec2 parent_size);

#endif // ARENA_UI_ELEMENT_H
```

### 6.4 Input Event System

```c
// ui_events.h

#ifndef ARENA_UI_EVENTS_H
#define ARENA_UI_EVENTS_H

#include <stdbool.h>
#include <stdint.h>
#include "arena/math/math.h"
#include "arena/input/input.h"

typedef enum UIEventType {
    UI_EVENT_NONE,

    // Mouse events
    UI_EVENT_MOUSE_MOVE,
    UI_EVENT_MOUSE_ENTER,
    UI_EVENT_MOUSE_LEAVE,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_CLICK,
    UI_EVENT_MOUSE_DOUBLE_CLICK,
    UI_EVENT_MOUSE_DRAG_START,
    UI_EVENT_MOUSE_DRAG,
    UI_EVENT_MOUSE_DRAG_END,
    UI_EVENT_SCROLL,

    // Keyboard events
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_KEY_REPEAT,
    UI_EVENT_CHAR,              // Text input

    // Focus events
    UI_EVENT_FOCUS,
    UI_EVENT_BLUR,

    // Custom events
    UI_EVENT_VALUE_CHANGED,
    UI_EVENT_SUBMIT,
    UI_EVENT_CANCEL
} UIEventType;

typedef struct UIEvent {
    UIEventType type;
    uint32_t target_id;             // Element that triggered event

    union {
        struct {
            Vec2 position;
            Vec2 delta;
            MouseButton button;
        } mouse;

        struct {
            Key key;
            bool shift;
            bool ctrl;
            bool alt;
        } key;

        struct {
            uint32_t codepoint;     // UTF-32 character
        } character;

        struct {
            Vec2 scroll;
        } scroll;

        struct {
            float old_value;
            float new_value;
        } value;
    };

    bool consumed;                  // Set true to stop propagation
} UIEvent;

// Event dispatch
typedef bool (*UIEventHandler)(void* context, const UIEvent* event);

void ui_event_dispatch(UIEvent* event, UIElement* target);

#endif // ARENA_UI_EVENTS_H
```

---

## 7. Asset Requirements

### 7.1 Texture Assets

| Asset Category | Count | Size (each) | Format | Atlas |
|---------------|-------|-------------|--------|-------|
| Logo | 1 | 512x256 | PNG | No |
| Background Images | 6 | 1920x1080 | JPG | No |
| Champion Portraits | ~160 | 64x64 | PNG | Yes |
| Champion Splash | ~160 | 308x560 | JPG | No |
| Rank Emblems | 10 | 128x128 | PNG | Yes |
| Ability Icons | ~640 | 64x64 | PNG | Yes |
| Summoner Icons | ~20 | 64x64 | PNG | Yes |
| Role Icons | 5 | 32x32 | PNG | Yes |
| UI Icons | ~50 | 24x24 | PNG | Yes |
| Button Sprites | 4 | 9-slice | PNG | Yes |

### 7.2 Font Assets

| Font | Weights | Formats | Purpose |
|------|---------|---------|---------|
| Primary (Spiegel/Inter) | Regular, SemiBold, Bold | TTF | UI text |
| Monospace (JetBrains Mono) | Regular | TTF | Chat, numbers |
| Icon Font (Font Awesome) | Regular | TTF | UI icons |

### 7.3 Audio Assets

| Category | Files | Format | Purpose |
|----------|-------|--------|---------|
| Button Clicks | 3 | OGG | UI feedback |
| Navigation | 4 | OGG | Screen transitions |
| Notifications | 5 | OGG | Alerts, messages |
| Queue Sounds | 3 | OGG | Match found, etc. |
| Champion Select | 2 | OGG | Lock-in, timer |
| Ambient | 1 | OGG | Background music |

### 7.4 Asset Loading Strategy

```c
typedef struct AssetManifest {
    // Critical (load during splash)
    const char* splash_textures[8];
    uint32_t splash_texture_count;

    // Priority (load during login)
    const char* priority_textures[32];
    uint32_t priority_texture_count;

    // Deferred (load on demand)
    const char* deferred_textures[256];
    uint32_t deferred_texture_count;

    // Fonts
    const char* fonts[8];
    uint32_t font_count;

    // Audio
    const char* audio_files[32];
    uint32_t audio_count;
} AssetManifest;

// Async loading
typedef void (*AssetLoadCallback)(const char* path, bool success, void* user_data);

void asset_load_async(const char* path, AssetLoadCallback callback, void* user_data);
void asset_load_batch(const char** paths, uint32_t count, AssetLoadCallback callback);
float asset_get_load_progress(void);
```

---

## 8. Input Handling

### 8.1 Input Contexts

```c
typedef enum InputContext {
    INPUT_CONTEXT_MENU,             // Main menu navigation
    INPUT_CONTEXT_TEXT,             // Text input active
    INPUT_CONTEXT_CHAMPION_SELECT,  // Champion select specific
    INPUT_CONTEXT_MODAL,            // Modal dialog active
    INPUT_CONTEXT_GAME              // In-game (not main menu)
} InputContext;

typedef struct MenuInputState {
    InputContext context;

    // Focus management
    uint32_t focused_element;
    bool focus_visible;             // Show focus indicator

    // Navigation
    Vec2 navigation_direction;      // D-pad / arrow keys
    bool confirm_pressed;           // Enter / A button
    bool cancel_pressed;            // Escape / B button

    // Mouse
    Vec2 mouse_position;
    bool mouse_moved_this_frame;

    // Text input
    char text_buffer[256];
    uint32_t cursor_position;
    bool text_input_active;
} MenuInputState;
```

### 8.2 Keyboard Navigation

| Key | Action |
|-----|--------|
| Tab | Next focusable element |
| Shift+Tab | Previous focusable element |
| Arrow Keys | Navigate within container |
| Enter/Space | Activate focused element |
| Escape | Cancel / Close / Back |
| F1-F4 | Quick navigation (Play, Profile, etc.) |

### 8.3 Controller Support

| Button | Action |
|--------|--------|
| D-Pad / Left Stick | Navigate |
| A (Xbox) / X (PS) | Confirm / Select |
| B (Xbox) / Circle (PS) | Cancel / Back |
| Start | Open settings |
| LB/RB | Tab navigation |
| LT/RT | Scroll |

```c
typedef struct ControllerMapping {
    int32_t confirm_button;         // A / X
    int32_t cancel_button;          // B / Circle
    int32_t menu_button;            // Start
    int32_t tab_prev_button;        // LB
    int32_t tab_next_button;        // RB
    int32_t scroll_axis;            // RT/LT axis
    float deadzone;
} ControllerMapping;
```

---

## 9. Transitions and Animations

### 9.1 Screen Transitions

```c
typedef enum TransitionType {
    TRANSITION_NONE,
    TRANSITION_FADE,
    TRANSITION_SLIDE_LEFT,
    TRANSITION_SLIDE_RIGHT,
    TRANSITION_SLIDE_UP,
    TRANSITION_SLIDE_DOWN,
    TRANSITION_ZOOM_IN,
    TRANSITION_ZOOM_OUT,
    TRANSITION_CROSSFADE
} TransitionType;

typedef struct TransitionConfig {
    TransitionType type;
    float duration;                 // Seconds
    EasingFunction easing;
} TransitionConfig;

// Default transitions per screen change
static const TransitionConfig DEFAULT_TRANSITIONS[] = {
    [SCREEN_SPLASH]          = {TRANSITION_FADE, 0.5f, EASE_OUT_CUBIC},
    [SCREEN_LOGIN]           = {TRANSITION_FADE, 0.3f, EASE_OUT_CUBIC},
    [SCREEN_HOME]            = {TRANSITION_FADE, 0.25f, EASE_OUT_QUAD},
    [SCREEN_PLAY]            = {TRANSITION_SLIDE_RIGHT, 0.2f, EASE_OUT_CUBIC},
    [SCREEN_CHAMPION_SELECT] = {TRANSITION_ZOOM_IN, 0.4f, EASE_OUT_BACK},
    [SCREEN_LOADING]         = {TRANSITION_FADE, 0.3f, EASE_LINEAR},
    [SCREEN_SETTINGS]        = {TRANSITION_SLIDE_UP, 0.25f, EASE_OUT_CUBIC},
    [SCREEN_PROFILE]         = {TRANSITION_SLIDE_RIGHT, 0.2f, EASE_OUT_CUBIC},
};
```

### 9.2 Easing Functions

```c
typedef enum EasingFunction {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_IN_OUT_CUBIC,
    EASE_IN_BACK,
    EASE_OUT_BACK,
    EASE_IN_OUT_BACK,
    EASE_IN_ELASTIC,
    EASE_OUT_ELASTIC
} EasingFunction;

float ease(EasingFunction func, float t) {
    switch (func) {
        case EASE_LINEAR:
            return t;

        case EASE_OUT_CUBIC:
            return 1.0f - powf(1.0f - t, 3.0f);

        case EASE_OUT_BACK: {
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
        }

        // ... other easing functions
        default:
            return t;
    }
}
```

### 9.3 Element Animations

```c
typedef struct UIAnimation {
    uint32_t target_element;
    AnimationProperty property;
    float from_value;
    float to_value;
    float duration;
    float elapsed;
    EasingFunction easing;
    bool loop;
    bool ping_pong;
    AnimationCallback on_complete;
} UIAnimation;

typedef enum AnimationProperty {
    ANIM_PROP_OPACITY,
    ANIM_PROP_SCALE,
    ANIM_PROP_X,
    ANIM_PROP_Y,
    ANIM_PROP_WIDTH,
    ANIM_PROP_HEIGHT,
    ANIM_PROP_ROTATION,
    ANIM_PROP_COLOR_R,
    ANIM_PROP_COLOR_G,
    ANIM_PROP_COLOR_B,
    ANIM_PROP_COLOR_A
} AnimationProperty;

// Animation helpers
void ui_animate(uint32_t element, AnimationProperty prop, float to, float duration, EasingFunction ease);
void ui_animate_sequence(uint32_t element, UIAnimation* anims, uint32_t count);
void ui_cancel_animations(uint32_t element);
```

---

## 10. Localization

### 10.1 String Table System

```c
typedef enum Language {
    LANG_EN_US,     // English (US)
    LANG_EN_GB,     // English (UK)
    LANG_ES_ES,     // Spanish (Spain)
    LANG_ES_MX,     // Spanish (Mexico)
    LANG_FR_FR,     // French
    LANG_DE_DE,     // German
    LANG_IT_IT,     // Italian
    LANG_PT_BR,     // Portuguese (Brazil)
    LANG_RU_RU,     // Russian
    LANG_KO_KR,     // Korean
    LANG_JA_JP,     // Japanese
    LANG_ZH_CN,     // Chinese (Simplified)
    LANG_ZH_TW,     // Chinese (Traditional)
    LANG_PL_PL,     // Polish
    LANG_TR_TR,     // Turkish
    LANG_COUNT
} Language;

typedef struct LocalizedString {
    const char* key;
    const char* translations[LANG_COUNT];
} LocalizedString;

// Access
const char* loc_get(const char* key);
const char* loc_get_formatted(const char* key, ...);
void loc_set_language(Language lang);
Language loc_get_language(void);

// Example keys
#define LOC_LOGIN_TITLE         "login.title"
#define LOC_LOGIN_USERNAME      "login.username"
#define LOC_LOGIN_PASSWORD      "login.password"
#define LOC_LOGIN_REMEMBER_ME   "login.remember_me"
#define LOC_LOGIN_SIGN_IN       "login.sign_in"
#define LOC_LOGIN_ERROR_INVALID "login.error.invalid_credentials"
```

### 10.2 String File Format (JSON)

```json
{
  "metadata": {
    "language": "en_US",
    "version": "1.0.0"
  },
  "strings": {
    "login.title": "Welcome to Arena",
    "login.username": "Username",
    "login.password": "Password",
    "login.remember_me": "Remember me",
    "login.sign_in": "Sign In",
    "login.create_account": "Create Account",
    "login.forgot_password": "Forgot Password?",
    "login.error.invalid_credentials": "Invalid username or password",
    "login.error.network": "Unable to connect to server",

    "home.play": "Play",
    "home.profile": "Profile",
    "home.settings": "Settings",
    "home.store": "Store",
    "home.social": "Social",

    "play.ranked": "Ranked",
    "play.normal": "Normal",
    "play.find_match": "Find Match",
    "play.in_queue": "In Queue: {0}",
    "play.estimated_time": "Estimated: ~{0}",

    "champion_select.lock_in": "Lock In",
    "champion_select.time_remaining": "{0} seconds",
    "champion_select.your_turn": "Your Turn to Pick!"
  }
}
```

### 10.3 RTL Support

```c
typedef struct TextLayoutConfig {
    Language language;
    bool is_rtl;                    // Right-to-left
    const char* font_family;        // May differ by language
    float font_scale;               // CJK may need different scale
} TextLayoutConfig;

static const TextLayoutConfig LANGUAGE_CONFIGS[] = {
    [LANG_EN_US] = {LANG_EN_US, false, "Inter", 1.0f},
    [LANG_AR_SA] = {LANG_AR_SA, true, "Noto Sans Arabic", 1.1f},
    [LANG_KO_KR] = {LANG_KO_KR, false, "Noto Sans KR", 0.95f},
    [LANG_JA_JP] = {LANG_JA_JP, false, "Noto Sans JP", 0.95f},
    [LANG_ZH_CN] = {LANG_ZH_CN, false, "Noto Sans SC", 0.95f},
    // ...
};
```

---

## 11. Performance Guidelines

### 11.1 Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| UI Render Time | < 2ms | < 4ms |
| Input Latency | < 16ms | < 33ms |
| Frame Rate | 60 FPS | 30 FPS |
| Memory (UI) | < 64 MB | < 128 MB |
| Texture Memory | < 256 MB | < 512 MB |

### 11.2 Optimization Techniques

**Texture Atlasing**
```c
typedef struct TextureAtlas {
    TextureID texture;
    uint32_t width;
    uint32_t height;

    struct {
        const char* name;
        Rect uv_rect;               // Normalized UV coordinates
        Vec2 pixel_size;
    } regions[256];
    uint32_t region_count;
} TextureAtlas;

// Load atlas from descriptor
TextureAtlas* atlas_load(const char* atlas_path);

// Get sprite region
const Rect* atlas_get_region(const TextureAtlas* atlas, const char* name);
```

**Glyph Caching**
```c
typedef struct GlyphCache {
    TextureID atlas;
    uint32_t atlas_size;

    struct {
        uint32_t codepoint;
        uint32_t font_size;
        Rect uv_rect;
        Vec2 bearing;
        Vec2 advance;
    } glyphs[4096];
    uint32_t glyph_count;

    // LRU eviction
    uint32_t lru_order[4096];
} GlyphCache;
```

**Lazy Loading**
```c
// Only load champion data when needed
typedef struct ChampionAssetLoader {
    bool portraits_loaded;
    bool splash_loaded[160];        // Per-champion
    bool ability_icons_loaded[160];

    // Load queue
    uint32_t load_queue[32];
    uint32_t queue_size;
} ChampionAssetLoader;

void champion_assets_ensure_loaded(uint32_t champion_id);
void champion_assets_preload_visible(uint32_t* visible_ids, uint32_t count);
```

### 11.3 Draw Call Batching

```c
typedef struct UIBatch {
    TextureID texture;
    uint32_t vertex_count;
    uint32_t index_count;
    BlendMode blend_mode;
} UIBatch;

typedef struct UIBatcher {
    UIBatch batches[64];
    uint32_t batch_count;

    // Geometry
    UIVertex* vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;

    uint16_t* indices;
    uint32_t index_count;
    uint32_t index_capacity;

    // Current state
    TextureID current_texture;
    BlendMode current_blend;
} UIBatcher;

void ui_batcher_begin(UIBatcher* batcher);
void ui_batcher_draw_rect(UIBatcher* b, Rect rect, Rect uv, Color color, TextureID tex);
void ui_batcher_draw_text(UIBatcher* b, const char* text, Vec2 pos, FontID font, Color color);
void ui_batcher_flush(UIBatcher* batcher);
```

---

## 12. Implementation Phases

### Phase 1: Core Infrastructure (Weeks 1-4)

**Deliverables:**
- [ ] Screen manager system
- [ ] ImGui integration with Vulkan
- [ ] Arena theme implementation
- [ ] Basic input handling
- [ ] Asset loading framework
- [ ] Splash screen
- [ ] Login screen (mock authentication)
- [ ] Settings screen (video/audio)

**Files:**
```
src/client/ui/
├── screen_manager.h/c
├── imgui_integration.h/c
├── ui_theme.h/c
├── ui_input.h/c
└── screens/
    ├── splash_screen.h/c
    ├── login_screen.h/c
    └── settings_screen.h/c
```

**Estimated LOC:** ~2,500

### Phase 2: Play Flow (Weeks 5-8)

**Deliverables:**
- [ ] Home screen with navigation
- [ ] Play screen with mode selection
- [ ] Queue system UI
- [ ] Champion select screen
- [ ] Loading screen
- [ ] Champion grid widget
- [ ] Custom UI widgets

**Files:**
```
src/client/ui/
├── widgets/
│   ├── champion_portrait.h/c
│   ├── progress_bar.h/c
│   ├── rank_badge.h/c
│   └── search_dropdown.h/c
└── screens/
    ├── home_screen.h/c
    ├── play_screen.h/c
    ├── champion_select_screen.h/c
    └── loading_screen.h/c
```

**Estimated LOC:** ~4,000

### Phase 3: Social & Profile (Weeks 9-11)

**Deliverables:**
- [ ] Profile screen with tabs
- [ ] Match history component
- [ ] Champion mastery display
- [ ] Friends list integration
- [ ] Party system UI
- [ ] Social screen

**Files:**
```
src/client/ui/
├── widgets/
│   ├── match_card.h/c
│   ├── friend_entry.h/c
│   └── party_panel.h/c
└── screens/
    ├── profile_screen.h/c
    └── social_screen.h/c
```

**Estimated LOC:** ~2,500

### Phase 4: Polish & Performance (Weeks 12-14)

**Deliverables:**
- [ ] Screen transitions
- [ ] UI animations
- [ ] Sound effects integration
- [ ] Localization support
- [ ] Performance optimization
- [ ] Controller support
- [ ] Accessibility features

**Files:**
```
src/client/ui/
├── ui_animation.h/c
├── ui_audio.h/c
├── ui_localization.h/c
├── ui_accessibility.h/c
└── ui_controller.h/c
```

**Estimated LOC:** ~1,500

### Total Estimates

| Phase | Duration | LOC | Files |
|-------|----------|-----|-------|
| Phase 1 | 4 weeks | ~2,500 | 12 |
| Phase 2 | 4 weeks | ~4,000 | 16 |
| Phase 3 | 3 weeks | ~2,500 | 10 |
| Phase 4 | 3 weeks | ~1,500 | 8 |
| **Total** | **14 weeks** | **~10,500** | **46** |

---

## 13. Testing Strategy

### 13.1 Unit Tests

```c
// Test screen manager transitions
void test_screen_manager_transitions(void) {
    ScreenManager* mgr = screen_manager_create();

    // Register mock screens
    Screen splash = {.id = SCREEN_SPLASH, .callbacks = mock_callbacks};
    screen_manager_register(mgr, &splash);

    Screen login = {.id = SCREEN_LOGIN, .callbacks = mock_callbacks};
    screen_manager_register(mgr, &login);

    // Test navigation
    screen_manager_push(mgr, SCREEN_SPLASH, NULL);
    TEST_ASSERT_EQUAL(SCREEN_SPLASH, screen_manager_current(mgr));

    screen_manager_go_to(mgr, SCREEN_LOGIN, NULL);

    // Simulate transition completion
    for (int i = 0; i < 30; i++) {
        screen_manager_update(mgr, 0.016f);  // 60 FPS
    }

    TEST_ASSERT_EQUAL(SCREEN_LOGIN, screen_manager_current(mgr));

    screen_manager_destroy(mgr);
}

// Test input validation
void test_login_validation(void) {
    LoginValidation result;

    // Empty fields
    result = validate_login_input("", "");
    TEST_ASSERT_FALSE(result.username_valid);
    TEST_ASSERT_FALSE(result.password_valid);

    // Valid input
    result = validate_login_input("player123", "securepass123");
    TEST_ASSERT_TRUE(result.username_valid);
    TEST_ASSERT_TRUE(result.password_valid);

    // Username too short
    result = validate_login_input("ab", "securepass123");
    TEST_ASSERT_FALSE(result.username_valid);
    TEST_ASSERT_STREQ("Username must be at least 3 characters", result.username_error);
}
```

### 13.2 Integration Tests

```c
// Test full login flow
void test_login_flow_integration(void) {
    // Start at splash
    ui_init();
    screen_manager_push(g_screens, SCREEN_SPLASH, NULL);

    // Wait for splash
    simulate_frames(180);  // 3 seconds at 60 FPS
    TEST_ASSERT_EQUAL(SCREEN_LOGIN, screen_manager_current(g_screens));

    // Enter credentials
    LoginScreen* login = get_screen_data(SCREEN_LOGIN);
    strcpy(login->username, "testuser");
    strcpy(login->password, "testpass123");

    // Simulate login button click
    simulate_button_click("login_button");
    simulate_frames(60);  // Wait for auth

    TEST_ASSERT_EQUAL(SCREEN_HOME, screen_manager_current(g_screens));
}
```

### 13.3 Visual Tests

- Screenshot comparison for each screen
- Layout tests at different resolutions (720p, 1080p, 1440p, 4K)
- Color contrast validation for accessibility
- Font rendering tests for different languages

### 13.4 Performance Tests

```c
void test_ui_render_performance(void) {
    ui_init();
    screen_manager_push(g_screens, SCREEN_HOME, NULL);

    // Warm up
    for (int i = 0; i < 60; i++) {
        screen_manager_update(g_screens, 0.016f);
        screen_manager_render(g_screens);
    }

    // Measure
    double total_time = 0;
    int frame_count = 300;

    for (int i = 0; i < frame_count; i++) {
        double start = get_time();
        screen_manager_render(g_screens);
        total_time += get_time() - start;
    }

    double avg_ms = (total_time / frame_count) * 1000.0;
    TEST_ASSERT_LESS_THAN(2.0, avg_ms);  // < 2ms target
}
```

---

## Appendices

### Appendix A: File Structure

```
src/client/ui/
├── core/
│   ├── screen_manager.h/c
│   ├── ui_element.h/c
│   ├── ui_events.h/c
│   ├── ui_layout.h/c
│   └── ui_focus.h/c
├── integration/
│   ├── imgui_integration.h/c
│   ├── imgui_vulkan.h/c
│   └── imgui_glfw.h/c
├── theme/
│   ├── ui_theme.h/c
│   ├── ui_colors.h
│   ├── ui_fonts.h/c
│   └── ui_styles.h
├── widgets/
│   ├── champion_portrait.h/c
│   ├── progress_bar.h/c
│   ├── rank_badge.h/c
│   ├── ability_icon.h/c
│   ├── search_dropdown.h/c
│   ├── friend_entry.h/c
│   ├── match_card.h/c
│   └── party_panel.h/c
├── screens/
│   ├── splash_screen.h/c
│   ├── login_screen.h/c
│   ├── home_screen.h/c
│   ├── play_screen.h/c
│   ├── champion_select_screen.h/c
│   ├── loading_screen.h/c
│   ├── profile_screen.h/c
│   ├── settings_screen.h/c
│   ├── store_screen.h/c
│   └── social_screen.h/c
├── systems/
│   ├── ui_animation.h/c
│   ├── ui_audio.h/c
│   ├── ui_localization.h/c
│   ├── ui_accessibility.h/c
│   └── ui_controller.h/c
└── assets/
    ├── asset_loader.h/c
    ├── texture_atlas.h/c
    └── glyph_cache.h/c
```

### Appendix B: Asset Paths

```
assets/
├── textures/
│   ├── ui/
│   │   ├── logo.png
│   │   ├── backgrounds/
│   │   │   ├── login_bg.jpg
│   │   │   ├── home_bg.jpg
│   │   │   └── champ_select_bg.jpg
│   │   ├── buttons/
│   │   │   └── button_atlas.png
│   │   └── icons/
│   │       └── icon_atlas.png
│   ├── champions/
│   │   ├── portraits/
│   │   │   └── champion_portraits_atlas.png
│   │   └── splash/
│   │       └── [champion_name].jpg
│   └── ranks/
│       └── rank_emblems_atlas.png
├── fonts/
│   ├── Inter-Regular.ttf
│   ├── Inter-SemiBold.ttf
│   ├── Inter-Bold.ttf
│   └── FontAwesome.ttf
├── audio/
│   └── ui/
│       ├── click_1.ogg
│       ├── click_2.ogg
│       ├── hover.ogg
│       ├── queue_pop.ogg
│       └── lock_in.ogg
└── localization/
    ├── en_US.json
    ├── es_ES.json
    ├── fr_FR.json
    └── ...
```

### Appendix C: Keyboard Shortcuts

| Shortcut | Screen | Action |
|----------|--------|--------|
| Escape | Any | Back / Cancel / Close |
| Enter | Login | Submit login |
| Enter | Queue | Accept match |
| Space | Champ Select | Lock in champion |
| F1 | Home | Go to Play |
| F2 | Home | Go to Profile |
| F3 | Home | Go to Social |
| F4 | Home | Go to Settings |
| Ctrl+F | Champ Select | Focus search |
| Tab | Any | Next focusable |
| Shift+Tab | Any | Previous focusable |

### Appendix D: References

- **Dear ImGui:** https://github.com/ocornut/imgui
- **cimgui (C bindings):** https://github.com/cimgui/cimgui
- **League of Legends UI/UX:** Visual reference for MOBA aesthetics
- **Vulkan ImGui Backend:** https://github.com/ocornut/imgui/tree/master/backends
- **Arena Engine Docs:**
  - `TECHNICAL_SPEC.md` - Engine architecture
  - `ARCHITECTURE_DECISIONS.md` - Design patterns
  - `UI_CHAT_DESIGN.md` - Chat system design
  - `UI_STEAM_FRIENDS_DESIGN.md` - Social integration

---

**Document Version:** 1.0
**Last Updated:** 2026-03-08
**Status:** Ready for Implementation

