---

## APPENDIX A: CODE STRUCTURE RECOMMENDATIONS

### Champion Definition Structure (C)

```c
// In src/arena/game/champion.c - Add to champion registry

typedef struct {
    uint32_t id;
    const char* name;
    const char* title;
    ChampionStats base_stats;
    uint32_t ability_ids[MAX_CHAMPION_ABILITIES];
    uint32_t color;
} ChampionDef;

// Jinx champion definition
ChampionDef jinx_def = {
    .id = CHAMPION_JINX,
    .name = "Jinx",
    .title = "The Loose Cannon",
    .base_stats = {
        .health = 590,
        .health_regen = 3.75,
        .mana = 260,
        .mana_regen = 6.7,
        .attack_damage = 59,
        .ability_power = 0,
        .armor = 32,
        .magic_resist = 30,
        .attack_speed = 0.625,
        .move_speed = 330,
        .health_per_level = 95,
        .mana_per_level = 40,
        .attack_damage_per_level = 2.2,
        .armor_per_level = 4,
        .magic_resist_per_level = 1.25
    },
    .ability_ids = {
        ABILITY_JINX_PASSIVE,
        ABILITY_JINX_Q,
        ABILITY_JINX_W,
        ABILITY_JINX_E,
        ABILITY_JINX_R
    },
    .color = 0xFF00AAFF  // Blue/Pink
};
```

### Ability Definition Structure

```c
// Jinx Q - Switcheroo!
AbilityDef jinx_q = {
    .id = ABILITY_JINX_Q,
    .name = "Switcheroo!",
    .target_type = ABILITY_TARGET_SELF,
    .range = 0,
    .radius = 0,
    .cooldown = 0,  // No cooldown for toggle
    .cast_time = 0.9,
    .mana_cost = 0,  // Cost per attack instead
    .effect_type = EFFECT_TOGGLE,
    .damage = 0,
    .projectile_speed = 0
};

// Jinx W - Zap!
AbilityDef jinx_w = {
    .id = ABILITY_JINX_W,
    .name = "Zap!",
    .target_type = ABILITY_TARGET_DIRECTION,
    .range = 1500,
    .radius = 60,  // Projectile width
    .cooldown = 8,  // Decreases per level
    .cast_time = 0.3,
    .mana_cost = 40,  // Increases per level
    .effect_type = EFFECT_PROJECTILE,
    .damage = 10,  // Base damage
    .projectile_speed = 2000
};

// Jinx E - Flame Chompers!
AbilityDef jinx_e = {
    .id = ABILITY_JINX_E,
    .name = "Flame Chompers!",
    .target_type = ABILITY_TARGET_POINT,
    .range = 925,
    .radius = 300,  // Explosion radius
    .cooldown = 24,  // Decreases per level
    .cast_time = 0.3,
    .mana_cost = 90,
    .effect_type = EFFECT_TRAP,
    .damage = 90,  // Base damage
    .duration = 5  // Trap duration
};

// Jinx R - Super Mega Death Rocket!
AbilityDef jinx_r = {
    .id = ABILITY_JINX_R,
    .name = "Super Mega Death Rocket!",
    .target_type = ABILITY_TARGET_DIRECTION,
    .range = 30000,  // Global
    .radius = 500,  // Explosion radius
    .cooldown = 85,  // Decreases per level
    .cast_time = 0.6,
    .mana_cost = 100,
    .effect_type = EFFECT_GLOBAL_PROJECTILE,
    .damage = 32.5,  // Minimum damage
    .projectile_speed = 1500  // Initial speed
};
```

### Component Structure for Jinx-Specific State

```c
// New component for weapon state
typedef struct JinxWeaponState {
    uint8_t current_weapon;  // 0 = minigun, 1 = rocket
    uint8_t minigun_stacks;  // 0-3 stacks
    float stack_timer;       // Time until stack expires
} JinxWeaponState;

// New component for passive tracking
typedef struct JinxPassiveState {
    Entity last_damaged_entity;
    float damage_timer;  // 3-second window
    bool passive_active;
    float passive_timer;  // 6-second duration
} JinxPassiveState;
```

---

## APPENDIX B: ABILITY IMPLEMENTATION PSEUDOCODE

### Q: Switcheroo! Implementation

