# Jinx - Visual & Animation Reference

**Complete visual design guide for Arena Engine implementation**

---

## CHARACTER DESIGN

### Appearance
- **Hair:** Blue and pink streaks, wild and energetic
- **Clothing:** Dark tactical outfit with neon accents
- **Build:** Slim, athletic, energetic posture
- **Personality:** Chaotic, excited, unpredictable

### Color Palette
```
Primary Blue:    #0099FF (RGB: 0, 153, 255)
Primary Pink:    #FF00FF (RGB: 255, 0, 255)
Accent Blue:     #00CCFF (RGB: 0, 204, 255)
Accent Pink:     #FF33FF (RGB: 255, 51, 255)
Dark Base:       #1A1A2E (RGB: 26, 26, 46)
```

---

## WEAPON DESIGNS

### Pow-Pow (Minigun)
```
Style:           Compact, rapid-fire minigun
Size:            Medium (one-handed)
Color Scheme:    Metallic gray + blue accents
Barrel:          Spinning, 6-barrel design
Effects:         Blue electrical arcs, spinning glow
Ammo:            Visible ammo counter or glowing rounds
Animation:       Barrel spins when firing
```

### Fishbones (Rocket Launcher)
```
Style:           Large, shoulder-mounted launcher
Size:            Large (two-handed)
Color Scheme:    Metallic + pink/magenta accents
Warhead:         Visible rocket in chamber
Effects:         Pink energy core, rocket trail
Ammo:            Visible rocket loading
Animation:       Recoil on fire, rocket ejects
```

---

## ANIMATION SPECIFICATIONS

### Idle Animations (Loop)
```
Minigun Idle:
  - Duration: 2-3 seconds
  - Description: Jinx holds minigun, twitching/fidgeting
  - Keyframes: Weapon bounce, head tilt, foot tap
  - Loop: Yes

Rocket Idle:
  - Duration: 2-3 seconds
  - Description: Jinx holds rocket launcher, looking around
  - Keyframes: Weapon shift, shoulder shrug, look around
  - Loop: Yes

Passive Active Idle:
  - Duration: 1-2 seconds
  - Description: Excited animation with energy effects
  - Keyframes: Jump, spin, weapon raise
  - Loop: Yes (while passive active)
```

### Movement Animations (Loop)
```
Minigun Walk:
  - Speed: Normal walk speed
  - Description: Bouncy, energetic walk with minigun
  - Keyframes: Hip sway, weapon bounce, arm swing
  - Loop: Yes

Rocket Walk:
  - Speed: Normal walk speed
  - Description: Heavier walk with rocket launcher
  - Keyframes: Wider stance, weapon stabilization
  - Loop: Yes

Passive Active Run:
  - Speed: Fast run (175% movement speed)
  - Description: Fast run with speed trail effects
  - Keyframes: Sprinting motion, weapon held back
  - Loop: Yes
```

### Attack Animations
```
Minigun Auto Attack:
  - Duration: 0.4 seconds
  - Description: Rapid fire animation, 3-frame attack
  - Keyframes: Aim, fire (3 bursts), recoil
  - Cancellable: Yes (by movement)
  - Loop: No

Rocket Auto Attack:
  - Duration: 0.6 seconds
  - Description: Slower, heavier attack with recoil
  - Keyframes: Aim, fire, heavy recoil, recover
  - Cancellable: No
  - Loop: No

Attack Speed Stacking:
  - Effect: Animation speed increases with stacks
  - Stack 1: 100% speed
  - Stack 2: 150% speed
  - Stack 3: 200% speed
```

### Ability Animations
```
Q: Weapon Swap
  - Duration: 0.9 seconds
  - Minigun → Rocket: Holster minigun, draw rocket
  - Rocket → Minigun: Holster rocket, draw minigun
  - Interruptible: Yes (by crowd control)
  - Visual: Smooth transition, weapon glow

W: Zap! Cast
  - Duration: 0.3 seconds
  - Description: Aim and fire electrical beam
  - Keyframes: Aim, fire, recoil
  - Projectile: Blue electrical beam
  - Impact: Spark burst, electrical explosion

E: Flame Chompers! Cast
  - Duration: 0.3 seconds
  - Description: Throw/place chompers
  - Keyframes: Wind-up, throw, release
  - Projectile: 3 chompers arc through air
  - Impact: Chompers land and arm

R: Super Mega Death Rocket! Cast
  - Duration: 0.6 seconds
  - Description: Channel and fire rocket
  - Keyframes: Aim, charge, fire, recoil
  - Projectile: Large rocket with trail
  - Impact: Large explosion with shockwave
```

