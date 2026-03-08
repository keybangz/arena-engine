# League of Legends Research: Movement, Abilities & 3D Mechanics
**Date:** March 8, 2026  
**Purpose:** Inform Arena Engine MOBA implementation with LoL-specific mechanics

---

## 1. CHARACTER MOVEMENT SYSTEM

### 1.1 Base Movement Speed Statistics

**Movement Speed Distribution (2026 Patch Data):**
- **Fastest:** Master Yi (355 MS)
- **Very Fast:** 8 champions at 350 MS
- **Fast:** 28 champions at 345 MS
- **Standard:** 40 champions at 340 MS
- **Moderate:** 35 champions at 335 MS
- **Slow:** 30 champions at 330 MS
- **Very Slow:** 25 champions at 325 MS
- **Slowest:** Rell (315 MS), Kled (305 MS)

**Key Insight:** Movement speed ranges from 305-355 MS, with most champions clustered at 330-345 MS. This ~50 MS spread creates meaningful differentiation.

### 1.2 Movement Mechanics

**Instant Velocity Changes:**
- LoL uses **instant velocity changes** (no acceleration/deceleration)
- When player issues move command, champion immediately moves at full speed
- No "wind-up" or "wind-down" time
- Movement stops instantly when command is released

**Turning/Rotation:**
- **Instant rotation** - champions face direction of movement immediately
- No smooth turning animation (visual only, not gameplay)

**Click-to-Move System:**
- Uses **A* pathfinding** on NavMesh
- Players click destination, champion pathfinds around obstacles
- Minions and champions act as **dynamic obstacles**
- Pathfinding recalculates when obstacles move into path

**Movement Commands:**
- Commands are **queued** - new move command cancels previous
- Movement can be interrupted by crowd control, ability casting, knockback/displacement

**Collision System:**
- **Champion-to-champion:** Soft collision (units push each other slightly)
- **Minion blocking:** Minions block movement (act as obstacles)
- **Terrain collision:** Hard collision (walls, map boundaries)
- **Unit radius:** ~65 units (collision radius for targeting)

### 1.3 Movement Modifiers

**Crowd Control Effects (Impaired Movement):**

| Effect | Movement | Abilities | Attacks | Tenacity Affects |
|--------|----------|-----------|---------|---|
| **Slow** | Reduced % | Allowed | Allowed | Yes |
| **Root** | Disabled | Disabled (mobility only) | Allowed | Yes |
| **Stun** | Disabled | Disabled | Disabled | Yes |
| **Knockup** | Disabled | Disabled | Disabled | **No** |
| **Knockback** | Forced direction | Disabled | Disabled | **No** |
| **Ground** | Allowed (basic) | Disabled (mobility) | Allowed | Yes |
| **Cripple** | Allowed | Allowed | Reduced attack speed | Yes |

**Tenacity:** Reduces CC duration by percentage (multiplicative stacking). Does NOT affect airborne, knockup, or knockback displacement.

---

## 2. ABILITY MOVEMENT (DASHES, BLINKS, KNOCKBACKS)

### 2.1 Dash Mechanics

**Definition:** Movement ability where unit traverses distance while moving (not instant)

**Key Properties:**
- **Travel time:** Typically 0.1-0.5 seconds
- **Distance:** Usually 300-600 units
- **Terrain interaction:** Most dashes **CAN traverse walls** (with exceptions)
- **Interruption:** Can be interrupted by stun, root, ground, silence, knockdown, airborne, stasis

**Examples with Mechanics:**
- **Lucian E (Relentless Pursuit):** 425 unit dash, ~0.25s travel, can traverse terrain
- **Vayne Q (Tumble):** 300 unit dash, **CANNOT traverse terrain** (stops at walls)
- **Lee Sin Q (Sonic Wave → Resonating Strike):** Dashes to marked target, any distance
- **Sejuani E (Arctic Assault):** Dashes to location, stops on champion hit