```c
void jinx_q_cast(World* world, Entity caster) {
    JinxWeaponState* weapon = world_get_component(world, caster, COMPONENT_JINX_WEAPON);

    // Toggle weapon
    weapon->current_weapon = (weapon->current_weapon == 0) ? 1 : 0;

    // Play animation
    play_animation(caster, "weapon_swap");

    // Apply weapon-specific effects
    if (weapon->current_weapon == 0) {
        // Minigun mode
        set_attack_range(caster, 525);
        set_attack_damage_multiplier(caster, 1.0);
    } else {
        // Rocket mode
        set_attack_range(caster, 625);  // +100 at level 1
        set_attack_damage_multiplier(caster, 1.1);
    }
}

void jinx_q_on_attack(World* world, Entity caster) {
    JinxWeaponState* weapon = world_get_component(world, caster, COMPONENT_JINX_WEAPON);

    if (weapon->current_weapon == 0) {
        // Minigun: apply attack speed stack
        if (weapon->minigun_stacks < 3) {
            weapon->minigun_stacks++;
        }
        weapon->stack_timer = 2.5;

        // Apply buff
        apply_buff(caster, BUFF_MINIGUN_STACK, weapon->minigun_stacks);
    } else {
        // Rocket: deduct mana
        Champion* champ = world_get_champion(world, caster);
        champ->current_mana -= 20;
    }
}
```

### W: Zap! Implementation

```c
void jinx_w_cast(World* world, Entity caster, Vec3 direction) {
    // Create projectile
    Entity projectile = world_spawn(world);

    // Set projectile properties
    Transform* transform = world_get_transform(world, caster);
    set_position(world, projectile, transform->position);

    Projectile* proj = world_add_component(world, projectile, COMPONENT_PROJECTILE);
    proj->owner = caster;
    proj->direction = normalize(direction);
    proj->speed = 2000;
    proj->range = 1500;
    proj->hit_first_only = true;

    // Damage calculation
    Champion* champ = world_get_champion(world, caster);
    float damage = 10 + (champ->attack_damage * 1.4);
    proj->damage = damage;

    // Slow effect
    proj->on_hit = jinx_w_on_hit;
}

void jinx_w_on_hit(World* world, Entity projectile, Entity target) {
    // Apply slow
    apply_crowd_control(world, target, CC_SLOW, 2.0, 0.4);

    // Apply reveal
    apply_reveal(world, target, 2.0);

    // Destroy projectile
    world_despawn(world, projectile);
}
```

### E: Flame Chompers! Implementation

```c
void jinx_e_cast(World* world, Entity caster, Vec3 position) {
    // Create 3 chompers in triangular formation
    Vec3 offsets[3] = {
        {0, 0, 0},
        {150, 0, 0},
        {-75, 130, 0}
    };

    for (int i = 0; i < 3; i++) {
        Entity chomper = world_spawn(world);

        Transform* transform = world_add_component(world, chomper, COMPONENT_TRANSFORM);
        transform->position = vec3_add(position, offsets[i]);

        Trap* trap = world_add_component(world, chomper, COMPONENT_TRAP);
        trap->owner = caster;
        trap->arm_time = 0.5;
        trap->duration = 5.0;
        trap->trigger_radius = 100;
        trap->on_trigger = jinx_e_on_trigger;
    }
}

void jinx_e_on_trigger(World* world, Entity trap, Entity target) {
    // Get trap owner for damage calculation
    Trap* trap_comp = world_get_component(world, trap, COMPONENT_TRAP);
    Entity owner = trap_comp->owner;
    Champion* champ = world_get_champion(world, owner);

    // Calculate damage
    float damage = 90 + (champ->ability_power * 1.0);

    // Apply damage to target
    DamageEvent event = {
        .source = owner,
        .target = target,
        .amount = damage,
        .type = DAMAGE_MAGIC
    };
    combat_apply_damage(world, &event);

    // Apply root
    apply_crowd_control(world, target, CC_ROOT, 1.5, 0);

    // Area damage to nearby enemies
    apply_area_damage(world, trap_comp->position, 300, damage * 0.8, owner);

    // Destroy trap
    world_despawn(world, trap);
}
```

### R: Super Mega Death Rocket! Implementation