### Special Animations
```
Passive Trigger:
  - Duration: 0.5 seconds
  - Description: Excitement animation
  - Keyframes: Jump, spin, weapon raise
  - Loop: No

Death Animation:
  - Duration: 1.5 seconds
  - Description: Explosion/destruction animation
  - Keyframes: Stagger, fall, explosion
  - Loop: No

Respawn Animation:
  - Duration: 1.0 seconds
  - Description: Spawn-in animation
  - Keyframes: Fade in, land, ready stance
  - Loop: No

Recall Animation:
  - Duration: 3.0 seconds
  - Description: Teleport/recall animation
  - Keyframes: Glow, spin, fade out
  - Loop: No

Hit Reaction:
  - Duration: 0.3 seconds
  - Description: Knockback/flinch animation
  - Keyframes: Stagger back, recover
  - Loop: No

Root Animation (Flame Chompers):
  - Duration: 1.5 seconds
  - Description: Rooted in place animation
  - Keyframes: Struggle, chains appear, struggle
  - Loop: Yes (while rooted)
```

---

## PARTICLE EFFECTS

### Weapon Effects
```
Minigun Attack:
  - Color: Blue (#0099FF)
  - Type: Muzzle flash, shell casings
  - Frequency: Every attack
  - Duration: 0.2 seconds

Rocket Attack:
  - Color: Pink (#FF00FF)
  - Type: Muzzle flash, smoke trail
  - Frequency: Every attack
  - Duration: 0.3 seconds
```

### Ability Effects
```
Passive (Get Excited!):
  - Color: Blue/Pink gradient
  - Type: Particle trail around character
  - Frequency: Continuous while active
  - Duration: 6 seconds

Q (Switcheroo!):
  - Minigun Mode: Blue electrical aura
  - Rocket Mode: Pink energy glow
  - Stacks: Glow intensity increases (1-3)
  - Duration: While weapon active

W (Zap!):
  - Projectile: Blue electrical beam
  - Impact: Spark burst, electrical explosion
  - Slow Effect: Blue electrical chains on target
  - Reveal: Target glows with blue outline
  - Duration: 2 seconds (slow/reveal)

E (Flame Chompers!):
  - Placement: Chomper sprites appear
  - Idle: Snapping animation, waiting
  - Trigger: Explosion with fire/electrical particles
  - Root Effect: Blue chains binding target
  - Duration: 5 seconds (on ground)

R (Super Mega Death Rocket!):
  - Projectile: Large rocket with pink/blue trail
  - Travel: Accelerating particle trail
  - Impact: Large explosion with shockwave
  - Nearby Damage: Secondary explosion rings
  - Execute: Enhanced explosion effect
  - Duration: Until impact
```

---

## VISUAL INDICATORS

### UI Elements
```
Weapon Mode Indicator:
  - Location: Bottom-left of screen
  - Shows: Current weapon (Minigun/Rocket)
  - Color: Blue (minigun) or Pink (rocket)
  - Update: Real-time

Attack Speed Stacks:
  - Location: Above character
  - Shows: 1-3 stacks
  - Color: Blue
  - Update: On attack

Passive Buff Indicator:
  - Location: Buff bar
  - Shows: Get Excited! active
  - Color: Blue/Pink
  - Duration: 6 seconds

Ability Cooldowns:
  - Location: Ability bar
  - Shows: Cooldown remaining
  - Color: Gray (on cooldown) or Ready (ready)
  - Update: Real-time

Mana Cost Indicator:
  - Location: Rocket attack indicator
  - Shows: 20 mana per rocket
  - Color: Blue
  - Update: On rocket attack
```

---

## ANIMATION TIMING REFERENCE

| Animation | Duration | Notes |
|-----------|----------|-------|
| Minigun Attack | 0.4s | Can be cancelled |
| Rocket Attack | 0.6s | Longer wind-up |
| Q Toggle | 0.9s | Can be interrupted |
| W Cast | 0.3s | Instant projectile |
| E Cast | 0.3s | Instant placement |
| R Cast | 0.6s | Channel ability |
| Passive Trigger | 0.5s | Brief excitement |
| Death | 1.5s | Explosion |
| Respawn | 1.0s | Spawn-in |
| Recall | 3.0s | Teleport |

---

## VISUAL HIERARCHY

### Priority 1 (Always Visible)
- Character model
- Weapon (current)
- Health bar
- Buff indicators

### Priority 2 (Combat)
- Attack animations
- Ability projectiles
- Damage numbers
- Crowd control effects

### Priority 3 (Environmental)
- Traps (Flame Chompers)
- Particle trails
- Explosion effects
- Reveal indicators

