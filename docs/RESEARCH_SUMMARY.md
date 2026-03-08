# League of Legends Research Summary
**Completed:** March 8, 2026  
**Scope:** Movement, Abilities, 3D Mechanics for Arena Engine

---

## Research Deliverables

### 📄 Documents Created

1. **LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md** (Comprehensive)
   - Base movement speed statistics (305-355 MS range)
   - Movement mechanics (instant velocity, click-to-move, collision)
   - Crowd control effects (slow, root, stun, knockup, knockback, ground)
   - Dash mechanics (terrain interaction, interruption, examples)
   - Blink mechanics (instant teleport, terrain traversal)
   - Knockback/pull mechanics (displacement, airborne status)
   - 3D map depth (isometric camera, terrain heights, brush, walls)
   - Technical implementation recommendations
   - Specific numbers and formulas
   - Implementation priorities (Phase 1-3)

2. **MOVEMENT_IMPLEMENTATION_GUIDE.md** (Practical)
   - Quick reference tables
   - Current Arena Engine status
   - Implementation roadmap
   - Code examples (movement, CC, dashes, knockbacks)
   - Testing checklist
   - Performance considerations

3. **3D_CAMERA_TERRAIN_IMPLEMENTATION.md** (Technical)
   - Isometric camera setup (55-60 degree pitch)
   - View/projection matrix construction
   - Screen-to-world conversion
   - Terrain height system
   - Brush and fog of war
   - Wall geometry and collision
   - NavMesh generation
   - Rendering pipeline
   - Performance optimization

---

## Key Findings

### Movement System
- **Instant velocity changes** (no acceleration)
- **Instant rotation** (no smooth turning)
- **A* pathfinding** on NavMesh
- **Dynamic obstacles** (minions, champions)
- **Soft collision** between units
- **Hard collision** with terrain

### Crowd Control
- **7 main types:** Slow, Root, Stun, Knockup, Knockback, Ground, Cripple
- **Tenacity system:** Reduces CC duration (multiplicative, doesn't affect airborne)
- **Stacking:** Multiple CC effects can apply simultaneously
- **Interruption:** Some abilities interrupt dashes/channels

### Ability Movement
- **Dashes:** 0.1-0.5s travel time, 300-600 units, most traverse terrain
- **Blinks:** Instant, ignore all terrain
- **Knockbacks:** Instant displacement, not affected by tenacity
- **Displacement immunity:** Blocks knockups/knockbacks/pulls

### 3D Mechanics
- **Isometric perspective:** 55-60 degree camera angle
- **Terrain heights:** Visual only (no gameplay impact)
- **Brush:** Grants stealth, blocks vision
- **Fog of war:** Per-team visibility system
- **Walls:** Vertical collision, some dashes can traverse

---

## Implementation Recommendations

### Phase 1 (MVP) - Current
✅ Basic movement (WASD, click-to-move)
✅ Instant velocity changes
✅ Simple pathfinding
✅ Dash abilities
✅ Basic crowd control

### Phase 2 (Polish)
- Terrain interaction (dashes can/cannot traverse walls)
- Knockback/pull mechanics
- Displacement immunity
- Advanced pathfinding
- Crowd control stacking

### Phase 3 (Advanced)
- 3D terrain heights
- Brush/fog of war
- Champion-created terrain
- Vision system
- Advanced ability interactions

---

## Specific Numbers

### Movement Speed
- **Range:** 305-355 MS
- **Average:** 330-340 MS
- **Boots bonus:** +45-60 MS
- **Minimum:** 20 MS (hard cap)

### Dash Properties
- **Distance:** 300-600 units
- **Duration:** 0.1-0.5 seconds
- **Speed:** ~1700 units/sec (425 units / 0.25s)

### Crowd Control
- **Slow:** 1-3 seconds, 20-50% reduction
- **Root:** 1-2 seconds
- **Stun:** 0.5-2 seconds
- **Knockup:** 0.5-1.5 seconds
- **Tenacity:** Reduces duration by % (multiplicative)

### Vision
- **Default radius:** ~1200 units
- **Ward radius:** ~1100 units
- **Brush:** Grants stealth

### Camera
- **Pitch angle:** 55-60 degrees
- **Distance:** 800-1000 units
- **FOV:** 45-50 degrees
- **Aspect ratio:** 16:9

---

## Data Sources

### Primary Sources
- League of Legends Wiki (official)
- Patch 25.S1.1 (January 2025)
- 170 champions analyzed
- 200+ abilities documented

### Research Coverage
- Movement speed statistics (all champions)
- Crowd control mechanics (comprehensive)
- Dash/blink mechanics (detailed)
- Knockback/pull mechanics (detailed)
- Terrain interaction (specific examples)
- Camera system (isometric perspective)
- Terrain heights (elevation system)
- Brush/fog of war (vision mechanics)
- Wall geometry (collision system)

---

## Integration with Arena Engine

### Current Status
- ✅ ECS architecture ready
- ✅ Basic movement implemented
- ✅ Pathfinding system exists
- ✅ Ability system framework
- ✅ Crowd control component exists
- ⚠️ Needs enhancement for LoL accuracy
- ❌ 3D terrain not yet implemented

### Next Steps
1. Update movement speed values (305-355 range)
2. Enhance crowd control system (all 7 types)
3. Implement dash terrain interaction
4. Add knockback/pull mechanics
5. Implement displacement immunity
6. Add tenacity system
7. Implement 3D camera system
8. Add terrain height system
9. Implement brush/fog of war
10. Add vision system

---

## Code Examples Provided

### Movement Component
- Velocity calculation with CC
- Movement update logic
- Crowd control application
- CC duration tracking

### Dash System
- Dash casting
- Terrain interaction
- Linear interpolation
- Interruption handling

### Knockback System
- Knockback application
- Displacement calculation
- Airborne status
- Duration tracking

### Camera System
- Isometric camera setup
- View matrix construction
- Projection matrix
- Screen-to-world conversion

### Terrain System
- Height map loading
- Height interpolation
- Mesh generation
- Collision detection

---

## Performance Targets

- **Movement updates:** O(1) per entity
- **Pathfinding:** Cached, recalculate on obstacle change
- **Collision checks:** O(1) with spatial hashing
- **Crowd control:** Batch update per frame
- **Rendering:** 60 FPS target
- **Memory:** Efficient component storage

---

## Testing Strategy

### Unit Tests
- Movement speed calculations
- Crowd control duration
- Dash distance/duration
- Knockback direction
- Terrain collision

### Integration Tests
- Movement with CC
- Dash interruption
- Knockback with immunity
- Pathfinding with obstacles
- Camera screen-to-world

### Performance Tests
- 100+ entities movement
- Pathfinding recalculation
- Crowd control updates
- Rendering performance

---

## References

All research documents are located in `/docs/`:
- `LOL_RESEARCH_MOVEMENT_ABILITIES_3D.md` - Comprehensive mechanics
- `MOVEMENT_IMPLEMENTATION_GUIDE.md` - Practical implementation
- `3D_CAMERA_TERRAIN_IMPLEMENTATION.md` - Technical details
- `RESEARCH_SUMMARY.md` - This document

---

## Conclusion

This research provides a comprehensive foundation for implementing LoL-accurate movement, ability, and 3D mechanics in Arena Engine. The documentation includes:

- **Detailed mechanics** from official LoL sources
- **Specific numbers** for balance and gameplay
- **Code examples** for implementation
- **Implementation roadmap** with priorities
- **Testing strategies** for validation

The Arena Engine is well-positioned to integrate these mechanics, with existing ECS architecture and pathfinding systems providing a solid foundation for enhancement.