```c
void jinx_r_cast(World* world, Entity caster, Vec3 direction) {
    // Create global projectile
    Entity rocket = world_spawn(world);

    Transform* transform = world_get_transform(world, caster);
    set_position(world, rocket, transform->position);

    GlobalProjectile* proj = world_add_component(world, rocket, COMPONENT_GLOBAL_PROJECTILE);
    proj->owner = caster;
    proj->direction = normalize(direction);
    proj->initial_speed = 1500;
    proj->final_speed = 2500;
    proj->acceleration_time = 1.0;
    proj->current_speed = 1500;
    proj->travel_time = 0;

    // Damage calculation
    Champion* champ = world_get_champion(world, caster);
    float bonus_ad = champ->attack_damage - 59;  // Subtract base AD

    proj->min_damage = 32.5 + (bonus_ad * 0.165);
    proj->max_damage = 200 + (bonus_ad * 1.2);
    proj->missing_health_percent = 0.25;  // 25% at level 1

    proj->on_hit = jinx_r_on_hit;
}

void jinx_r_on_hit(World* world, Entity rocket, Entity target) {
    GlobalProjectile* proj = world_get_component(world, rocket, COMPONENT_GLOBAL_PROJECTILE);

    // Calculate damage based on travel time
    float damage_ratio = min(proj->travel_time / 1.0, 1.0);
    float base_damage = proj->min_damage + (proj->max_damage - proj->min_damage) * damage_ratio;

    // Add missing health damage
    Health* health = world_get_health(world, target);
    float missing_health = health->max - health->current;
    float missing_health_damage = missing_health * proj->missing_health_percent;

    float total_damage = base_damage + missing_health_damage;

    // Check for execute
    if (health->current <= total_damage) {
        total_damage *= 1.5;  // Execute bonus
    }

    // Apply damage
    DamageEvent event = {
        .source = proj->owner,
        .target = target,
        .amount = total_damage,
        .type = DAMAGE_PHYSICAL
    };
    combat_apply_damage(world, &event);

    // Area damage to nearby enemies
    apply_area_damage(world, get_position(world, target), 500, total_damage * 0.8, proj->owner);

    // Destroy rocket
    world_despawn(world, rocket);
}
```

---

## APPENDIX C: PASSIVE IMPLEMENTATION

```c
void jinx_passive_on_damage(World* world, Entity caster, Entity target) {
    JinxPassiveState* passive = world_get_component(world, caster, COMPONENT_JINX_PASSIVE);

    // Track damage to entity
    passive->last_damaged_entity = target;
    passive->damage_timer = 3.0;  // 3-second window
}

void jinx_passive_on_kill(World* world, Entity caster, Entity killed) {
    JinxPassiveState* passive = world_get_component(world, caster, COMPONENT_JINX_PASSIVE);

    // Check if killed entity was damaged within 3 seconds
    if (killed == passive->last_damaged_entity && passive->damage_timer > 0) {
        // Apply passive buff
        passive->passive_active = true;
        passive->passive_timer = 6.0;

        // Apply movement speed buff (175% decaying)
        apply_decaying_buff(caster, BUFF_GET_EXCITED, 6.0, 1.75);

        // Apply attack speed buff (25%)
        apply_buff(caster, BUFF_GET_EXCITED_AS, 0.25);
    }
}

void jinx_passive_system_update(World* world, float dt) {
    // Update all Jinx passive states
    for (Entity entity = 0; entity < MAX_ENTITIES; entity++) {
        if (!world_is_alive(world, entity)) continue;

        JinxPassiveState* passive = world_get_component(world, entity, COMPONENT_JINX_PASSIVE);
        if (!passive) continue;

        // Update damage timer
        if (passive->damage_timer > 0) {
            passive->damage_timer -= dt;
        }

        // Update passive timer
        if (passive->passive_timer > 0) {
            passive->passive_timer -= dt;
        } else {
            passive->passive_active = false;
        }
    }
}
```

---

## APPENDIX D: STAT SCALING FORMULAS

### Health Scaling
```
Health(level) = 590 + (level - 1) * 95
Level 18: 590 + 17 * 95 = 2,205 HP
```

### Attack Damage Scaling
```
AD(level) = 59 + (level - 1) * 2.2
Level 18: 59 + 17 * 2.2 = 96.4 AD
```

### Attack Speed Scaling
```
AS(level) = 0.625 * (1 + (level - 1) * 0.021)
Level 18: 0.625 * (1 + 17 * 0.021) = 0.625 * 1.357 = 0.848 attacks/second
```

### Armor Scaling
```
Armor(level) = 32 + (level - 1) * 4
Level 18: 32 + 17 * 4 = 100 Armor
```

### Damage Reduction from Armor
```
Damage Reduction = Armor / (100 + Armor)
At 100 Armor: 100 / 200 = 50% reduction
```

---

## APPENDIX E: BALANCE TUNING PARAMETERS

