# Movement System Implementation Guide for Arena Engine
**Based on League of Legends Research**

---

## Quick Reference: LoL Movement Mechanics

### Movement Speed Ranges
- **Minimum:** 305 MS (Kled)
- **Maximum:** 355 MS (Master Yi)
- **Average:** 330-340 MS (most champions)
- **Typical item bonus:** +45-60 MS (boots)

### Crowd Control Effects
| Type | Blocks Movement | Blocks Abilities | Blocks Attacks | Tenacity Affects |
|------|---|---|---|---|
| Slow | Reduces % | No | No | Yes |
| Root | Yes | Mobility only | No | Yes |
| Stun | Yes | Yes | Yes | Yes |
| Knockup | Yes | Yes | Yes | **No** |
| Knockback | Forced | Yes | Yes | **No** |
| Ground | No (basic) | Mobility only | No | Yes |

### Dash Properties
- **Travel time:** 0.1-0.5 seconds
- **Distance:** 300-600 units
- **Terrain traversal:** Most can, some cannot (Vayne Q, Galio Q cannot)
- **Interruption:** Stun, root, ground, silence, knockdown, airborne, stasis

### Blink Properties
- **Travel time:** 0 seconds (instant)
- **Terrain traversal:** Always (ignores all terrain)
- **Interruption:** Cannot be interrupted (already completed)
- **Examples:** Flash, Ezreal E, Shaco R

---

## Current Arena Engine Status

### ✅ Already Implemented
- Basic WASD movement (200 pixels/sec)
- Instant velocity changes
- Simple collision detection
- Projectile abilities (Fireball)
- Basic dash ability (ABILITY_DASH)
- Minion AI with pathfinding
- Tile-based map system

### ⚠️ Needs Enhancement
- Movement speed values (should be 305-355 range, not 200)
- Crowd control system (slow, root, stun, knockup, knockback)
- Dash terrain interaction (can/cannot traverse walls)
- Knockback/pull mechanics
- Displacement immunity
- Tenacity system
- Advanced pathfinding (dynamic obstacles)

### ❌ Not Yet Implemented
- Blink abilities (instant teleport)
- Brush/fog of war
- Vision system
- Champion-created terrain
- 3D terrain heights
- Advanced ability interactions

---

## Implementation Roadmap

### Phase 1: Core Movement (Current)
**Goal:** Establish LoL-accurate movement foundation

**Tasks:**
1. Update movement speed values (305-355 range)
2. Implement crowd control component
3. Add slow/root/stun effects
4. Create CC update system
5. Test with minions and champions

**Estimated LOC:** 300-400

### Phase 2: Ability Movement
**Goal:** Implement dashes, blinks, knockbacks

**Tasks:**
1. Enhance dash system (terrain interaction)
2. Implement blink abilities
3. Add knockback/pull mechanics
4. Implement displacement immunity
5. Add knockdown interruption

**Estimated LOC:** 400-500

### Phase 3: Advanced Systems
**Goal:** Polish and advanced features

**Tasks:**
1. Implement tenacity system
2. Add brush/fog of war
3. Implement vision system
4. Add champion-created terrain
5. Advanced pathfinding

**Estimated LOC:** 500-700

---

## Code Examples

### Movement Speed Component Update

```c
// In movement_system_update()
void update_movement(World* world, Entity entity, float dt) {
    Transform* t = world_get_transform(world, entity);
    Velocity* v = world_get_velocity(world, entity);
    MovementComponent* mov = world_get_movement(world, entity);
    
    if (!t || !v || !mov) return;
    
    // Calculate effective movement speed
    float base_speed = mov->move_speed;  // 305-355
    float effective_speed = base_speed * mov->speed_multiplier;
    
    // Apply crowd control
    if (mov->is_stunned || mov->is_rooted) {
        effective_speed = 0;
    } else if (mov->slow_percent > 0) {
        effective_speed *= (1.0f - mov->slow_percent);
    }
    
    // Clamp to minimum
    if (effective_speed < 20.0f) effective_speed = 20.0f;
    
    // Update velocity based on movement direction
    if (mov->is_moving && effective_speed > 0) {
        Vec3 dir = vec3_normalize(vec3_sub(mov->target_pos, t->position));
        v->x = dir.x * effective_speed;
        v->y = dir.y * effective_speed;
    } else {
        v->x = 0;
        v->y = 0;
    }
    
    // Apply velocity to position
    t->x += v->x * dt;
    t->y += v->y * dt;
}
```

### Crowd Control Application

