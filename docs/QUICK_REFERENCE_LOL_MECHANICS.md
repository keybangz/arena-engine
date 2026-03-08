# League of Legends Mechanics - Quick Reference Card
**For Arena Engine Implementation**

---

## MOVEMENT SPEED

| Stat | Value |
|------|-------|
| Slowest Champion | 305 MS (Kled) |
| Fastest Champion | 355 MS (Master Yi) |
| Average | 330-340 MS |
| Boots of Speed | +45 MS |
| Tier 2 Boots | +60 MS |
| Minimum Speed | 20 MS (hard cap) |

---

## CROWD CONTROL EFFECTS

### Slow
- **Effect:** Reduces movement speed by %
- **Duration:** 1-3 seconds
- **Affected by Tenacity:** YES
- **Blocks Movement:** Partially (reduced)
- **Blocks Abilities:** NO
- **Blocks Attacks:** NO

### Root
- **Effect:** Prevents movement, allows mobility abilities
- **Duration:** 1-2 seconds
- **Affected by Tenacity:** YES
- **Blocks Movement:** YES
- **Blocks Abilities:** Mobility only
- **Blocks Attacks:** NO

### Stun
- **Effect:** Prevents all actions
- **Duration:** 0.5-2 seconds
- **Affected by Tenacity:** YES
- **Blocks Movement:** YES
- **Blocks Abilities:** YES
- **Blocks Attacks:** YES

### Knockup (Airborne)
- **Effect:** Forced displacement, prevents all actions
- **Duration:** 0.5-1.5 seconds
- **Affected by Tenacity:** **NO**
- **Blocks Movement:** YES
- **Blocks Abilities:** YES
- **Blocks Attacks:** YES

### Knockback
- **Effect:** Forced displacement away from source
- **Duration:** Instant (displacement only)
- **Affected by Tenacity:** **NO**
- **Blocks Movement:** YES (forced)
- **Blocks Abilities:** YES
- **Blocks Attacks:** YES

### Ground
- **Effect:** Prevents mobility spells (dashes, blinks)
- **Duration:** 1-2 seconds
- **Affected by Tenacity:** YES
- **Blocks Movement:** NO (basic movement allowed)
- **Blocks Abilities:** Mobility only
- **Blocks Attacks:** NO

### Cripple
- **Effect:** Reduces attack speed
- **Duration:** 1-3 seconds
- **Affected by Tenacity:** YES
- **Blocks Movement:** NO
- **Blocks Abilities:** NO
- **Blocks Attacks:** NO (reduced speed)

---

## DASH MECHANICS

| Property | Value |
|----------|-------|
| Travel Time | 0.1-0.5 seconds |
| Distance | 300-600 units |
| Speed | ~1700 units/sec |
| Terrain Traversal | Most YES, some NO |
| Interrupted by | Stun, Root, Ground, Silence, Knockdown, Airborne, Stasis |
| Examples | Lucian E, Vayne Q, Lee Sin Q, Sejuani E |

### Dashes that CAN traverse terrain
- Lucian E, Lee Sin Q/R, Ahri R, Tristana W, Sejuani E, Riven Q/E, Vi Q, Yasuo E

### Dashes that CANNOT traverse terrain
- Vayne Q, Galio Q, Kled Q (first cast), Riven Q (first 2 casts)

---

## BLINK MECHANICS

| Property | Value |
|----------|-------|
| Travel Time | 0 seconds (instant) |
| Distance | Unlimited |
| Terrain Traversal | **Always** |
| Interrupted by | Cannot be interrupted |
| Examples | Flash, Ezreal E, Shaco R, LeBlanc W |

---

## KNOCKBACK/PULL MECHANICS

| Property | Value |
|----------|-------|
| Travel Time | Instant |
| Distance | 300-600 units |
| Affected by Tenacity | **NO** |
| Applies Airborne | YES |
| Interrupted by | Displacement immunity |
| Examples | Lee Sin R, Blitzcrank Q, Alistar Q |

---

## DISPLACEMENT IMMUNITY

**Grants immunity to:**
- Knockup (airborne)
- Knockback
- Pull
- Stasis
- Suspension
- Kinematics
- Sleep

**Does NOT grant immunity to:**
- Root
- Stun
- Slow
- Ground
- Silence