### Ability Cooldown Scaling
| Ability | Level 1 | Level 5 | Reduction |
|---------|---------|---------|-----------|
| W (Zap!) | 8s | 4s | 1s per level |
| E (Chompers) | 24s | 10s | 3.5s per level |
| R (Rocket) | 85s | 45s | 10s per level |

### Mana Cost Scaling
| Ability | Level 1 | Level 5 | Increase |
|---------|---------|---------|----------|
| W (Zap!) | 40 | 60 | 5 per level |
| E (Chompers) | 90 | 90 | 0 per level |
| R (Rocket) | 100 | 100 | 0 per level |

### Damage Scaling
| Ability | Base | Per Level | AD Ratio |
|---------|------|-----------|----------|
| W (Zap!) | 10 | +50 | 1.4x |
| E (Chompers) | 90 | +50 | 1.0x AP |
| R (Rocket) | 32.5 | +15 | 0.165x bonus AD |


# Jinx - Reference Champion Design

**Version:** 1.0.0
**Date:** 2026-03-08
**Purpose:** Complete reference implementation for the first playable champion in Arena Engine

---

## 1. BASE STATS & SCALING

### Level 1 Base Stats
| Stat | Value | Per-Level Scaling |
|------|-------|-------------------|
| Health | 590 | +95 per level |
| Health Regen | 3.75 | +0.75 per level |
| Mana | 260 | +40 per level |
| Mana Regen | 6.7 | +0.45 per level |
| Attack Damage | 59 | +2.2 per level |
| Ability Power | 0 | +0 per level |
| Armor | 32 | +4 per level |
| Magic Resist | 30 | +1.25 per level |
| Attack Speed | 0.625 | +2.1% per level |
| Movement Speed | 330 | +0 per level |
| Attack Range | 525 (minigun) / 625 (rocket) | Variable |
| Crit Chance | 0% | +0% per level |
| Crit Damage | 175% | +0% per level |

### Stat Progression Example
- **Level 1:** 590 HP, 59 AD, 32 Armor
- **Level 6:** 1065 HP, 70.2 AD, 52 Armor
- **Level 18:** 2300 HP, 98.6 AD, 96 Armor

---

## 2. ABILITY KIT - COMPLETE SPECIFICATIONS

### PASSIVE: Get Excited!

**Trigger Condition:**
- Whenever a champion, epic jungle monster, or structure that Jinx has dealt damage to within the last 3 seconds is killed or destroyed

**Effects:**
- **Movement Speed Bonus:** +175% (decaying over duration)
- **Attack Speed Bonus:** +25%
- **Duration:** 6 seconds
- **Decay:** Linear decay from 175% to 0% over 6 seconds

**Visual Indicators:**
- Blue/pink particle trail around Jinx
- Weapon glows with energy
- Movement speed lines/trails
- Buff icon in UI

**Implementation Notes:**
- Tracks damage dealt to entities within 3-second window
- Triggers on kill/destroy event
- Stacks with other movement speed bonuses
- Decaying buff (not flat duration)

---

### Q: SWITCHEROO! (Toggle Ability)

**Type:** Toggle (No Cooldown, No Mana Cost to Toggle)

#### Pow-Pow (Minigun Mode)
**Activation:** Toggle to minigun
**Range:** 525 units (standard attack range)
**Attack Damage:** 100% AD (normal attacks)

**Stacking Attack Speed Buff:**
- **Per Stack:** +30/55/80/105/130% attack speed
- **Max Stacks:** 3
- **Stack Duration:** 2.5 seconds
- **Stack Refresh:** Each attack refreshes duration and adds stack if below max

**Mechanics:**
- Scales 10% less with bonus attack speed (multiplicative penalty)
- No mana cost per attack
- Rapid fire playstyle
- Stacks decay individually after 2.5s

#### Fishbones (Rocket Launcher Mode)
**Activation:** Toggle to rocket launcher
**Range:** 525 + 100/125/150/175/200 bonus range (625-725 units)
**Attack Damage:** 110% AD (splash damage)

**Splash Damage:**
- Deals 110% AD to target and nearby enemies
- Splash radius: ~300 units (estimated)
- All enemies in radius take full damage

**Mana Cost:** 20 mana per rocket attack
**Mechanics:**
- Scales 10% less with bonus attack speed
- Longer range enables safer positioning
- Higher mana cost limits sustained fire
- Splash enables waveclear and teamfight damage

**Toggle Mechanics:**
- **Cast Time:** 0.9 seconds (can be interrupted)
- **Cooldown:** None (instant toggle)
- **Mana Cost:** None (only rocket attacks cost mana)
- **Animation:** Weapon swap animation plays