```c
void apply_crowd_control(World* world, Entity target, 
                         CrowdControlType type, float duration) {
    MovementComponent* mov = world_get_movement(world, target);
    if (!mov) return;
    
    switch (type) {
        case CC_SLOW:
            mov->slow_percent = 0.3f;  // 30% slow
            mov->cc_duration = duration;
            break;
        case CC_ROOT:
            mov->is_rooted = true;
            mov->cc_duration = duration;
            break;
        case CC_STUN:
            mov->is_stunned = true;
            mov->cc_duration = duration;
            break;
        case CC_KNOCKUP:
            mov->is_stunned = true;  // Can't move/attack/cast
            // Knockup displacement (not affected by tenacity)
            mov->cc_duration = duration;
            break;
    }
}

void update_crowd_control(World* world, Entity entity, float dt) {
    MovementComponent* mov = world_get_movement(world, entity);
    if (!mov || mov->cc_duration <= 0) return;
    
    mov->cc_duration -= dt;
    
    if (mov->cc_duration <= 0) {
        // Remove CC
        mov->is_rooted = false;
        mov->is_stunned = false;
        mov->slow_percent = 0;
    }
}
```

### Dash Implementation

```c
void cast_dash(World* world, Entity caster, Vec3 direction, 
               float distance, float duration, bool can_traverse_terrain) {
    Transform* t = world_get_transform(world, caster);
    MovementComponent* mov = world_get_movement(world, caster);
    
    if (!t || !mov) return;
    
    // Calculate end position
    Vec3 normalized = vec3_normalize(direction);
    Vec3 end_pos = vec3_add(t->position, vec3_scale(normalized, distance));
    
    // Check terrain if needed
    if (!can_traverse_terrain) {
        if (!map_line_of_sight(map, t->x, t->y, end_pos.x, end_pos.y)) {
            // Find wall intersection and stop there
            end_pos = find_wall_intersection(t->position, end_pos);
        }
    }
    
    // Start dash
    mov->is_dashing = true;
    mov->dash_start = t->position;
    mov->dash_end = end_pos;
    mov->dash_duration = duration;
    mov->dash_elapsed = 0;
}

void update_dash(World* world, Entity entity, float dt) {
    Transform* t = world_get_transform(world, entity);
    MovementComponent* mov = world_get_movement(world, entity);
    
    if (!mov || !mov->is_dashing) return;
    
    mov->dash_elapsed += dt;
    float t_progress = mov->dash_elapsed / mov->dash_duration;
    
    if (t_progress >= 1.0f) {
        // Dash complete
        t->x = mov->dash_end.x;
        t->y = mov->dash_end.y;
        mov->is_dashing = false;
    } else {
        // Linear interpolation
        t->x = mov->dash_start.x + (mov->dash_end.x - mov->dash_start.x) * t_progress;
        t->y = mov->dash_start.y + (mov->dash_end.y - mov->dash_start.y) * t_progress;
    }
}
```

### Knockback Implementation

```c
void apply_knockback(World* world, Entity target, Vec3 source_pos, 
                     float distance, float duration) {
    Transform* t = world_get_transform(world, target);
    MovementComponent* mov = world_get_movement(world, target);
    
    if (!t || !mov) return;
    
    // Calculate knockback direction (away from source)
    Vec3 dir = vec3_normalize(vec3_sub(t->position, source_pos));
    Vec3 end_pos = vec3_add(t->position, vec3_scale(dir, distance));
    
    // Apply knockback as forced movement
    mov->is_knockback = true;
    mov->knockback_start = t->position;
    mov->knockback_end = end_pos;
    mov->knockback_duration = duration;
    mov->knockback_elapsed = 0;
    
    // Knockback also applies airborne status
    mov->is_stunned = true;  // Can't move/attack/cast
    mov->cc_duration = duration;
}
```

---

## Testing Checklist

### Movement
- [ ] Champion moves at correct speed (330-340 MS)
- [ ] Movement stops instantly when key released
- [ ] Click-to-move pathfinds around obstacles
- [ ] Minions block movement

### Crowd Control
- [ ] Slow reduces movement speed by correct %
- [ ] Root prevents movement completely
- [ ] Stun prevents movement and abilities
- [ ] Knockup prevents all actions
- [ ] CC durations expire correctly

### Dashes
- [ ] Dash travels correct distance in correct time
- [ ] Dash can be interrupted by stun/root
- [ ] Dash terrain interaction works (can/cannot traverse)
- [ ] Knockdown interrupts dash mid-way

### Knockbacks
- [ ] Knockback moves target away from source
- [ ] Knockback applies airborne status
- [ ] Knockback duration not affected by tenacity
- [ ] Displacement immunity blocks knockback

---

## Performance Considerations

1. **Pathfinding:** Cache paths, only recalculate when obstacles change
2. **Collision checks:** Use spatial hashing for O(1) lookups
3. **CC updates:** Batch update all CC effects per frame
4. **Dash interpolation:** Use simple linear interpolation (fast)
5. **Vision system:** Implement later (not critical for MVP)

---

## References

- **LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md** - Detailed LoL mechanics
- **TECHNICAL_SPEC.md** - Arena Engine architecture
- **MILESTONES.md** - Project roadmap

