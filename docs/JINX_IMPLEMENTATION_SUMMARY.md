# Jinx Implementation Summary

**Quick Reference for Arena Engine Development**

---

## CHAMPION OVERVIEW

**Name:** Jinx, The Loose Cannon  
**Role:** ADC (Attack Damage Carry)  
**Playstyle:** High-risk, high-reward damage dealer with weapon switching and area control

---

## KEY STATS AT LEVEL 1

| Stat | Value |
|------|-------|
| Health | 590 |
| Mana | 260 |
| Attack Damage | 59 |
| Attack Speed | 0.625 |
| Movement Speed | 330 |
| Armor | 32 |
| Magic Resist | 30 |

---

## ABILITY QUICK REFERENCE

### Passive: Get Excited!
- **Trigger:** Kill/destroy within 3 seconds of damage
- **Effect:** +175% movement speed (decaying), +25% attack speed
- **Duration:** 6 seconds
- **Complexity:** SIMPLE

### Q: Switcheroo!
- **Type:** Toggle (no cooldown)
- **Minigun:** 525 range, 100% AD, stacks attack speed (max 3)
- **Rocket:** 625 range, 110% AD, 20 mana per shot, splash damage
- **Complexity:** MEDIUM

### W: Zap!
- **Type:** Linear skillshot
- **Range:** 1500 units
- **Damage:** 10 + 1.4 AD
- **Effect:** 40-80% slow, 2-second reveal
- **Cooldown:** 8-4 seconds
- **Complexity:** MEDIUM

### E: Flame Chompers!
- **Type:** Trap placement (3 chompers)
- **Range:** 925 units
- **Damage:** 90 + 1.0 AP
- **Effect:** 1.5-second root, area damage
- **Cooldown:** 24-10 seconds
- **Complexity:** MEDIUM-COMPLEX

### R: Super Mega Death Rocket!
- **Type:** Global ultimate
- **Range:** Global
- **Damage:** 32.5-200 + 0.165-1.2 bonus AD + 25-35% missing health
- **Effect:** Accelerating projectile, execute bonus
- **Cooldown:** 85-45 seconds
- **Complexity:** COMPLEX

---

## IMPLEMENTATION PRIORITY

### Phase 1 (Week 1-2): Core Systems
1. Weapon toggle system (Q)
2. Kill tracking (Passive)
3. Attack speed stacking
4. **Deliverable:** Functional weapon switching with passive

### Phase 2 (Week 3): Skillshots
1. Linear projectile system (W)
2. Slow mechanic
3. Reveal mechanic
4. **Deliverable:** Functional Zap ability

### Phase 3 (Week 4): Area Control
1. Trap placement system (E)
2. Proximity detection
3. Area damage
4. **Deliverable:** Functional Flame Chompers

### Phase 4 (Week 5): Ultimate
1. Global projectile system (R)
2. Acceleration mechanics
3. Missing health scaling
4. Execute detection
5. **Deliverable:** Functional Super Mega Death Rocket

### Phase 5 (Week 6): Polish
1. Animations
2. Visual effects
3. Sound effects
4. Balance tuning
5. **Deliverable:** Complete, polished champion

---

## REQUIRED SYSTEMS

### Must Implement
- [ ] Dual weapon attack system
- [ ] Attack speed stacking buff
- [ ] Linear projectile system
- [ ] Crowd control (slow, root)
- [ ] Trap/minion system
- [ ] Global projectile system
- [ ] Acceleration physics
- [ ] Missing health calculation
- [ ] Execute detection
- [ ] Area damage with falloff
- [ ] Reveal mechanic
- [ ] Decaying buff system

### Already Exists (Reuse)
- [x] ECS framework
- [x] Health/damage system
- [x] Cooldown system
- [x] Mana system
- [x] Transform/position system
- [x] Event system

---

## COMPLEXITY BREAKDOWN

| Component | Complexity | Est. LOC | Dependencies |
|-----------|-----------|---------|--------------|
| Passive | SIMPLE | 150-200 | Event system, Buff system |
| Q | MEDIUM | 300-400 | Attack system, Buff system |
| W | MEDIUM | 250-350 | Projectile system, CC system |
| E | MEDIUM-COMPLEX | 400-500 | Entity spawning, Proximity detection |
| R | COMPLEX | 500-700 | Global projectile, Physics, Damage calc |
| **Total** | **MEDIUM** | **1600-2150** | **7 systems** |

---

## STAT SCALING EXAMPLES

### Level 1 vs Level 18
| Stat | Level 1 | Level 18 | Growth |
|------|---------|----------|--------|
| Health | 590 | 2,205 | +1,615 |
| Attack Damage | 59 | 96.4 | +37.4 |
| Armor | 32 | 100 | +68 |
| Attack Speed | 0.625 | 0.848 | +35.7% |

---

## DAMAGE CALCULATIONS

### Zap! (Level 5, 300 AD)
```
Base: 210
AD Scaling: 300 * 1.4 = 420
Total: 630 damage
```

### Flame Chompers! (Level 5, 100 AP)
```
Base: 290
AP Scaling: 100 * 1.0 = 100
Total: 390 damage
```

### Super Mega Death Rocket! (Level 3, 300 AD, 50% missing health)
```
Min: 62.5 + (300 * 0.165) = 112.5
Max: 500 + (300 * 1.2) = 860
Missing Health: 50% * 0.35 = 17.5%
Total Range: 112.5 - 877.5 damage
```

---

## VISUAL DESIGN

### Color Scheme
- **Primary:** Blue and Pink/Magenta
- **Minigun:** Blue electrical effects
- **Rocket:** Pink energy effects
- **Passive:** Blue/pink particle trail

### Weapon Appearances
- **Pow-Pow:** Compact minigun with blue accents
- **Fishbones:** Large rocket launcher with pink accents

---

## TESTING CHECKLIST

### Critical Tests
- [ ] Weapon toggle works correctly
- [ ] Attack speed stacks properly (max 3)
- [ ] Passive triggers on kills only
- [ ] Zap hits first enemy only
- [ ] Chompers detonate on champion contact
- [ ] Rocket accelerates over 1 second
- [ ] Missing health scaling works
- [ ] Execute bonus applies correctly
- [ ] Area damage applies to nearby enemies
- [ ] All cooldowns decrease per level

---

## BALANCE NOTES

### Strengths
- High damage output with items
- Long range (rocket mode)
- Area control (traps)
- Global ultimate
- Passive enables cleanup kills

### Weaknesses
- Weak early game
- Mana-gated (rocket attacks)
- Vulnerable to crowd control
- Passive requires kills
- Melee champions can close gap

---

## REFERENCE DOCUMENTS

1. **JINX_REFERENCE_DESIGN.md** - Complete detailed specification
2. **JINX_IMPLEMENTATION_SUMMARY.md** - This quick reference
3. **Code examples in appendices** - Implementation pseudocode

---

## NEXT STEPS

1. Review JINX_REFERENCE_DESIGN.md for complete specifications
2. Identify which systems need to be built vs. reused
3. Create implementation tasks for each phase
4. Begin Phase 1: Core mechanics (weapon toggle + passive)
5. Iterate through phases with testing at each stage

---

## CONTACT & NOTES

This design is based on League of Legends Patch 26.5 (2026-03-08).  
All mechanics are documented with sufficient detail for implementation.  
Balance tuning may be required based on playtesting.

For questions or clarifications, refer to the detailed specification document.