**Implementation Complexity:** MEDIUM
- Requires dual attack system
- Conditional damage calculation
- Mana cost per attack (not per ability)
- Attack speed scaling penalty

---

### W: ZAP! (Skillshot)

**Type:** Linear Skillshot / Projectile

**Targeting:** Direction-based (point-and-click direction)
**Range:** 1500 units
**Projectile Speed:** ~2000 units/second (estimated)
**Projectile Width:** ~60 units (thin beam)

**Damage:**
- **Base Damage:** 10/60/110/160/210 (+1.4 AD)
- **Damage Type:** Physical
- **Scaling:** 140% Attack Damage ratio

**Crowd Control:**
- **Slow Amount:** 40/50/60/70/80%
- **Slow Duration:** 2 seconds
- **Reveal Duration:** 2 seconds (reveals target location)

**Cooldown:** 8/7/6/5/4 seconds
**Mana Cost:** 40/45/50/55/60
**Cast Time:** ~0.3 seconds (instant cast)

**Mechanics:**
- Hits first enemy champion only
- Can be blocked by minions (stops at first unit hit)
- Reveals target for 2 seconds (vision granted)
- Slow is applied on hit
- Long range enables poke from safety

**Visual Effects:**
- Blue/pink electrical beam
- Spark/shock particle on impact
- Slow visual indicator on target
- Reveal indicator

**Implementation Complexity:** MEDIUM
- Projectile collision detection
- First-hit-only logic
- Reveal mechanic integration
- Slow application

---

### E: FLAME CHOMPERS! (Trap Placement)

**Type:** Area Control / Trap Placement

**Targeting:** Point-target (place at location)
**Range:** 925 units
**Number of Chompers:** 3 per cast
**Chomper Spacing:** ~150 units apart (triangular formation)

**Trap Mechanics:**
- **Arm Time:** ~0.5 seconds (after placement)
- **Duration on Ground:** 5 seconds
- **Trigger Radius:** ~100 units (enemy champion proximity)
- **Trigger Type:** Detonates on enemy champion contact

**Damage & CC:**
- **Damage:** 90/140/190/240/290 (+100% AP)
- **Damage Type:** Magic
- **Root Duration:** 1.5 seconds
- **Root Radius:** ~300 units (nearby enemies)

**Cooldown:** 24/20.5/17/13.5/10 seconds
**Mana Cost:** 90
**Cast Time:** ~0.3 seconds

**Mechanics:**
- Places 3 chompers in formation
- Each chomper is independent
- Detonates on first champion contact
- Nearby enemies (within radius) also take damage and root
- Provides vision of surrounding area
- Can be destroyed by enemies (estimated ~1 hit)
- Stacks on ground (multiple casts create multiple trap zones)

**Visual Effects:**
- Chomper sprites (animated jaws)
- Explosion particle effect on trigger
- Root visual indicator
- Vision indicator

**Implementation Complexity:** MEDIUM-COMPLEX
- Multiple entity spawning
- Proximity detection
- Area damage calculation
- Vision granting
- Trap destruction mechanics

---

### R: SUPER MEGA DEATH ROCKET! (Global Ultimate)

**Type:** Global Skillshot / Ultimate

**Targeting:** Direction-based (global range)
**Range:** Global (entire map)
**Projectile Speed:** Accelerates over first 1 second
- **Initial Speed:** ~1500 units/second
- **Final Speed:** ~2500 units/second
- **Acceleration:** Linear over 1 second

**Damage Calculation:**
- **Minimum Damage:** 32.5/47.5/62.5 (+0.165 bonus AD) + 25/30/35% missing health
- **Maximum Damage:** 200/350/500 (+1.2 bonus AD) + 25/30/35% missing health
- **Damage Type:** Physical
- **Scaling:** Bonus AD (not total AD)

**Damage Scaling by Distance:**
- Damage increases linearly over first 1 second of travel
- At 0.5s: ~50% of max damage
- At 1.0s: 100% of max damage
- After 1.0s: Maintains max damage

**Execute Mechanic:**
- If target would die from damage, bonus execute damage applies
- Missing health damage cannot exceed 1200 against monsters
- Bonus damage on execute (estimated +50% damage)

**Explosion Radius:**
- **Primary Target:** Full damage
- **Nearby Enemies:** 80% damage (within ~500 unit radius)
- **Explosion Type:** Circular AoE

