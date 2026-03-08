# Arena Engine In-Game UI/HUD System - Technical Design Document

**Version:** 1.0  
**Date:** 2026-03-08  
**Status:** Design Phase  
**Target Implementation:** 10-12 weeks  
**Renderer:** Vulkan 1.3  
**Camera:** 3D Isometric (55-60° angle)

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [HUD Layout Overview](#2-hud-layout-overview)
3. [Bottom Center - Champion Panel](#3-bottom-center---champion-panel)
4. [Top Bar](#4-top-bar)
5. [Minimap](#5-minimap)
6. [Team Frames](#6-team-frames)
7. [World-Space UI](#7-world-space-ui)
8. [Scoreboard](#8-scoreboard)
9. [Shop UI](#9-shop-ui)
10. [Ping System](#10-ping-system)
11. [Settings and Customization](#11-settings-and-customization)
12. [Code Architecture](#12-code-architecture)
13. [Rendering Pipeline](#13-rendering-pipeline)
14. [Performance Considerations](#14-performance-considerations)
15. [Implementation Phases](#15-implementation-phases)

---

## 1. Executive Summary

### 1.1 HUD Purpose and Philosophy

The Arena Engine HUD serves as the primary interface between player and game, providing:

- **Real-time Information:** Health, mana, cooldowns, gold, and game state
- **Strategic Awareness:** Minimap, team status, objective timers
- **Ability Execution:** Visual feedback for targeting and ability usage
- **Economic Management:** Item shop, gold tracking, build progression

**Design Principles:**

| Principle | Description |
|-----------|-------------|
| **Clarity** | Information hierarchy with most critical data most visible |
| **Minimalism** | Preserve gameplay area - HUD should not dominate the screen |
| **Consistency** | Uniform visual language across all UI elements |
| **Responsiveness** | Sub-frame latency for player actions and feedback |
| **Accessibility** | Colorblind options, scalable text, high contrast modes |

### 1.2 Screen Real Estate Management

**Reference Resolution:** 1920x1080 (16:9 baseline)  
**Supported Resolutions:** 1280x720 to 3840x2160  
**UI Scaling:** 0.75x to 1.5x (user configurable)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          TOP BAR (48px)                                 │
│               Timer | Scores | Objective Timers                         │
├─────────────────────────────────────────────────────────────────────────┤
│         │                                                     │         │
│  ALLY   │                                                     │         │
│ FRAMES  │                                                     │         │
│  (96px) │              GAMEPLAY AREA                          │ MINIMAP │
│         │           (~70% screen area)                        │ (220px) │
│         │                                                     │         │
│         │                                                     │         │
├─────────┴─────────────────────────────────────────────────────┴─────────┤
│                       BOTTOM BAR (120px)                                │
│  Chat | Portrait | HP/MP | Abilities | Items | Stats | Gold            │
└─────────────────────────────────────────────────────────────────────────┘
```

**Screen Area Allocation:**

| Region | Size (1080p) | Percentage |
|--------|--------------|------------|
| Gameplay Area | ~1700x840 | ~68% |
| Top Bar | 1920x48 | ~4% |
| Bottom Bar | 1920x120 | ~10% |
| Minimap | 220x220 | ~2% |
| Team Frames | 96x500 | ~2% |
| Margins/Padding | - | ~14% |

### 1.3 Performance Requirements

| Metric | Target | Maximum |
|--------|--------|---------|
| UI Render Time | <0.5ms | 1.0ms |
| Draw Calls (UI) | <50 | 100 |
| Texture Memory | <32MB | 64MB |
| CPU Time (update) | <0.2ms | 0.5ms |
| Input Latency | <1 frame | 2 frames |

**Optimization Targets:**

```c
// Performance budgets (microseconds per frame at 60 FPS)
#define HUD_UPDATE_BUDGET_US    200     // CPU update time
#define HUD_RENDER_BUDGET_US    500     // GPU render time
#define HUD_BATCH_TARGET        32      // Target draw calls
#define HUD_TEXTURE_ATLAS_SIZE  2048    // 2K texture atlas
```

---

## 2. HUD Layout Overview

### 2.1 Complete Layout Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ [Game Timer]            [Team Kills: 5 - 3]           [Dragon 2:30] [Baron]│ <- Top Bar (48px)
├───────┬─────────────────────────────────────────────────────────────────────┤
│ Ally1 │                                                         ┌──────────┤
│ Ally2 │                                                         │ MINIMAP  │
│ Ally3 │                                                         │          │
│ Ally4 │                   GAMEPLAY AREA                         │  220x220 │
│       │                (3D Isometric View)                      │          │
│       │                                                         │          │
│       │                                                         └──────────┤
├───────┴──────────────────────────────────────────────────────────┬─────────┤
│ ┌─────┐  ┌─────────────────────────────────────────────────────┐ │ STATS   │
│ │CHAT │  │[P][HP BAR][MP BAR] [Q][W][E][R] [1][2][3][4][5][6]  │ │         │
│ │     │  │  Portrait  Bars     Abilities      Item Slots       │ │ ═══════ │
│ └─────┘  └─────────────────────────────────────────────────────┘ │Gold:1234│
└──────────────────────────────────────────────────────────────────┴─────────┘
                                                                    <- Bottom Bar (120px)
```

### 2.2 Z-Order (Render Layers)

```c
typedef enum HUDLayer {
    HUD_LAYER_WORLD_SPACE   = 0,    // Health bars, indicators (3D projected)
    HUD_LAYER_BACKGROUND    = 100,  // HUD panel backgrounds
    HUD_LAYER_CONTENT       = 200,  // Icons, bars, text
    HUD_LAYER_OVERLAY       = 300,  // Cooldown sweeps, highlights
    HUD_LAYER_TOOLTIP       = 400,  // Tooltips, hover info
    HUD_LAYER_POPUP         = 500,  // Shop, scoreboard, menus
    HUD_LAYER_MODAL         = 600,  // Confirmation dialogs
    HUD_LAYER_DEBUG         = 999,  // Debug overlays
} HUDLayer;
```

### 2.3 Anchor System

All HUD elements use an anchor-based positioning system for resolution independence:

```c
typedef enum HUDAnchor {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_CENTER_LEFT,
    ANCHOR_CENTER,
    ANCHOR_CENTER_RIGHT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT,
} HUDAnchor;

typedef struct HUDRect {
    HUDAnchor anchor;
    Vec2 offset;        // Offset from anchor point (pixels)
    Vec2 size;          // Element size (pixels)
    float scale;        // UI scale multiplier
} HUDRect;

// Convert anchor-relative position to screen coordinates
Vec2 hud_rect_to_screen(const HUDRect* rect, Vec2 screen_size);
```

---

## 3. Bottom Center - Champion Panel

The champion panel is the most information-dense area of the HUD, containing essential moment-to-moment data.

### 3.1 Champion Portrait

```c
typedef struct ChampionPortrait {
    uint32_t champion_id;
    uint32_t texture_id;        // Portrait texture
    uint8_t level;              // Current level (1-18)
    float exp_current;          // Current XP in level
    float exp_required;         // XP required for next level
    bool level_up_available;    // Skill point available
    bool is_dead;               // Gray out when dead
    float respawn_timer;        // Seconds until respawn
} ChampionPortrait;
```

**Visual Specifications:**

| Property | Value |
|----------|-------|
| Size | 64x64 pixels |
| Border | 2px, team color |
| Level Badge | 16x16, bottom-right |
| XP Bar | 64x4, below portrait |
| Level Up Glow | Pulsing gold border |
| Dead State | Grayscale + respawn timer |

**Portrait States:**

```c
typedef enum PortraitState {
    PORTRAIT_NORMAL,            // Standard display
    PORTRAIT_LEVEL_UP,          // Glowing, skill point available
    PORTRAIT_LOW_HEALTH,        // Pulsing red border (<25% HP)
    PORTRAIT_DEAD,              // Grayscale with timer overlay
    PORTRAIT_RESPAWNING,        // Countdown visible
} PortraitState;
```

**XP Bar Implementation:**

```c
typedef struct XPBar {
    float progress;             // 0.0 - 1.0
    uint32_t fill_color;        // 0xFFFFD700 (gold)
    uint32_t bg_color;          // 0xFF333333 (dark gray)
    bool animate_level_up;      // Flash on level gain
} XPBar;

void xp_bar_render(const XPBar* bar, Vec2 position, float width, float height);
```

### 3.2 Health and Mana Bars

```c
typedef struct ResourceBar {
    float current;              // Current value
    float maximum;              // Maximum value
    float previous;             // For damage/heal animation
    float regen_per_second;     // Regeneration rate
    uint32_t primary_color;     // Main bar color
    uint32_t secondary_color;   // Background color
    uint32_t damage_color;      // Recent damage (red fade)
    uint32_t heal_color;        // Recent healing (green flash)
    bool show_numbers;          // Display "523 / 890"
    bool show_percentage;       // Display "58%"
} ResourceBar;

typedef struct ResourceDisplay {
    ResourceBar health;
    ResourceBar mana;           // NULL for manaless champions
    ResourceBar secondary;      // Energy, rage, etc.
} ResourceDisplay;
```

**Visual Specifications:**

| Property | Health Bar | Mana Bar | Secondary |
|----------|------------|----------|-----------|
| Width | 200px | 200px | 200px |
| Height | 20px | 16px | 12px |
| Normal Color | Green (#00CC00) | Blue (#0066FF) | Varies |
| Low Color (<25%) | Red (#FF3333) | N/A | N/A |
| Background | Dark Gray (#222222) | Dark Blue (#111133) | Varies |
| Border | 1px black | 1px black | 1px black |
| Font | 12px, white, center | 12px, white, center | 10px |

**Bar Animation System:**

```c
typedef struct BarAnimation {
    float target_value;         // Where bar is animating to
    float display_value;        // Current displayed value
    float velocity;             // Animation speed
    float damage_chunk;         // Red "damage taken" section
    float damage_fade_timer;    // Time until damage chunk fades
} BarAnimation;

// Smooth bar transitions (damage shows red, then fades)
void resource_bar_animate(ResourceBar* bar, float new_value, float dt);

// Visual constants
#define BAR_ANIMATE_SPEED       5.0f    // Units per second
#define BAR_DAMAGE_FADE_DELAY   0.5f    // Seconds before fade
#define BAR_DAMAGE_FADE_SPEED   2.0f    // Fade speed
```

### 3.3 Ability Bar

The ability bar displays the champion's 4 active abilities (Q, W, E, R) plus the passive ability.

```c
typedef enum AbilityState {
    ABILITY_STATE_READY,        // Can be cast (bright)
    ABILITY_STATE_COOLDOWN,     // On cooldown (dark + sweep)
    ABILITY_STATE_NO_MANA,      // Not enough mana (blue tint)
    ABILITY_STATE_DISABLED,     // Cannot cast (gray, locked)
    ABILITY_STATE_CHARGING,     // Channeling/charging
    ABILITY_STATE_ACTIVE,       // Currently active (glowing border)
} AbilityState;

typedef struct AbilitySlot {
    uint32_t ability_id;        // Ability definition ID
    uint32_t icon_texture;      // Icon texture ID
    char hotkey;                // 'Q', 'W', 'E', 'R'

    // State
    AbilityState state;
    uint8_t level;              // 0 = not learned, 1-5 = rank
    bool can_level_up;          // Skill point available for this

    // Cooldown
    float cooldown_remaining;   // Seconds remaining
    float cooldown_total;       // Total cooldown duration

    // Cost
    float mana_cost;            // Mana/resource cost
    float current_mana;         // For cost comparison

    // Charges (for charge-based abilities)
    uint8_t charges_current;    // Current charges
    uint8_t charges_max;        // Maximum charges
    float charge_timer;         // Time until next charge
} AbilitySlot;
```

**Visual Specifications:**

| Property | Value |
|----------|-------|
| Icon Size | 48x48 pixels |
| Spacing | 4px between icons |
| Passive Icon | 40x40, left of Q |
| Hotkey Label | 10px, top-left corner |
| Cooldown Number | 16px bold, center |
| Mana Cost | 10px, below icon |
| Level Pips | 5 dots below icon |

**Cooldown Overlay (Radial Sweep):**

```c
typedef struct CooldownOverlay {
    float progress;             // 0.0 (full CD) to 1.0 (ready)
    uint32_t overlay_color;     // Semi-transparent black
    bool show_timer;            // Show seconds remaining
    float sweep_angle;          // Current angle (radians)
} CooldownOverlay;

// Renders a clock-wipe effect over ability icon
void cooldown_overlay_render(
    const CooldownOverlay* overlay,
    Vec2 center,
    float radius
);

// Shader for radial cooldown sweep
// Uses UV coordinates to mask based on angle from center
```

**Level Pips:**

```c
typedef struct LevelPips {
    uint8_t current_level;      // 0-5
    uint8_t max_level;          // Usually 5
    bool can_level_up;          // Highlight next pip
    uint32_t filled_color;      // Gold for filled
    uint32_t empty_color;       // Gray for empty
    uint32_t available_color;   // Green for levelable
} LevelPips;

// Render 5 small circles below ability icon
void level_pips_render(const LevelPips* pips, Vec2 position);
```

### 3.4 Item Slots

```c
typedef struct ItemSlot {
    uint32_t item_id;           // 0 = empty slot
    uint32_t icon_texture;      // Item icon texture
    char hotkey;                // '1', '2', '3', '4', '5', '6'

    // Stack/charges
    uint8_t stack_count;        // For consumables (wards, pots)
    uint8_t max_stacks;         // Maximum stack size

    // Active item cooldown
    bool has_active;            // Item has active ability
    float active_cooldown;      // Remaining cooldown
    float active_cooldown_max;  // Total cooldown

    // State
    bool is_empty;              // No item in slot
    bool can_sell;              // Within sell window
    float sell_timer;           // Undo timer remaining
} ItemSlot;

typedef struct ItemBar {
    ItemSlot slots[6];          // 6 item slots
    uint32_t selected_slot;     // Currently selected (for shop)
    bool show_hotkeys;          // Display 1-6 labels
} ItemBar;
```

**Visual Specifications:**

| Property | Value |
|----------|-------|
| Slot Size | 40x40 pixels |
| Spacing | 2px between slots |
| Empty Slot | Dark border, no fill |
| Stack Badge | 12px, bottom-right |
| Hotkey Label | 8px, top-left |
| Active Cooldown | Same as ability cooldown |

**Item Slot States:**

```c
typedef enum ItemSlotState {
    ITEM_SLOT_EMPTY,            // Dark, no item
    ITEM_SLOT_FILLED,           // Has item, normal display
    ITEM_SLOT_ON_COOLDOWN,      // Active item cooling down
    ITEM_SLOT_HOVER,            // Mouse hovering (tooltip)
    ITEM_SLOT_SELLABLE,         // Gold border (can undo)
} ItemSlotState;
```

### 3.5 Stats Panel

```c
typedef struct StatsDisplay {
    // Offensive stats
    float attack_damage;        // AD
    float ability_power;        // AP
    float attack_speed;         // AS (attacks per second)
    float crit_chance;          // 0.0 - 1.0
    float armor_pen;            // Flat armor penetration
    float magic_pen;            // Flat magic penetration
    float armor_pen_percent;    // % armor penetration
    float magic_pen_percent;    // % magic penetration
    float lifesteal;            // Life steal percentage
    float omnivamp;             // All damage healing

    // Defensive stats
    float armor;                // Physical resistance
    float magic_resist;         // Magic resistance
    float health_regen;         // HP5 (per 5 seconds)
    float mana_regen;           // MP5 (per 5 seconds)

    // Utility stats
    float movement_speed;       // Units per second
    float ability_haste;        // Cooldown reduction
    float tenacity;             // CC duration reduction

    // Economy
    uint32_t gold;              // Current gold
    uint32_t cs;                // Creep score (minions killed)
} StatsDisplay;

typedef struct StatsPanel {
    StatsDisplay stats;
    bool is_expanded;           // Compact vs full view
    bool show_tooltips;         // Explain stat calculations
} StatsPanel;
```

**Compact View (Default):**

```
┌─────────────────┐
│ ⚔ 85   🛡 45   │  <- AD, Armor
│ ✦ 120  🔮 32   │  <- AP, MR
│ ⚡ 1.2  👟 380  │  <- AS, MS
│ 💰 1,234       │  <- Gold
└─────────────────┘
```

**Expanded View (Hover):**

Shows all stats with labels and source breakdown (base + items + runes).

---

## 4. Top Bar

The top bar provides game-wide information visible at a glance.

### 4.1 Game Timer

```c
typedef struct GameTimer {
    float elapsed_seconds;      // Total game time
    bool is_paused;             // Pause state (if applicable)
    uint32_t display_format;    // MM:SS or HH:MM:SS
} GameTimer;

// Format: "12:34" (minutes:seconds)
// After 60 minutes: "1:12:34"
void game_timer_format(float seconds, char* buffer, size_t buffer_size);
```

**Visual Specifications:**

| Property | Value |
|----------|-------|
| Position | Top center |
| Font | 24px bold, white |
| Background | Semi-transparent black |
| Size | 80x32 pixels |
| Update Rate | Every second |

### 4.2 Team Scores

```c
typedef struct TeamScores {
    uint16_t blue_kills;        // Blue team total kills
    uint16_t red_kills;         // Red team total kills
    uint32_t blue_gold;         // Blue team total gold
    uint32_t red_gold;          // Red team total gold
    uint8_t blue_towers;        // Blue towers destroyed
    uint8_t red_towers;         // Red towers destroyed
} TeamScores;
```

**Visual Layout:**

```
      ┌─────────────────────────┐
      │  🔵 5   vs   3 🔴      │
      │     [12.4k]   [10.2k]  │  <- Optional gold display
      └─────────────────────────┘
```

| Property | Value |
|----------|-------|
| Font | 28px bold |
| Blue Color | #3399FF |
| Red Color | #FF3333 |
| Spacing | 20px between numbers |
| Update | Instant on kill |

### 4.3 Objective Timers

```c
typedef enum ObjectiveType {
    OBJECTIVE_DRAGON,
    OBJECTIVE_RIFT_HERALD,
    OBJECTIVE_BARON,
    OBJECTIVE_ELDER_DRAGON,
} ObjectiveType;

typedef struct ObjectiveTimer {
    ObjectiveType type;
    float respawn_time;         // Seconds until spawn
    bool is_alive;              // Currently spawned
    bool is_contested;          // Being fought
    uint8_t team_control;       // Which team has vision
} ObjectiveTimer;

typedef struct ObjectiveTimerBar {
    ObjectiveTimer dragon;
    ObjectiveTimer baron;
    ObjectiveTimer herald;
    bool show_all;              // Expand to show all
} ObjectiveTimerBar;
```

**Visual Layout:**

```
┌────────────────────────────────────┐
│ 🐉 2:30          🏰 Baron 5:00    │
└────────────────────────────────────┘
```

**Timer States:**

| State | Display | Animation |
|-------|---------|-----------|
| Dead (>30s) | "Dragon 2:30" | Static |
| Spawning (<30s) | "Dragon 0:15" | Pulsing |
| Alive | Dragon icon only | Glow |
| Contested | "CONTEST!" | Flashing |

---

## 5. Minimap

The minimap provides strategic awareness of the entire battlefield.

### 5.1 Core Structure

```c
typedef struct Minimap {
    // Position and size
    Vec2 screen_position;       // Bottom-right anchor
    Vec2 size;                  // Default: 220x220 pixels
    float zoom_level;           // 1.0 = full map, 2.0 = zoomed

    // Map data
    uint32_t terrain_texture;   // Simplified map texture
    float world_size;           // World units for full map
    float map_scale;            // World to minimap scale

    // Display options
    bool show_champions;        // Show champion icons
    bool show_minions;          // Show minion dots
    bool show_wards;            // Show ward positions
    bool show_pings;            // Show ping markers
    bool show_fog_of_war;       // Respect fog
    bool mirror_minimap;        // Flip for red side

    // Interaction
    bool allow_click_move;      // Right-click to move
    bool allow_click_ping;      // G+click to ping
    Vec2 camera_rect_min;       // Camera viewport bounds
    Vec2 camera_rect_max;
} Minimap;
```

### 5.2 Minimap Icons

```c
typedef enum MinimapIconType {
    MINIMAP_ICON_ALLY_CHAMPION,     // Blue circle with portrait
    MINIMAP_ICON_ENEMY_CHAMPION,    // Red circle with portrait
    MINIMAP_ICON_ALLY_MINION,       // Small blue dot
    MINIMAP_ICON_ENEMY_MINION,      // Small red dot
    MINIMAP_ICON_ALLY_TOWER,        // Blue square
    MINIMAP_ICON_ENEMY_TOWER,       // Red square
    MINIMAP_ICON_ALLY_WARD,         // Blue eye
    MINIMAP_ICON_ENEMY_WARD,        // Red eye (revealed)
    MINIMAP_ICON_DRAGON,            // Dragon icon
    MINIMAP_ICON_BARON,             // Baron icon
    MINIMAP_ICON_PING,              // Ping marker
} MinimapIconType;

typedef struct MinimapIcon {
    MinimapIconType type;
    Vec2 world_position;        // World coordinates
    Vec2 minimap_position;      // Calculated screen position
    uint32_t texture_id;        // Icon texture
    float scale;                // Icon size multiplier
    float rotation;             // For directional icons
    bool visible;               // Fog of war check
} MinimapIcon;
```

**Icon Sizes:**

| Icon Type | Size (pixels) |
|-----------|---------------|
| Champion | 16x16 |
| Minion | 4x4 |
| Tower | 10x10 |
| Ward | 6x6 |
| Objective | 14x14 |
| Ping | 20x20 |

### 5.3 Camera Viewport

```c
typedef struct MinimapCamera {
    Vec2 world_min;             // Camera view bounds (world)
    Vec2 world_max;
    uint32_t border_color;      // White rectangle
    float border_width;         // 1-2 pixels
} MinimapCamera;

// Convert world position to minimap position
Vec2 world_to_minimap(const Minimap* map, Vec3 world_pos);

// Convert minimap click to world position
Vec3 minimap_to_world(const Minimap* map, Vec2 minimap_pos);
```

### 5.4 Minimap Rendering

```c
void minimap_render(const Minimap* map, Renderer* renderer) {
    // 1. Render terrain texture (with fog of war mask)
    render_minimap_terrain(map, renderer);

    // 2. Render camera viewport rectangle
    render_minimap_camera(map, renderer);

    // 3. Batch render all icons
    for (uint32_t i = 0; i < icon_count; i++) {
        if (minimap_icons[i].visible) {
            render_minimap_icon(&minimap_icons[i], renderer);
        }
    }

    // 4. Render active pings (with fade)
    render_minimap_pings(map, renderer);

    // 5. Render border
    render_minimap_border(map, renderer);
}
```

---

## 6. Team Frames

Team frames display allied champion status on the left side of the screen.

### 6.1 Team Frame Structure

```c
typedef struct TeamFrame {
    // Identity
    uint32_t player_id;
    uint32_t champion_id;
    char summoner_name[32];
    uint32_t portrait_texture;

    // Status
    float health_current;
    float health_max;
    float mana_current;
    float mana_max;
    uint8_t level;

    // Ultimate
    bool ultimate_ready;
    float ultimate_cooldown;
    float ultimate_cooldown_max;

    // State
    bool is_alive;
    float respawn_timer;
    bool is_disconnected;

    // Summoner spells
    uint32_t summoner1_id;
    uint32_t summoner2_id;
    float summoner1_cooldown;
    float summoner2_cooldown;
} TeamFrame;

typedef struct TeamFramePanel {
    TeamFrame frames[4];        // 4 allies (excluding self)
    uint32_t frame_count;
    bool collapsed;             // Minimize display
    bool show_summoner_spells;
} TeamFramePanel;
```

### 6.2 Visual Layout

```
┌──────────────────────┐
│ ┌────┐ ████████ Lv8  │  <- Portrait, HP bar, Level
│ │ 📷 │ ██████── 45s  │  <- MP bar, Ult CD
│ └────┘ [D][F]        │  <- Summoner spells
├──────────────────────┤
│ ┌────┐ ██████── Lv10 │
│ │ 📷 │ ████████ ✓    │  <- Ult ready checkmark
│ └────┘ [D][F]        │
├──────────────────────┤
│ ┌────┐ ░░░░░░░░ DEAD │  <- Dead state
│ │ 💀 │ Respawn: 25s  │
│ └────┘               │
└──────────────────────┘
```

**Visual Specifications:**

| Property | Value |
|----------|-------|
| Frame Width | 96 pixels |
| Frame Height | 80 pixels per ally |
| Portrait Size | 32x32 pixels |
| HP Bar | 56x8 pixels |
| MP Bar | 56x4 pixels |
| Ult Icon | 16x16 pixels |

### 6.3 Interaction

```c
typedef enum TeamFrameAction {
    TF_ACTION_NONE,
    TF_ACTION_CENTER_CAMERA,    // Left click
    TF_ACTION_PING_ASSIST,      // Alt+click
    TF_ACTION_TARGET_ALLY,      // For ally-targeted abilities
} TeamFrameAction;

TeamFrameAction team_frame_handle_click(
    TeamFramePanel* panel,
    Vec2 click_position,
    uint32_t modifiers
);
```

---

## 7. World-Space UI

World-space UI elements exist in 3D space and are projected to screen coordinates.

### 7.1 Health Bars

```c
typedef struct WorldHealthBar {
    EntityId entity;            // Associated entity
    uint8_t entity_type;        // Champion, minion, tower, etc.
    uint8_t team;               // For color determination

    // Health data
    float health_current;
    float health_max;
    float shield_amount;        // Shield overlay

    // Position
    Vec3 world_position;        // Feet position
    float height_offset;        // Above unit
    Vec2 screen_position;       // Calculated

    // Display
    float bar_width;            // Based on unit type
    bool show_level;            // For champions
    uint8_t level;              // Champion level
    bool visible;               // Fog/range check
} WorldHealthBar;
```

**Health Bar Sizes by Unit Type:**

| Unit Type | Width | Height | Height Offset |
|-----------|-------|--------|---------------|
| Champion | 100px | 10px | 120 units |
| Minion | 40px | 4px | 50 units |
| Tower | 120px | 12px | 200 units |
| Dragon/Baron | 150px | 14px | 180 units |
| Ward | 30px | 3px | 30 units |

**Health Bar Colors:**

```c
#define HEALTHBAR_ALLY      0xFF00CC00  // Green
#define HEALTHBAR_ENEMY     0xFFCC0000  // Red
#define HEALTHBAR_NEUTRAL   0xFFCCCC00  // Yellow
#define HEALTHBAR_SHIELD    0xFFFFFFFF  // White overlay
#define HEALTHBAR_DAMAGE    0xFF880000  // Dark red (recent damage)
```

### 7.2 Floating Combat Text

```c
typedef enum FloatingTextType {
    FLOAT_TEXT_DAMAGE_PHYSICAL,
    FLOAT_TEXT_DAMAGE_MAGIC,
    FLOAT_TEXT_DAMAGE_TRUE,
    FLOAT_TEXT_DAMAGE_CRIT,
    FLOAT_TEXT_HEAL,
    FLOAT_TEXT_SHIELD,
    FLOAT_TEXT_GOLD,
    FLOAT_TEXT_XP,
    FLOAT_TEXT_MISS,
    FLOAT_TEXT_STATUS,          // "STUNNED", "SLOWED"
} FloatingTextType;

typedef struct FloatingText {
    char text[32];              // "523", "CRIT!", "+50g"
    FloatingTextType type;

    // Position
    Vec3 world_origin;          // Starting position
    Vec2 screen_position;       // Current screen position
    Vec2 velocity;              // Screen-space drift

    // Animation
    float lifetime;             // Total duration
    float elapsed;              // Time since spawn
    float scale;                // Size multiplier
    float alpha;                // Fade out

    // Style
    uint32_t color;
    bool is_crit;               // Larger, different style
} FloatingText;

typedef struct FloatingTextPool {
    FloatingText* texts;
    uint32_t capacity;
    uint32_t count;
    uint32_t next_free;         // Free list head
} FloatingTextPool;
```

**Floating Text Animation:**

```c
void floating_text_update(FloatingText* text, float dt) {
    text->elapsed += dt;

    // Progress (0 to 1)
    float t = text->elapsed / text->lifetime;

    // Drift upward and slightly random horizontal
    text->screen_position.y -= 60.0f * dt;  // 60 pixels per second up
    text->screen_position.x += text->velocity.x * dt;

    // Scale: Start big, shrink slightly
    text->scale = text->is_crit ? 1.5f - 0.3f * t : 1.0f - 0.2f * t;

    // Fade out in last 30%
    if (t > 0.7f) {
        text->alpha = 1.0f - (t - 0.7f) / 0.3f;
    }
}

// Default durations
#define FLOAT_TEXT_DURATION     1.5f    // Seconds
#define FLOAT_TEXT_CRIT_DURATION 2.0f   // Crits last longer
```

**Text Colors:**

| Type | Color | Style |
|------|-------|-------|
| Physical Damage | White | Normal |
| Magic Damage | Purple (#CC66FF) | Normal |
| True Damage | White | Bold |
| Critical | Orange (#FF8800) | Large, exclamation |
| Healing | Green (#00FF00) | + prefix |
| Shield | White | Shield icon |
| Gold | Gold (#FFD700) | +g suffix |

### 7.3 Ability Indicators

```c
typedef enum IndicatorType {
    INDICATOR_NONE,
    INDICATOR_RANGE_CIRCLE,     // Show ability range
    INDICATOR_SKILLSHOT_LINE,   // Line/rectangle skillshot
    INDICATOR_SKILLSHOT_ARC,    // Curved skillshot
    INDICATOR_AOE_CIRCLE,       // Area of effect
    INDICATOR_CONE,             // Cone ability
    INDICATOR_VECTOR,           // Point + direction
} IndicatorType;

typedef struct AbilityIndicator {
    uint32_t ability_id;
    IndicatorType type;

    // Origin (usually caster position)
    Vec3 origin;
    Vec3 target;                // Mouse world position

    // Geometry
    float range;                // Max range
    float width;                // Skillshot width
    float length;               // Skillshot length
    float angle;                // Cone angle (radians)
    float radius;               // AOE radius

    // Display
    bool is_valid_target;       // In range?
    bool show_range_circle;     // Always show range
    uint32_t valid_color;       // Green when valid
    uint32_t invalid_color;     // Red when invalid
    float alpha;                // Transparency
} AbilityIndicator;
```

**Indicator Types Visualization:**

```
Range Circle:
    ┌───────────────┐
    │     ╱ ╲       │
    │   ╱     ╲     │
    │  │   ●   │    │  ● = Champion
    │   ╲     ╱     │  Circle shows max range
    │     ╲ ╱       │
    └───────────────┘

Skillshot Line:
    ┌───────────────┐
    │               │
    │   ● ═══════►  │  Arrow shows direction
    │               │  Width shows hitbox
    └───────────────┘

AOE Circle:
    ┌───────────────┐
    │       ◯       │
    │     ╱   ╲     │  ◯ = Target location
    │   ●        ╲  │  Circle shows AOE
    │     ╲   ╱     │
    │       ◯       │
    └───────────────┘

Cone:
    ┌───────────────┐
    │      ╲   ╱    │
    │       ╲ ╱     │  Lines show cone edges
    │        ●      │  Filled area = hit zone
    └───────────────┘
```

**Indicator Rendering:**

```c
void ability_indicator_render(
    const AbilityIndicator* indicator,
    Renderer* renderer
) {
    uint32_t color = indicator->is_valid_target
        ? indicator->valid_color
        : indicator->invalid_color;

    switch (indicator->type) {
        case INDICATOR_RANGE_CIRCLE:
            render_circle_outline(
                world_to_screen(indicator->origin),
                indicator->range * world_scale,
                color,
                2.0f  // Line width
            );
            break;

        case INDICATOR_SKILLSHOT_LINE:
            render_skillshot_rectangle(
                indicator->origin,
                indicator->target,
                indicator->width,
                indicator->range,
                color
            );
            break;

        case INDICATOR_AOE_CIRCLE:
            render_circle_filled(
                world_to_screen(indicator->target),
                indicator->radius * world_scale,
                color,
                0.3f  // Alpha
            );
            break;

        case INDICATOR_CONE:
            render_cone(
                indicator->origin,
                indicator->target,
                indicator->angle,
                indicator->range,
                color
            );
            break;
    }
}

---

## 8. Scoreboard

The scoreboard displays comprehensive game statistics for all players.

### 8.1 Scoreboard Structure

```c
typedef struct ScoreboardEntry {
    // Identity
    uint8_t team;               // 0 = blue, 1 = red
    uint8_t slot;               // Player slot (0-4)
    char player_name[32];
    uint32_t champion_id;
    uint32_t portrait_texture;

    // Stats
    uint16_t kills;
    uint16_t deaths;
    uint16_t assists;
    uint32_t cs;                // Creep score
    uint32_t gold;              // Total gold earned
    uint32_t damage_dealt;      // To champions
    uint32_t damage_taken;

    // Inventory
    uint32_t items[6];          // Item IDs
    uint32_t trinket;           // Ward/sweeper

    // State
    uint8_t level;
    bool is_alive;
    float respawn_timer;
    bool is_disconnected;

    // Summoner spells
    uint32_t summoner1_id;
    uint32_t summoner2_id;
    float summoner1_cooldown;
    float summoner2_cooldown;
} ScoreboardEntry;

typedef struct Scoreboard {
    ScoreboardEntry entries[10];    // All 10 players
    uint8_t local_player_slot;      // Highlight self
    bool is_visible;

    // Display options
    bool show_gold;
    bool show_cs;
    bool show_damage;
    bool show_summoner_cooldowns;
} Scoreboard;
```

### 8.2 Visual Layout

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              SCOREBOARD                                      │
├──────────────────────────────────────────────────────────────────────────────┤
│ BLUE TEAM                                                                    │
├────┬──────────────┬─────────┬──────┬───────┬──────────────────────────┬─────┤
│ 📷 │ PlayerName   │  K/D/A  │  CS  │ Gold  │ [Item][Item][Item]...    │ Lv  │
├────┼──────────────┼─────────┼──────┼───────┼──────────────────────────┼─────┤
│ 🧙 │ MidLaner123  │  5/2/8  │ 156  │ 8.4k  │ [💎][⚔][🛡][📿][🥾][📖]│  12 │
│ ⭐ │ TopPlayer    │  3/3/4  │ 124  │ 6.2k  │ [⚔][🛡][🥾][  ][  ][  ]│  11 │
│ ... │             │         │      │       │                          │     │
├──────────────────────────────────────────────────────────────────────────────┤
│ RED TEAM                                                                     │
├────┬──────────────┬─────────┬──────┬───────┬──────────────────────────┬─────┤
│ 🏹 │ EnemyADC     │  4/1/6  │ 178  │ 9.1k  │ [💎][⚔][🥾][📿][🗡][  ]│  13 │
│ ... │             │         │      │       │                          │     │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 8.3 Interaction

```c
typedef enum ScoreboardAction {
    SB_ACTION_NONE,
    SB_ACTION_MUTE_PLAYER,      // Right-click player
    SB_ACTION_VIEW_PROFILE,     // Left-click player name
    SB_ACTION_REPORT_PLAYER,    // Context menu
    SB_ACTION_HONOR_PLAYER,     // Post-game only
} ScoreboardAction;

// Toggle scoreboard
void scoreboard_toggle(Scoreboard* sb) {
    sb->is_visible = !sb->is_visible;
}

// Tab hold behavior
void scoreboard_set_visible(Scoreboard* sb, bool visible) {
    sb->is_visible = visible;
}
```

**Scoreboard Controls:**

| Action | Control |
|--------|---------|
| Show/Hide | Hold Tab |
| Toggle Lock | Tap Tab |
| Mute Player | Right-click portrait |
| Expand Stats | Mouse hover |

---

## 9. Shop UI

The shop allows players to purchase items during the game.

### 9.1 Shop Structure

```c
typedef enum ItemCategory {
    ITEM_CAT_ALL,
    ITEM_CAT_RECOMMENDED,
    ITEM_CAT_STARTER,
    ITEM_CAT_BOOTS,
    ITEM_CAT_BASIC,
    ITEM_CAT_EPIC,
    ITEM_CAT_LEGENDARY,
    ITEM_CAT_DAMAGE,
    ITEM_CAT_ATTACK_SPEED,
    ITEM_CAT_CRIT,
    ITEM_CAT_LIFESTEAL,
    ITEM_CAT_ARMOR,
    ITEM_CAT_MAGIC_RESIST,
    ITEM_CAT_HEALTH,
    ITEM_CAT_MANA,
    ITEM_CAT_ABILITY_POWER,
    ITEM_CAT_COUNT,
} ItemCategory;

typedef struct ShopItem {
    uint32_t item_id;
    const char* name;
    const char* description;
    uint32_t icon_texture;
    uint32_t gold_cost;
    uint32_t sell_value;
    uint32_t recipe_items[4];   // Component item IDs
    uint32_t recipe_cost;       // Combine cost
    ItemCategory categories[4]; // Item categories
    bool is_purchasable;        // Meets requirements
    bool can_afford;            // Has gold
} ShopItem;

typedef struct ShopPanel {
    bool is_open;

    // Filtering
    char search_query[64];
    ItemCategory active_category;

    // Selection
    uint32_t selected_item_id;
    uint32_t hovered_item_id;

    // Recommendations
    uint32_t recommended_items[12];
    uint32_t recommended_count;

    // Build path
    uint32_t build_path_items[6];
    uint32_t build_path_count;

    // Purchase state
    uint32_t current_gold;
    float undo_timer;           // Time left to undo
    uint32_t last_purchased;    // For undo
} ShopPanel;
```

### 9.2 Visual Layout

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ [X]  ITEM SHOP                                         Gold: 💰 2,345       │
├────────────────┬─────────────────────────────────────────────────────────────┤
│ CATEGORIES     │  🔍 [Search...                                    ]        │
│ ────────────── │  ────────────────────────────────────────────────────────  │
│ > Recommended  │  RECOMMENDED FOR: Warrior                                  │
│   All Items    │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                │
│   ────────────│  │ ⚔ │ │ 🛡 │ │ 🥾 │ │ 💎 │ │ 📿 │ │ 🗡 │                │
│   Damage       │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                │
│   Defense      │                                                            │
│   Magic        │  ALL ITEMS                                                 │
│   Movement     │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
│   Consumables  │  │    │ │    │ │    │ │    │ │    │ │    │ │    │ │    │ │
│                │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │
│                │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │
│                │  │    │ │    │ │    │ │    │ │    │ │    │ │    │ │    │ │
│                │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │
├────────────────┴─────────────────────────────────────────────────────────────┤
│ SELECTED ITEM: Blade of the Ruined King                                     │
│ ┌────┐ ────────────────────────────────────────────────────────────────────│
│ │ ⚔ │ +40 Attack Damage  |  +25% Attack Speed  |  +12% Life Steal         │
│ └────┘                                                                       │
│ UNIQUE Passive: Deal 12% current health physical damage on-hit.            │
│                                                                              │
│ BUILD PATH:  [Pickaxe] + [Recurve Bow] + [Vampiric Scepter] + 425g         │
│              ┌────┐     ┌────┐          ┌────┐                              │
│              │ ✓ │     │ ✗ │          │ ✗ │                              │
│              └────┘     └────┘          └────┘                              │
│                                                                              │
│ TOTAL COST: 3200g                              [BUY] [UNDO]                 │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 Shop Controls

```c
typedef enum ShopAction {
    SHOP_ACTION_NONE,
    SHOP_ACTION_OPEN,           // P key or click
    SHOP_ACTION_CLOSE,          // Escape or P
    SHOP_ACTION_BUY,            // Double-click or Enter
    SHOP_ACTION_SELL,           // Right-click owned item
    SHOP_ACTION_UNDO,           // Ctrl+Z or button
    SHOP_ACTION_SEARCH,         // Focus search box
} ShopAction;

// Shop interaction
void shop_purchase_item(ShopPanel* shop, uint32_t item_id);
void shop_sell_item(ShopPanel* shop, uint8_t slot);
void shop_undo_purchase(ShopPanel* shop);
void shop_search(ShopPanel* shop, const char* query);
void shop_set_category(ShopPanel* shop, ItemCategory category);
```

**Undo System:**

```c
#define SHOP_UNDO_WINDOW    10.0f   // 10 seconds to undo

typedef struct UndoPurchase {
    uint32_t item_id;
    uint32_t gold_spent;
    float time_remaining;
    bool is_valid;              // Invalidate if item used
} UndoPurchase;

void shop_check_undo_validity(ShopPanel* shop);
```

---

## 10. Ping System

The ping system enables quick non-verbal communication.

### 10.1 Ping Types

```c
typedef enum PingType {
    PING_GENERIC,               // Yellow circle - "Look here"
    PING_DANGER,                // Red ! - "Danger"
    PING_ASSIST,                // Blue flag - "Help me"
    PING_ON_MY_WAY,             // Green arrow - "Coming"
    PING_ENEMY_MISSING,         // Yellow ? - "MIA"
    PING_ENEMY_VISION,          // Red eye - "Enemy has vision"
    PING_NEED_VISION,           // Blue eye - "Need ward"
    PING_PUSH,                  // Blue sword - "Push lane"
    PING_ALL_IN,                // Red target - "All in"
    PING_HOLD,                  // Yellow hand - "Wait"
    PING_BAIT,                  // Purple hook - "Bait them"
} PingType;

typedef struct Ping {
    PingType type;
    uint32_t sender_id;         // Player who pinged
    Vec3 world_position;        // Where ping was placed
    EntityId target_entity;     // If pinged on entity
    float lifetime;             // Fade out timer
    float elapsed;
    bool visible;
} Ping;
```

### 10.2 Ping Wheel

```c
typedef struct PingWheel {
    bool is_open;
    Vec2 center_screen;         // Where wheel opened
    Vec3 world_target;          // World position to ping

    PingType segments[8];       // Up to 8 options
    uint8_t segment_count;
    int8_t hovered_segment;     // -1 if none
    int8_t selected_segment;    // Locked selection

    float open_time;            // For animation
} PingWheel;
```

**Ping Wheel Layout:**

```
           [Danger]
              ▲
              │
   [Missing] ◄─┼─► [On My Way]
              │
              ▼
           [Assist]
```

### 10.3 Ping Controls

```c
// Quick ping (G + click)
void ping_quick(Vec3 world_position, PingType type);

// Wheel ping (G + hold + drag)
void ping_wheel_open(Vec2 screen_position, Vec3 world_target);
void ping_wheel_update(PingWheel* wheel, Vec2 mouse_position);
void ping_wheel_close(PingWheel* wheel);

// Smart ping (Alt + click entity)
void ping_entity(EntityId entity, PingType type);
```

### 10.4 Ping Visuals

```c
typedef struct PingVisual {
    // World-space
    Vec3 world_position;
    float world_scale;          // Pulsing animation

    // Minimap
    Vec2 minimap_position;
    float minimap_scale;

    // Animation
    float pulse_phase;
    float alpha;

    // Style
    uint32_t color;
    uint32_t icon_texture;
} PingVisual;

// Ping animation constants
#define PING_DURATION           4.0f    // Seconds visible
#define PING_PULSE_SPEED        3.0f    // Hz
#define PING_FADE_START         3.0f    // Start fading at 3s
```

### 10.5 Ping Rate Limiting

```c
typedef struct PingLimiter {
    float cooldown_remaining;
    uint8_t pings_available;    // Charges
    uint8_t max_pings;          // Usually 5
    float recharge_time;        // Seconds per charge
    bool is_muted;              // Exceeded limit
    float mute_timer;           // Unmute countdown
} PingLimiter;

#define PING_COOLDOWN           0.5f    // Between pings
#define PING_MAX_BURST          5       // Max rapid pings
#define PING_RECHARGE_TIME      2.0f    // Per charge
#define PING_MUTE_DURATION      10.0f   // If spamming
```

---

## 11. Settings and Customization

### 11.1 HUD Settings

```c
typedef struct HUDSettings {
    // Scale
    float global_scale;         // 0.75 - 1.5
    float minimap_scale;        // 0.8 - 1.2
    float health_bar_scale;     // 0.5 - 1.5

    // Positions
    bool minimap_left;          // Mirror minimap position
    bool flip_minimap;          // Flip for red side

    // Information display
    bool show_champion_names;
    bool show_summoner_names;
    bool show_timestamps;       // In chat
    bool show_ability_ranges;   // Always show

    // Health bars
    bool show_ally_health_bars;
    bool show_enemy_health_bars;
    bool show_minion_health_bars;

    // Combat text
    bool show_damage_numbers;
    bool show_healing_numbers;
    bool show_gold_earned;

    // Accessibility
    uint8_t colorblind_mode;    // 0=off, 1=deuteranopia, 2=protanopia, 3=tritanopia
    bool high_contrast_mode;
    float text_scale;           // 0.8 - 1.5
} HUDSettings;
```

### 11.2 Colorblind Modes

```c
typedef enum ColorblindMode {
    COLORBLIND_OFF,
    COLORBLIND_DEUTERANOPIA,    // Red-green (most common)
    COLORBLIND_PROTANOPIA,      // Red-blind
    COLORBLIND_TRITANOPIA,      // Blue-yellow
} ColorblindMode;

typedef struct ColorPalette {
    uint32_t ally_color;
    uint32_t enemy_color;
    uint32_t neutral_color;
    uint32_t health_color;
    uint32_t mana_color;
    uint32_t danger_color;
    uint32_t safe_color;
} ColorPalette;

// Get appropriate colors for colorblind mode
ColorPalette get_colorblind_palette(ColorblindMode mode);
```

**Color Palettes:**

| Element | Default | Deuteranopia | Protanopia | Tritanopia |
|---------|---------|--------------|------------|------------|
| Ally | Blue #3399FF | Blue #3399FF | Blue #3399FF | Blue #0077BB |
| Enemy | Red #FF3333 | Orange #FF8800 | Yellow #FFCC00 | Red #CC3311 |
| Health | Green #00CC00 | Cyan #00CCCC | Cyan #00CCCC | Green #009988 |
| Danger | Red #FF0000 | Orange #FF6600 | Yellow #FFFF00 | Magenta #EE3377 |

---

## 12. Code Architecture

### 12.1 Master HUD Structure

```c
typedef struct GameHUD {
    // Screen-space panels
    ChampionPanel champion_panel;
    AbilityBar ability_bar;
    ItemBar item_bar;
    StatsPanel stats_panel;
    Minimap minimap;
    TeamFramePanel team_frames;
    TopBar top_bar;
    ChatPanel chat;

    // World-space elements
    WorldHealthBar* health_bars;
    uint32_t health_bar_count;
    uint32_t health_bar_capacity;

    FloatingTextPool floating_text;

    AbilityIndicator ability_indicator;
    bool show_ability_indicator;

    // Overlay panels
    Scoreboard scoreboard;
    ShopPanel shop;
    SettingsPanel settings;
    PingWheel ping_wheel;

    // State
    HUDSettings user_settings;
    bool is_paused;
    bool is_dead;
    bool in_shop_range;

    // Resources
    uint32_t icon_atlas;        // Texture atlas for icons
    uint32_t font_texture;      // Font atlas
    Arena* ui_arena;            // Memory for UI elements

    // Input state
    Vec2 mouse_position;
    bool mouse_over_ui;         // Block game input
} GameHUD;
```

### 12.2 Core API

```c
// Lifecycle
GameHUD* hud_create(Arena* arena, HUDSettings settings);
void hud_destroy(GameHUD* hud);

// Frame update
void hud_update(GameHUD* hud, const GameState* game, float dt);
void hud_render(GameHUD* hud, Renderer* renderer);

// Input handling
bool hud_handle_input(GameHUD* hud, const InputEvent* event);
bool hud_wants_input(const GameHUD* hud);  // UI consuming input?

// Subsystem access
void hud_show_shop(GameHUD* hud, bool show);
void hud_show_scoreboard(GameHUD* hud, bool show);
void hud_open_ping_wheel(GameHUD* hud, Vec2 position);

// Settings
void hud_apply_settings(GameHUD* hud, const HUDSettings* settings);
void hud_set_colorblind_mode(GameHUD* hud, ColorblindMode mode);
void hud_set_scale(GameHUD* hud, float scale);

// World-space management
void hud_add_floating_text(GameHUD* hud, const FloatingText* text);
void hud_update_health_bar(GameHUD* hud, EntityId entity, float health, float max_health);
void hud_show_ability_indicator(GameHUD* hud, const AbilityIndicator* indicator);
void hud_hide_ability_indicator(GameHUD* hud);
```

### 12.3 File Structure

```
src/client/ui/
├── hud/
│   ├── hud.h                   # Main HUD API
│   ├── hud.c                   # HUD lifecycle and coordination
│   ├── hud_render.c            # Rendering logic
│   ├── hud_input.c             # Input handling
│   ├── hud_layout.c            # Positioning and scaling
│   │
│   ├── champion_panel.h/c      # Portrait, HP/MP, XP
│   ├── ability_bar.h/c         # QWER + passive
│   ├── item_bar.h/c            # 6 item slots
│   ├── stats_panel.h/c         # Stats display
│   │
│   ├── minimap.h/c             # Minimap rendering
│   ├── team_frames.h/c         # Allied champion frames
│   ├── top_bar.h/c             # Timer, scores, objectives
│   │
│   ├── world_ui/
│   │   ├── health_bars.h/c     # World-space health bars
│   │   ├── floating_text.h/c   # Damage numbers
│   │   └── indicators.h/c      # Ability indicators
│   │
│   ├── scoreboard.h/c          # Tab scoreboard
│   ├── shop.h/c                # Item shop
│   └── ping_wheel.h/c          # Ping system
│
├── widgets/
│   ├── button.h/c
│   ├── progress_bar.h/c
│   ├── icon.h/c
│   ├── tooltip.h/c
│   ├── text.h/c
│   └── scroll_panel.h/c
│
└── ui_common.h                 # Shared types and utilities
```

---

## 13. Rendering Pipeline

### 13.1 Render Order

```c
void hud_render(GameHUD* hud, Renderer* renderer) {
    // ═══════════════════════════════════════════════════════════
    // PHASE 1: World-Space UI (rendered during 3D pass)
    // ═══════════════════════════════════════════════════════════

    // 1.1 Ability indicators (on ground)
    if (hud->show_ability_indicator) {
        render_ability_indicator(&hud->ability_indicator, renderer);
    }

    // 1.2 World health bars (billboarded)
    render_world_health_bars(hud->health_bars, hud->health_bar_count, renderer);

    // 1.3 Floating combat text
    render_floating_text(&hud->floating_text, renderer);

    // ═══════════════════════════════════════════════════════════
    // PHASE 2: Screen-Space UI (2D overlay)
    // ═══════════════════════════════════════════════════════════

    renderer_begin_ui(renderer);  // Switch to orthographic

    // 2.1 Background panels
    render_hud_backgrounds(hud, renderer);

    // 2.2 Core HUD elements
    render_minimap(&hud->minimap, renderer);
    render_team_frames(&hud->team_frames, renderer);
    render_top_bar(&hud->top_bar, renderer);
    render_champion_panel(&hud->champion_panel, renderer);
    render_ability_bar(&hud->ability_bar, renderer);
    render_item_bar(&hud->item_bar, renderer);
    render_stats_panel(&hud->stats_panel, renderer);
    render_chat(&hud->chat, renderer);

    // 2.3 Ping markers (on minimap and world)
    render_pings(hud, renderer);

    // ═══════════════════════════════════════════════════════════
    // PHASE 3: Popup Panels (top layer)
    // ═══════════════════════════════════════════════════════════

    if (hud->scoreboard.is_visible) {
        render_scoreboard(&hud->scoreboard, renderer);
    }

    if (hud->shop.is_open) {
        render_shop(&hud->shop, renderer);
    }

    if (hud->ping_wheel.is_open) {
        render_ping_wheel(&hud->ping_wheel, renderer);
    }

    // 3.1 Tooltips (always on top)
    render_active_tooltip(hud, renderer);

    renderer_end_ui(renderer);
}
```

### 13.2 Batching Strategy

```c
typedef struct UIBatch {
    uint32_t texture_id;        // Current texture (atlas)
    UIVertex* vertices;
    uint32_t vertex_count;
    uint32_t vertex_capacity;
    uint16_t* indices;
    uint32_t index_count;
    uint32_t index_capacity;
} UIBatch;

typedef struct UIRenderer {
    UIBatch batches[8];         // Multiple texture batches
    uint32_t batch_count;

    uint32_t icon_atlas;        // Ability/item icons
    uint32_t font_atlas;        // Text rendering
    uint32_t ui_atlas;          // Borders, backgrounds

    VkPipeline ui_pipeline;
    VkDescriptorSet descriptor_set;
} UIRenderer;

// Batch all UI draw calls
void ui_batch_quad(UIRenderer* ui, Vec2 pos, Vec2 size, Vec4 uv, uint32_t color);
void ui_batch_text(UIRenderer* ui, const char* text, Vec2 pos, float size, uint32_t color);
void ui_batch_icon(UIRenderer* ui, uint32_t icon_id, Vec2 pos, Vec2 size);

// Flush all batches at end of frame
void ui_render_batches(UIRenderer* ui, Renderer* renderer);
```

### 13.3 Texture Atlases

```c
typedef struct IconAtlas {
    uint32_t texture_id;
    uint32_t width;
    uint32_t height;
    uint32_t icon_size;         // 64x64 for abilities
    uint32_t icons_per_row;

    // Lookup
    Vec4* icon_uvs;             // Pre-calculated UVs
    uint32_t icon_count;
} IconAtlas;

// Atlas organization (2048x2048 texture)
// ┌──────────────────────────────────┐
// │ ABILITIES (0-127)                │  512 icons, 64x64
// │ 32x4 = 128 per section           │
// ├──────────────────────────────────┤
// │ ITEMS (128-383)                  │  256 icons, 48x48
// │                                  │
// ├──────────────────────────────────┤
// │ CHAMPIONS (384-447)              │  64 portraits, 64x64
// ├──────────────────────────────────┤
// │ UI ELEMENTS (448-511)            │  Borders, backgrounds
// └──────────────────────────────────┘

Vec4 icon_atlas_get_uv(const IconAtlas* atlas, uint32_t icon_id);
```

---

## 14. Performance Considerations

### 14.1 Optimization Strategies

```c
// 1. Batch draw calls
#define MAX_UI_DRAW_CALLS       50      // Target
#define MAX_UI_VERTICES         16384   // Per batch

// 2. Update frequencies
#define HUD_UPDATE_HZ           60      // Full update rate
#define MINIMAP_UPDATE_HZ       10      // Minimap can be slower
#define HEALTHBAR_CULL_DIST     2000.0f // World units

// 3. Object pooling
typedef struct FloatingTextPool {
    FloatingText texts[64];     // Pre-allocated
    uint32_t active_mask[2];    // Bitfield for active texts
    uint32_t next_slot;
} FloatingTextPool;

// 4. Dirty flags
typedef struct HUDDirtyFlags {
    bool champion_panel;
    bool ability_bar;
    bool item_bar;
    bool minimap;
    bool team_frames;
    bool top_bar;
} HUDDirtyFlags;
```

### 14.2 Memory Budget

| System | Budget | Notes |
|--------|--------|-------|
| Icon Atlas | 16 MB | 2048x2048 RGBA |
| Font Atlas | 2 MB | 1024x1024 |
| UI Vertex Buffer | 2 MB | Dynamic |
| Health Bar Pool | 1 MB | 200 entities max |
| Floating Text Pool | 256 KB | 64 texts max |
| UI Arena | 8 MB | Per-frame allocations |
| **Total** | **~30 MB** | |

### 14.3 Profiling Points

```c
typedef struct HUDMetrics {
    float update_time_ms;
    float render_time_ms;
    uint32_t draw_calls;
    uint32_t vertices_rendered;
    uint32_t textures_bound;
    uint32_t health_bars_rendered;
    uint32_t floating_texts_active;
} HUDMetrics;

void hud_gather_metrics(GameHUD* hud, HUDMetrics* metrics);

// Debug overlay
void hud_render_debug_overlay(GameHUD* hud, const HUDMetrics* metrics);
```

---

## 15. Implementation Phases

### Phase 1: Core HUD Foundation (Weeks 1-2)

**Deliverables:**
- [ ] HUD layout system (anchors, scaling)
- [ ] Champion portrait with level/XP
- [ ] Health and mana bars with animations
- [ ] Basic ability bar (icons, hotkeys)
- [ ] Item slots (empty state)

**Files:**
- `src/client/ui/hud/hud.h/c`
- `src/client/ui/hud/hud_layout.c`
- `src/client/ui/hud/champion_panel.h/c`
- `src/client/ui/hud/ability_bar.h/c`
- `src/client/ui/hud/item_bar.h/c`

### Phase 2: Ability System Integration (Weeks 3-4)

**Deliverables:**
- [ ] Cooldown overlays (radial sweep)
- [ ] Mana cost display
- [ ] Level pips
- [ ] Ability states (ready, cooldown, no mana, disabled)
- [ ] Active item cooldowns

**Files:**
- `src/client/ui/hud/ability_bar.c` (expand)
- `src/client/ui/widgets/cooldown_overlay.h/c`

### Phase 3: Minimap and Team Frames (Weeks 5-6)

**Deliverables:**
- [ ] Minimap terrain rendering
- [ ] Champion icons on minimap
- [ ] Camera viewport rectangle
- [ ] Click-to-move/ping support
- [ ] Allied team frames
- [ ] Ultimate ready indicators

**Files:**
- `src/client/ui/hud/minimap.h/c`
- `src/client/ui/hud/team_frames.h/c`

### Phase 4: World-Space UI (Weeks 7-8)

**Deliverables:**
- [ ] World health bars (champions, minions, towers)
- [ ] Floating combat text system
- [ ] Ability indicators (range, skillshot, AOE)
- [ ] Health bar culling

**Files:**
- `src/client/ui/hud/world_ui/health_bars.h/c`
- `src/client/ui/hud/world_ui/floating_text.h/c`
- `src/client/ui/hud/world_ui/indicators.h/c`

### Phase 5: Shop and Scoreboard (Weeks 9-10)

**Deliverables:**
- [ ] Scoreboard display (Tab)
- [ ] Item shop panel
- [ ] Search and filtering
- [ ] Item purchase/sell/undo
- [ ] Build path visualization

**Files:**
- `src/client/ui/hud/scoreboard.h/c`
- `src/client/ui/hud/shop.h/c`

### Phase 6: Ping System and Polish (Weeks 11-12)

**Deliverables:**
- [ ] Ping wheel UI
- [ ] Ping visuals (world and minimap)
- [ ] Rate limiting
- [ ] Sound integration
- [ ] Settings panel
- [ ] Colorblind modes
- [ ] Performance optimization

**Files:**
- `src/client/ui/hud/ping_wheel.h/c`
- `src/client/ui/hud/settings.h/c`
- `src/client/ui/hud/hud_render.c` (optimize)

---

## Appendix A: Key Constants

```c
// ============================================================================
// HUD Constants
// ============================================================================

// Screen positions (1080p baseline)
#define HUD_TOP_BAR_HEIGHT          48
#define HUD_BOTTOM_BAR_HEIGHT       120
#define HUD_TEAM_FRAME_WIDTH        96
#define HUD_MINIMAP_SIZE            220
#define HUD_MINIMAP_MARGIN          8
#define HUD_CHAT_WIDTH              320
#define HUD_CHAT_HEIGHT             200

// Champion panel
#define HUD_PORTRAIT_SIZE           64
#define HUD_HP_BAR_WIDTH            200
#define HUD_HP_BAR_HEIGHT           20
#define HUD_MP_BAR_HEIGHT           16

// Abilities
#define HUD_ABILITY_ICON_SIZE       48
#define HUD_ABILITY_SPACING         4
#define HUD_ABILITY_LEVEL_PIPS      5

// Items
#define HUD_ITEM_SLOT_SIZE          40
#define HUD_ITEM_SLOT_SPACING       2
#define HUD_ITEM_SLOT_COUNT         6

// World UI
#define WORLD_HEALTHBAR_CHAMPION_WIDTH  100
#define WORLD_HEALTHBAR_MINION_WIDTH    40
#define WORLD_HEALTHBAR_TOWER_WIDTH     120
#define WORLD_FLOAT_TEXT_DURATION       1.5f

// Animation
#define HUD_BAR_ANIMATE_SPEED       5.0f
#define HUD_FADE_SPEED              2.0f
#define HUD_PULSE_SPEED             3.0f

// Performance
#define HUD_MAX_HEALTH_BARS         200
#define HUD_MAX_FLOATING_TEXT       64
#define HUD_ICON_ATLAS_SIZE         2048
#define HUD_VERTEX_BUFFER_SIZE      65536
```

---

## Appendix B: Color Definitions

```c
// ============================================================================
// HUD Colors
// ============================================================================

// Team colors
#define COLOR_ALLY              0xFF3399FF  // Blue
#define COLOR_ENEMY             0xFFFF3333  // Red
#define COLOR_NEUTRAL           0xFFCCCC00  // Yellow

// Resource bars
#define COLOR_HEALTH            0xFF00CC00  // Green
#define COLOR_HEALTH_LOW        0xFFFF3333  // Red (<25%)
#define COLOR_HEALTH_DAMAGE     0xFF880000  // Dark red (recent)
#define COLOR_MANA              0xFF0066FF  // Blue
#define COLOR_ENERGY            0xFFFFFF00  // Yellow
#define COLOR_RAGE              0xFFFF0000  // Red

// Abilities
#define COLOR_ABILITY_READY     0xFFFFFFFF  // White (bright)
#define COLOR_ABILITY_COOLDOWN  0xFF444444  // Dark gray
#define COLOR_ABILITY_NO_MANA   0xFF4466AA  // Blue tint
#define COLOR_ABILITY_DISABLED  0xFF333333  // Gray

// Combat text
#define COLOR_DAMAGE_PHYSICAL   0xFFFFFFFF  // White
#define COLOR_DAMAGE_MAGIC      0xFFCC66FF  // Purple
#define COLOR_DAMAGE_TRUE       0xFFFFFFFF  // White (bold)
#define COLOR_DAMAGE_CRIT       0xFFFF8800  // Orange
#define COLOR_HEAL              0xFF00FF00  // Green
#define COLOR_GOLD              0xFFFFD700  // Gold

// Indicators
#define COLOR_INDICATOR_VALID   0x8800FF00  // Semi-transparent green
#define COLOR_INDICATOR_INVALID 0x88FF0000  // Semi-transparent red

// UI backgrounds
#define COLOR_PANEL_BG          0xCC1A1A2E  // Dark blue, semi-transparent
#define COLOR_PANEL_BORDER      0xFF333366  // Slightly lighter
```

---

## Appendix C: Input Bindings

```c
// ============================================================================
// Default HUD Input Bindings
// ============================================================================

typedef struct HUDBindings {
    // Abilities
    Key ability_q;              // Q
    Key ability_w;              // W
    Key ability_e;              // E
    Key ability_r;              // R

    // Items
    Key item_slots[6];          // 1, 2, 3, 4, 5, 6
    Key trinket;                // 4 (default ward)

    // Summoners
    Key summoner_d;             // D
    Key summoner_f;             // F

    // UI toggles
    Key scoreboard;             // Tab
    Key shop;                   // P
    Key chat;                   // Enter
    Key all_chat;               // Shift+Enter

    // Pings
    Key ping_generic;           // G
    Key ping_danger;            // V
    Key ping_missing;           // Ctrl+click minimap

    // Camera
    Key center_camera;          // Space
    Key lock_camera;            // Y
} HUDBindings;

// Quick cast settings
typedef enum QuickCastMode {
    QUICK_CAST_OFF,             // Show indicator, cast on release
    QUICK_CAST_ON,              // Cast immediately on press
    QUICK_CAST_WITH_INDICATOR,  // Show indicator, cast on press
} QuickCastMode;
```

---

**End of Document**