### Priority 4 (UI)
- Cooldown indicators
- Mana cost display
- Weapon mode indicator
- Stack counters

---

## SOUND DESIGN NOTES

### Voice Lines
- Idle: Excited, chaotic personality
- Attack: Aggressive, energetic
- Ability Cast: Unique for each ability
- Kill: Excited, celebratory
- Death: Surprised, dramatic

### Sound Effects
- Minigun: Rapid fire sound
- Rocket: Heavy launch sound
- Zap: Electrical discharge
- Chompers: Snapping, explosion
- Rocket: Whoosh, explosion

---

## SKIN VARIATIONS

### Classic Jinx
- Base design as described above

### Potential Skins
- **Arcane Jinx:** Arcane universe design
- **PROJECT: Jinx:** Cyberpunk design
- **Star Guardian Jinx:** Magical girl design
- **Crime City Jinx:** Noir design

---

## IMPLEMENTATION NOTES

1. All animations should be smooth and responsive
2. Weapon swap should feel snappy (0.9s is quick)
3. Particle effects should not obscure gameplay
4. Color coding helps player readability
5. Animation speed increases with attack speed stacks
6. Passive trigger should be visually obvious
7. Crowd control effects should be clear
8. Ability impacts should feel impactful

---

## REFERENCE IMAGES

### Official Splash Art (Classic)

![Jinx Splash Art](https://ddragon.leagueoflegends.com/cdn/img/champion/splash/Jinx_0.jpg)

*Classic Jinx splash art - Full character design with Pow-Pow (minigun) and Fishbones (rocket launcher)*

### Champion Select / Loading Screen

![Jinx Loading Screen](https://ddragon.leagueoflegends.com/cdn/img/champion/loading/Jinx_0.jpg)

*Loading screen portrait - Used in champion select and game loading*

### In-Game Model Reference

![Jinx Centered Portrait](https://ddragon.leagueoflegends.com/cdn/img/champion/centered/Jinx_0.jpg)

*Centered splash crop - Used for in-game shop and collection UI*

### Ability Icons

| Ability | Icon | Name |
|---------|------|------|
| Passive | ![Get Excited!](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/passive/Jinx_Passive.png) | Get Excited! |
| Q | ![Switcheroo!](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/spell/JinxQ.png) | Switcheroo! |
| W | ![Zap!](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/spell/JinxW.png) | Zap! |
| E | ![Flame Chompers!](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/spell/JinxE.png) | Flame Chompers! |
| R | ![Super Mega Death Rocket!](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/spell/JinxR.png) | Super Mega Death Rocket! |

### Champion Square Icon

![Jinx Square](https://ddragon.leagueoflegends.com/cdn/14.24.1/img/champion/Jinx.png)

*Square icon used in scoreboard, minimap, and HUD*

### Skin Variants (Reference)

| Skin | Splash Art |
|------|------------|
| Crime City Jinx | ![Crime City](https://ddragon.leagueoflegends.com/cdn/img/champion/splash/Jinx_1.jpg) |
| Star Guardian Jinx | ![Star Guardian](https://ddragon.leagueoflegends.com/cdn/img/champion/splash/Jinx_4.jpg) |
| PROJECT: Jinx | ![PROJECT](https://ddragon.leagueoflegends.com/cdn/img/champion/splash/Jinx_20.jpg) |
| Arcane Jinx | ![Arcane](https://ddragon.leagueoflegends.com/cdn/img/champion/splash/Jinx_37.jpg) |

### Video References

Ability preview videos are available at:
- **Passive:** [Get Excited! Video](https://lol.dyn.riotcdn.net/x/videos/champion-abilities/0222/ability_0222_P1.webm)
- **Q:** [Switcheroo! Video](https://lol.dyn.riotcdn.net/x/videos/champion-abilities/0222/ability_0222_Q1.webm)
- **W:** [Zap! Video](https://lol.dyn.riotcdn.net/x/videos/champion-abilities/0222/ability_0222_W1.webm)
- **E:** [Flame Chompers! Video](https://lol.dyn.riotcdn.net/x/videos/champion-abilities/0222/ability_0222_E1.webm)
- **R:** [Super Mega Death Rocket! Video](https://lol.dyn.riotcdn.net/x/videos/champion-abilities/0222/ability_0222_R1.webm)

### Additional Resources

- [Official Champion Page](https://www.leagueoflegends.com/en-us/champions/jinx/)
- [OP.GG Jinx Builds](https://op.gg/lol/champions/jinx/build)
- [U.GG Jinx Builds](https://u.gg/lol/champions/jinx/build)
- Arcane Netflix series (Jinx character design and backstory)