**Cooldown:** 85/65/45 seconds
**Mana Cost:** 100
**Cast Time:** 0.6 seconds (channel/cast)

**Mechanics:**
- Global range enables map-wide impact
- Accelerating projectile rewards planning
- Missing health scaling enables execute potential
- Nearby enemies take reduced damage
- Can be blocked by terrain/structures
- Visible to all players (global indicator)

**Visual Effects:**
- Large rocket projectile with trail
- Explosion particle effect on impact
- Shockwave visual
- Global sound effect
- Minimap indicator

**Implementation Complexity:** COMPLEX
- Global projectile system
- Acceleration mechanics
- Missing health calculation
- Execute detection
- Area damage with falloff
- Terrain collision

---

## 3. ABILITY PROGRESSION

### Recommended Leveling Order
**Early Game (Levels 1-6):**
- Level 1: Q (Switcheroo!)
- Level 2: W (Zap!)
- Level 3: E (Flame Chompers!)
- Level 4: Q
- Level 5: W
- Level 6: R (Super Mega Death Rocket!)

**Mid Game (Levels 7-11):**
- Prioritize: Q > W > E
- Reason: Q provides consistent DPS, W provides utility, E provides control

**Late Game (Levels 12-18):**
- Max R whenever available (levels 6, 11, 16)
- Max Q first (primary damage source)
- Max W second (cooldown reduction)
- Max E last (utility ability)

### Ability Scaling Summary
| Ability | Scaling Type | Primary Stat | Secondary Stat |
|---------|--------------|--------------|-----------------|
| Q (Minigun) | Attack Speed | Attack Speed | - |
| Q (Rocket) | Damage | Attack Damage | - |
| W | Damage + CC | Attack Damage | - |
| E | Damage + CC | Ability Power | - |
| R | Damage + Execute | Bonus Attack Damage | Missing Health |

---

## 4. ANIMATIONS REQUIRED

### Idle Animations
- **Minigun Idle:** Jinx holds minigun, twitching/fidgeting
- **Rocket Idle:** Jinx holds rocket launcher, looking around
- **Passive Active Idle:** Excited animation with energy effects

### Movement Animations
- **Minigun Walk:** Bouncy, energetic walk with minigun
- **Rocket Walk:** Heavier walk with rocket launcher
- **Passive Active Run:** Fast run with speed trail effects

### Attack Animations
- **Minigun Auto Attack:** Rapid fire animation, 3-frame attack
- **Rocket Auto Attack:** Slower, heavier attack with recoil
- **Attack Speed Stacking:** Animation speed increases with stacks

### Ability Animations
- **Q Toggle Animation:** Weapon swap (0.9s duration)
  - Minigun to Rocket: Holster minigun, draw rocket
  - Rocket to Minigun: Holster rocket, draw minigun
- **W Cast Animation:** Aim and fire beam (0.3s)
- **E Cast Animation:** Throw/place chompers (0.3s)
- **R Cast Animation:** Channel and fire rocket (0.6s)

### Special Animations
- **Passive Trigger:** Excitement animation (brief)
- **Death Animation:** Explosion/destruction animation
- **Respawn Animation:** Spawn-in animation
- **Recall Animation:** Teleport/recall animation
- **Hit Reaction:** Knockback/flinch animation
- **Crowd Control:** Root animation (Flame Chompers)

### Animation Timing
| Animation | Duration | Notes |
|-----------|----------|-------|
| Minigun Attack | 0.4s | Can be cancelled by movement |
| Rocket Attack | 0.6s | Longer wind-up |
| Q Toggle | 0.9s | Can be interrupted |
| W Cast | 0.3s | Instant projectile |
| E Cast | 0.3s | Instant placement |
| R Cast | 0.6s | Channel ability |

---

## 5. VISUAL DESIGN ELEMENTS

### Color Palette
- **Primary Colors:** Blue and Pink/Magenta
- **Hair:** Blue and pink streaks
- **Clothing:** Dark with neon accents
- **Weapons:** Metallic with blue/pink energy effects
- **Particles:** Blue/pink electrical effects

### Weapon Designs

#### Pow-Pow (Minigun)
- **Style:** Compact, rapid-fire minigun
- **Color:** Metallic gray with blue accents
- **Size:** Medium (held in one hand)
- **Effects:** Blue spinning barrel, electrical arcs
- **Ammo Indicator:** Visible ammo counter or glow