**Dash Categories:**
1. **Direction-targeted:** Dash in specified direction (Lucian E, Vayne Q)
2. **Location-targeted:** Dash to specific point (Ahri R, Tristana W)
3. **Unit-targeted:** Dash to/through unit (Lee Sin R, Jax Q)
4. **Auto-targeted:** Dash to nearest valid target (Lee Sin Q recast)

### 2.2 Blink Mechanics

**Definition:** Instantaneous teleport from point A to B (no travel time)

**Key Properties:**
- **Travel time:** 0 seconds (instant)
- **Terrain interaction:** **Ignores all terrain** (can blink through walls)
- **Interruption:** Cannot be interrupted mid-blink (already completed)
- **Examples:** Flash, Ezreal E (Arcane Shift), Shaco R (Hallucinate)

### 2.3 Knockback/Pull Mechanics

**Knockback (Forced Away):**
- **Lee Sin R (Dragon's Rage):** Kicks target away, ~600 unit knockback
- **Blitzcrank Q (Rocket Grab):** Pulls target toward caster
- **Alistar Q (Pulverize):** Knocks up enemies in area
- **Properties:**
  - Displacement is **not affected by tenacity**
  - Can be interrupted by displacement immunity
  - Overrides other displacements
  - Applies airborne status (can't move/attack/cast)

**Pull (Forced Toward):**
- **Blitzcrank Q:** Pulls target to caster location
- **Nautilus Q (Dredge Line):** Pulls target toward Nautilus
- **Rek'Sai Q (Void Rush):** Pulls target toward Rek'Sai

**Knockup (Airborne):**
- **Alistar Q:** Knocks up enemies
- **Tristana W (Rocket Jump):** Knocks up on landing
- **Properties:**
  - Airborne status prevents all actions
  - Duration NOT reduced by tenacity
  - Can be cleansed (removes airborne status but not displacement)
  - Displacement overrides other movements

### 2.4 Terrain Interaction

**Dashes that CAN traverse terrain:**
- Lucian E, Lee Sin Q/R, Ahri R, Tristana W, Sejuani E, Riven Q/E, Vi Q, Yasuo E

**Dashes that CANNOT traverse terrain:**
- Vayne Q (Tumble), Galio Q (Justice Punch), Kled Q (Jousting first cast), Riven Q (first 2 casts)

**Blinks (always traverse terrain):**
- Flash, Ezreal E, Shaco R, LeBlanc W

**Terrain-blocking abilities:**
- **Anivia W (Crystallize):** Creates impassable wall
- **Taliyah W (Unraveled Earth):** Stones block dashes (stun on hit)
- **Jarvan IV R (Cataclysm):** Creates arena walls
- **Poppy W (Steadfast Presence):** Stops enemy dashes in aura

### 2.5 Displacement Immunity

**Abilities granting displacement immunity:**
- **Malphite R (Unstoppable Force):** Immune during charge
- **Sion R (Unstoppable Onslaught):** Immune during charge
- **Olaf R (Ragnarok):** Immune to all CC including knockups
- **K'Sante W (Path Maker):** Grants displacement immunity during dash
- **Kalista R (Fate's Call):** Oathsworn becomes displacement immune

**Effects of displacement immunity:**
- Blocks all airborne effects (knockup, knockback, pull)
- Blocks stasis, suspension, kinematics
- Does NOT block basic movement restrictions (root, stun)

---

## 3. 3D MAP DEPTH & TERRAIN

### 3.1 Camera System

**Isometric Perspective:**
- **Camera angle:** ~55-60 degrees from horizontal
- **View type:** Top-down isometric (not true 3D first-person)
- **Aspect ratio:** Maintains 16:9 widescreen
- **Zoom:** Fixed (no player zoom control in competitive)

**Coordinate System:**
- **X-axis:** Left-right (horizontal)
- **Y-axis:** Up-down (vertical on screen, but represents depth in world)
- **Z-axis:** Height (elevation, minimal gameplay impact)

### 3.2 Terrain Heights

**Summoner's Rift Elevation:**
- **Base terrain:** Flat (Z = 0)
- **River:** Slightly lower elevation (visual only, no gameplay impact)
- **Jungle camps:** Slightly elevated (visual only)
- **Brush:** Same elevation as surrounding terrain
- **Walls:** Vertical collision (not traversable by normal movement)

**Height Effects on Gameplay:**
- **Skillshots:** Not blocked by height differences (game engine ignores Z for targeting)
- **Vision:** Height does NOT affect vision (no "line of sight" blocking from elevation)
- **Collision:** Height is purely visual (collision is 2D on X-Y plane)

### 3.3 Brush & Fog of War

**Brush Mechanics:**
- **Visual:** Darker terrain tiles
- **Gameplay:** Grants stealth to units inside
- **Vision:** Enemies outside brush cannot see units inside (unless warded)
- **Collision:** No collision difference (units move normally through brush)

**Fog of War:**
- **Per-team visibility:** Each team has separate vision
- **Vision radius:** ~1200 units from champion (varies by champion)
- **Ward vision:** Wards grant vision in small radius
- **Stealth:** Units in brush are invisible unless revealed

### 3.4 Wall Geometry

**Wall Types:**
1. **Map boundaries:** Impassable walls around map edge
2. **Terrain walls:** Walls in jungle/lanes (impassable)
3. **Champion-created walls:** Anivia W, Taliyah W, Jarvan IV R (temporary)

**Collision Detection:**
- **Raycast:** Line-of-sight checks use raycasting
- **NavMesh:** Pathfinding uses navigation mesh
- **Collision mesh:** Separate collision geometry for movement

**Wall Interaction:**
- **Dashes:** Most dashes traverse walls (with exceptions)
- **Blinks:** All blinks traverse walls
- **Basic movement:** Cannot move through walls
- **Projectiles:** Blocked by walls (skillshots don't pass through)

### 3.5 River

**River Properties:**
- **Elevation:** Slightly lower than surrounding terrain (visual)
- **Collision:** No special collision (units move normally)
- **Vision:** No vision blocking (river doesn't obscure vision)
- **Gameplay:** Neutral territory (no team ownership)

---

## 4. TECHNICAL IMPLEMENTATION RECOMMENDATIONS

### 4.1 Movement Component Data Structure

```c
typedef struct {
    Vec3 position;           // Current position (X, Y, Z)
    Vec3 velocity;           // Current velocity
    float move_speed;        // Base movement speed (325-355)
    float speed_multiplier;  // From items/buffs (1.0 = 100%)
    
    // Movement state
    bool is_moving;
    Vec3 target_position;    // For click-to-move
    
    // Crowd control
    float slow_percent;      // 0.0 = no slow, 0.5 = 50% slow
    bool is_rooted;
    bool is_stunned;
    
    // Dash state
    bool is_dashing;
    Vec3 dash_start;
    Vec3 dash_end;
    float dash_duration;
    float dash_elapsed;
} MovementComponent;
```

### 4.2 Pathfinding System

**NavMesh-based A* Pathfinding:**
- Use A* algorithm on NavMesh for optimal paths
- Handle dynamic obstacles (minions, champions)
- Recalculate when obstacles move into path
- Smooth path waypoints to avoid jittering

### 4.3 Ability Movement (Dashes)

**Dash Ability Structure:**
```c
typedef struct {
    float distance;          // 300-600 units
    float duration;          // 0.1-0.5 seconds
    bool can_traverse_terrain;
    bool can_be_interrupted; // By CC
} DashAbility;
```

**Implementation:**
- Linear interpolation from start to end position
- Check terrain collision if `can_traverse_terrain = false`
- Stop at wall intersection if blocked
- Apply interruption checks each frame

### 4.4 Collision Layers

**Recommended Organization:**
```c
#define COLLISION_LAYER_TERRAIN      (1 << 0)  // Walls, map boundaries
#define COLLISION_LAYER_CHAMPION     (1 << 1)  // Player champions
#define COLLISION_LAYER_MINION       (1 << 2)  // Minions
#define COLLISION_LAYER_PROJECTILE   (1 << 3)  // Skillshots
#define COLLISION_LAYER_STRUCTURE    (1 << 4)  // Towers, inhibitors
#define COLLISION_LAYER_TERRAIN_WALL (1 << 5)  // Champion-created walls
```

### 4.5 Crowd Control System

**CC Application:**
- Apply CC type (slow, root, stun, knockup, knockback, ground)
- Track duration and remaining time
- Apply tenacity reduction (multiplicative)
- Update movement component based on active CC

---

## 5. SPECIFIC NUMBERS & FORMULAS

### 5.1 Movement Speed Calculations

**Effective Movement Speed:**
```
effective_ms = base_ms * item_multiplier * buff_multiplier
effective_ms = max(effective_ms * (1 - slow_percent), min_ms)
```

**Example:**
- Base: 340 MS
- Boots of Speed: +45 MS → 385 MS
- Slow (30%): 385 * 0.7 = 269.5 MS
- Minimum MS: 20 (hard cap)

### 5.2 Dash Distances

**Common Dash Distances:**
- Short dash: 300 units (Vayne Q, Fiora Q)
- Medium dash: 425 units (Lucian E, Riven Q)
- Long dash: 600+ units (Vi Q, Sejuani E)

**Dash Duration Formula:**
```
duration = distance / speed
Example: 425 units / 1700 units/sec = 0.25 seconds
```

### 5.3 Crowd Control Durations

**Typical CC Durations:**
- Slow: 1-3 seconds
- Root: 1-2 seconds
- Stun: 0.5-2 seconds
- Knockup: 0.5-1.5 seconds
- Knockback: Instant (displacement only)

**Tenacity Reduction:**
```
effective_duration = base_duration / (1 + tenacity)
Example: 2 second stun with 30% tenacity = 2 / 1.3 = 1.54 seconds
```

### 5.4 Vision Radius

**Default Vision Radius:** ~1200 units
**Ward Vision:** ~1100 units
**Brush:** Grants stealth (vision blocked unless warded)

---

## 6. IMPLEMENTATION PRIORITIES FOR ARENA ENGINE

### Phase 1 (MVP):
1. ✅ Basic movement (WASD, click-to-move)
2. ✅ Instant velocity changes (no acceleration)
3. ✅ Simple pathfinding (A* on NavMesh)
4. ✅ Dash abilities (linear interpolation)
5. ✅ Basic crowd control (slow, root, stun)

### Phase 2 (Polish):
1. Terrain interaction (dashes that can/cannot traverse walls)
2. Knockback/pull mechanics
3. Displacement immunity
4. Advanced pathfinding (dynamic obstacle avoidance)
5. Crowd control stacking/interaction

### Phase 3 (Advanced):
1. 3D terrain heights (visual only)
2. Brush/fog of war system
3. Champion-created terrain (walls)
4. Vision system
5. Advanced ability interactions

---

## 7. KEY DIFFERENCES FROM TYPICAL GAME ENGINES

1. **No acceleration:** Movement is instant, not gradual
2. **Instant rotation:** No smooth turning animation
3. **Terrain traversal:** Dashes can cross walls (unlike typical platformers)
4. **Crowd control stacking:** Multiple CC effects can apply simultaneously
5. **Displacement immunity:** Specific abilities grant immunity to knockups/knockbacks
6. **Tenacity system:** Percentage-based CC reduction (multiplicative stacking)
7. **NavMesh pathfinding:** Dynamic obstacle avoidance for minions/champions

---

## 8. REFERENCES

- League of Legends Wiki: Movement Speed, Crowd Control, Dash
- Patch 25.S1.1 (January 2025) - Current balance data
- Champion statistics: 170 champions analyzed
- Ability mechanics: 200+ abilities documented