**Examples:**
- Malphite R, Sion R, Olaf R, K'Sante W, Kalista R

---

## TENACITY SYSTEM

**Formula:**
```
effective_duration = base_duration / (1 + tenacity)
```

**Example:**
- 2 second stun with 30% tenacity = 2 / 1.3 = 1.54 seconds

**Does NOT affect:**
- Airborne (knockup, knockback, pull)
- Knockdown
- Nearsight
- Stasis
- Suppression

---

## CAMERA SYSTEM

| Property | Value |
|----------|-------|
| Perspective | Isometric |
| Pitch Angle | 55-60 degrees |
| Yaw Angle | 0 degrees (fixed) |
| Distance | 800-1000 units |
| FOV | 45-50 degrees |
| Aspect Ratio | 16:9 widescreen |

---

## TERRAIN HEIGHTS

| Terrain | Height | Gameplay Impact |
|---------|--------|---|
| Base Terrain | 0 units | None |
| River | -10 units | Visual only |
| Jungle | +5 units | Visual only |
| Walls | 0 units | Collision only |

**Key:** Height does NOT affect:
- Skillshot blocking
- Vision
- Collision (2D only)

---

## VISION SYSTEM

| Property | Value |
|----------|-------|
| Default Vision Radius | ~1200 units |
| Ward Vision | ~1100 units |
| Brush Effect | Grants stealth |
| Fog of War | Per-team visibility |
| Height Impact | None |

---

## PATHFINDING

| Property | Value |
|----------|-------|
| Algorithm | A* on NavMesh |
| Dynamic Obstacles | Minions, Champions |
| Recalculation | When obstacles move |
| Path Smoothing | Yes |
| Collision Avoidance | Yes |

---

## COLLISION LAYERS

```
TERRAIN      - Walls, map boundaries
CHAMPION     - Player champions
MINION       - Minions
PROJECTILE   - Skillshots
STRUCTURE    - Towers, inhibitors
TERRAIN_WALL - Champion-created walls
```

---

## IMPLEMENTATION CHECKLIST

### Phase 1 (MVP)
- [ ] Movement speed 305-355 range
- [ ] Instant velocity changes
- [ ] Click-to-move pathfinding
- [ ] Slow/Root/Stun effects
- [ ] Basic dash abilities
- [ ] Minion collision

### Phase 2 (Polish)
- [ ] Knockup/Knockback mechanics
- [ ] Displacement immunity
- [ ] Dash terrain interaction
- [ ] Tenacity system
- [ ] Ground/Cripple effects
- [ ] Advanced pathfinding

### Phase 3 (Advanced)
- [ ] Blink abilities
- [ ] 3D terrain heights
- [ ] Brush/fog of war
- [ ] Vision system
- [ ] Champion-created terrain
- [ ] Advanced ability interactions

---

## COMMON MISTAKES TO AVOID

1. ❌ Acceleration/deceleration (should be instant)
2. ❌ Smooth turning (should be instant)
3. ❌ Knockup affected by tenacity (should NOT be)
4. ❌ Dashes always traverse terrain (some cannot)
5. ❌ Height affects vision (should NOT)
6. ❌ Collision is 3D (should be 2D only)
7. ❌ Tenacity stacks additively (should be multiplicative)

---

## USEFUL FORMULAS

**Effective Movement Speed:**
```
effective_ms = base_ms * item_multiplier * buff_multiplier
effective_ms = max(effective_ms * (1 - slow_percent), 20)
```

**Dash Duration:**
```
duration = distance / speed
Example: 425 units / 1700 units/sec = 0.25 seconds
```

**Tenacity Reduction:**
```
effective_duration = base_duration / (1 + tenacity)
Example: 2 sec stun with 30% tenacity = 1.54 seconds
```

**Bilinear Height Interpolation:**
```
h = h00*(1-fx)*(1-fy) + h10*fx*(1-fy) + h01*(1-fx)*fy + h11*fx*fy
```

---

## REFERENCES

- **Comprehensive:** LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md
- **Practical:** MOVEMENT_IMPLEMENTATION_GUIDE.md
- **Technical:** 3D_CAMERA_TERRAIN_IMPLEMENTATION.md
- **Summary:** RESEARCH_SUMMARY.md