#### Fishbones (Rocket Launcher)
- **Style:** Large, shoulder-mounted rocket launcher
- **Color:** Metallic with pink/magenta accents
- **Size:** Large (two-handed weapon)
- **Effects:** Pink energy core, rocket trail
- **Ammo Indicator:** Visible rocket in chamber

### Ability Visual Effects

#### Q: Switcheroo!
- **Weapon Swap:** Smooth transition animation
- **Minigun Mode:** Blue electrical aura around weapon
- **Rocket Mode:** Pink energy glow around weapon
- **Attack Speed Stacks:** Increasing glow intensity (1-3 stacks)

#### W: Zap!
- **Projectile:** Blue electrical beam
- **Impact:** Spark burst and electrical explosion
- **Slow Effect:** Blue electrical chains on target
- **Reveal:** Target glows with blue outline

#### E: Flame Chompers!
- **Placement:** Chomper sprites appear (animated jaws)
- **Idle:** Snapping animation, waiting for trigger
- **Trigger:** Explosion with fire/electrical particles
- **Root Effect:** Blue chains binding target

#### R: Super Mega Death Rocket!
- **Projectile:** Large rocket with pink/blue trail
- **Travel:** Accelerating particle trail
- **Impact:** Large explosion with shockwave
- **Nearby Damage:** Secondary explosion rings
- **Execute:** Enhanced explosion effect

### Particle Effects Summary
| Effect | Color | Trigger |
|--------|-------|---------|
| Minigun Attack | Blue | Each attack |
| Rocket Attack | Pink | Each attack |
| Passive Trigger | Blue/Pink | Kill/destroy |
| W Impact | Blue | Zap hit |
| E Explosion | Orange/Red | Chomper trigger |
| R Impact | Pink/Blue | Rocket hit |

---

## 6. IMPLEMENTATION COMPLEXITY ANALYSIS

### Ability Complexity Ratings

#### Q: Switcheroo! - **MEDIUM**
**Complexity Factors:**
- Dual attack system (two weapon modes)
- Conditional damage calculation (110% vs 100% AD)
- Attack speed scaling penalty (10% less)
- Mana cost per attack (not per ability)
- Stack management (up to 3 stacks)

**Dependencies:**
- Attack system (must support dual modes)
- Mana system (per-attack cost)
- Buff system (attack speed stacking)
- Animation system (weapon swap)

**Estimated LOC:** 300-400 lines

---

#### W: Zap! - **MEDIUM**
**Complexity Factors:**
- Projectile system (linear skillshot)
- First-hit-only logic
- Slow application
- Reveal mechanic
- Long range (1500 units)

**Dependencies:**
- Projectile system
- Collision detection
- Crowd control system (slow)
- Vision/reveal system
- Cooldown system

**Estimated LOC:** 250-350 lines

---

#### E: Flame Chompers! - **MEDIUM-COMPLEX**
**Complexity Factors:**
- Multiple entity spawning (3 chompers)
- Proximity detection
- Area damage calculation
- Vision granting
- Trap destruction mechanics
- Formation placement

**Dependencies:**
- Entity spawning system
- Proximity detection
- Area damage system
- Vision system
- Trap/minion system

**Estimated LOC:** 400-500 lines

---

#### R: Super Mega Death Rocket! - **COMPLEX**
**Complexity Factors:**
- Global projectile system
- Acceleration mechanics (over 1 second)
- Missing health calculation
- Execute detection
- Area damage with falloff (80% nearby)
- Terrain collision
- Global visibility

**Dependencies:**
- Global projectile system
- Acceleration physics
- Health calculation system
- Area damage system
- Terrain collision
- Global event system

**Estimated LOC:** 500-700 lines

---

#### Passive: Get Excited! - **SIMPLE**
**Complexity Factors:**
- Damage tracking (3-second window)
- Kill/destroy event detection
- Buff application
- Decaying buff (not flat duration)

**Dependencies:**
- Damage tracking system
- Event system (kill/destroy)
- Buff system (decaying)

**Estimated LOC:** 150-200 lines

---

## 7. SYSTEM DEPENDENCIES

### Required Systems
1. **Attack System** - Dual weapon modes, per-attack mana cost, attack speed scaling
2. **Projectile System** - Linear projectiles, acceleration, collision detection, area damage
3. **Crowd Control System** - Slow and root application, duration tracking
4. **Buff System** - Stacking buffs, decaying buffs, buff icons
5. **Vision System** - Reveal mechanic, vision granting, fog of war integration
6. **Trap System** - Trap placement, proximity detection, trap destruction
7. **Event System** - Kill events, destroy events, damage events

### Optional Enhancements
- Animation system (for smooth transitions)
- Particle system (for visual effects)
- Sound system (for audio feedback)
- UI system (for ability indicators)

---

## 8. IMPLEMENTATION ROADMAP

### Phase 1: Core Mechanics (Week 1-2)
1. Implement Q (Switcheroo!) - weapon toggle system
2. Implement Passive (Get Excited!) - kill tracking
3. Basic attack system with dual modes
4. Attack speed stacking

### Phase 2: Skillshots (Week 3)
1. Implement W (Zap!) - linear skillshot
2. Implement slow mechanic
3. Implement reveal mechanic

### Phase 3: Area Control (Week 4)
1. Implement E (Flame Chompers!) - trap placement
2. Implement proximity detection
3. Implement trap destruction

### Phase 4: Ultimate (Week 5)
1. Implement R (Super Mega Death Rocket!) - global ultimate
2. Implement acceleration mechanics
3. Implement missing health scaling
4. Implement execute detection

### Phase 5: Polish (Week 6)
1. Animations
2. Visual effects
3. Sound effects
4. Balance tuning

---

## 9. TESTING CHECKLIST

### Passive: Get Excited!
- [ ] Triggers on champion kill
- [ ] Triggers on structure destroy
- [ ] Triggers on epic monster kill
- [ ] Movement speed bonus applies
- [ ] Attack speed bonus applies
- [ ] Duration is 6 seconds
- [ ] Buff decays properly
- [ ] Doesn't trigger on minion kills

### Q: Switcheroo!
- [ ] Toggle between minigun and rocket
- [ ] Minigun attacks deal 100% AD
- [ ] Rocket attacks deal 110% AD
- [ ] Rocket attacks cost 20 mana
- [ ] Minigun stacks attack speed (max 3)
- [ ] Stacks decay after 2.5 seconds
- [ ] Attack speed penalty applies (10% less)
- [ ] Weapon swap animation plays
- [ ] Toggle can be interrupted

### W: Zap!
- [ ] Fires in target direction
- [ ] Range is 1500 units
- [ ] Hits first enemy only
- [ ] Applies slow (40-80% based on level)
- [ ] Slow lasts 2 seconds
- [ ] Reveals target for 2 seconds
- [ ] Damage scales with AD (1.4x)
- [ ] Cooldown decreases per level
- [ ] Mana cost increases per level

### E: Flame Chompers!
- [ ] Places 3 chompers
- [ ] Chompers arm after 0.5 seconds
- [ ] Chompers last 5 seconds
- [ ] Detonates on champion contact
- [ ] Applies root (1.5 seconds)
- [ ] Damage scales with AP (100%)
- [ ] Nearby enemies take damage
- [ ] Provides vision
- [ ] Cooldown decreases per level

### R: Super Mega Death Rocket!
- [ ] Fires globally
- [ ] Projectile accelerates over 1 second
- [ ] Damage increases with distance
- [ ] Scales with bonus AD (1.2x)
- [ ] Missing health scaling applies
- [ ] Execute bonus applies
- [ ] Nearby enemies take 80% damage
- [ ] Cooldown decreases per level
- [ ] Mana cost is 100

---

## 10. BALANCE CONSIDERATIONS

### Early Game (Levels 1-6)
- Weak early game (low base stats)
- Relies on minigun stacking for DPS
- Zap provides utility and poke
- Flame Chompers provides control

### Mid Game (Levels 7-11)
- Comes online with items
- Rocket launcher provides range safety
- Passive enables teamfight resets
- Ultimate provides global pressure

### Late Game (Levels 12-18)
- Scales well with attack damage
- Rocket launcher enables kiting
- Passive enables cleanup kills
- Ultimate provides execute potential

### Counterplay
- Melee champions struggle against rocket range
- Crowd control vulnerable to roots
- Mana-gated (rocket attacks cost mana)
- Passive requires kills (not guaranteed)

---

## 11. CONCLUSION

Jinx is an excellent reference champion for Arena Engine implementation due to:

1. **Diverse Ability Kit** - Covers multiple ability types (toggle, skillshot, trap, ultimate)
2. **Moderate Complexity** - Challenging but achievable implementation
3. **Clear Mechanics** - Well-defined interactions and effects
4. **Scalability** - Works well at all game stages
5. **Visual Appeal** - Distinctive design and effects

This reference design provides sufficient detail for implementation while allowing flexibility in specific mechanics and balance tuning.

