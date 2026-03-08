# Arena Engine 3D Transition Implementation Proposal

**Version:** 0.8.0-DRAFT
**Date:** 2026-03-08
**Authors:** Arena Engine Team
**Status:** PROPOSAL

---

## Executive Summary

This proposal outlines the transition of Arena Engine from 2D quad-based rendering to full 3D mesh rendering while maintaining compatibility with existing gameplay systems. The transition targets three milestones (v0.8.0, v0.9.0, v0.10.0) over 6-9 weeks, culminating in a visually polished v1.0.0 release.

### Key Decisions
- **Rendering:** Forward rendering (simpler implementation, sufficient for MOBA scene complexity)
- **Asset Format:** glTF 2.0 via cgltf (industry standard, excellent tool support)
- **Migration Strategy:** Parallel pipelines (2D UI + 3D world) rather than full replacement
- **Shading:** Stylized PBR (metallic-roughness workflow)

### Current Engine Status (v0.7.0)
| System | Status | 3D Readiness |
|--------|--------|--------------|
| Vulkan Renderer | ✅ Complete | ⚠️ Needs depth buffer, mesh pipeline |
| Math Library | ✅ Complete | ✅ Vec3, Mat4, Quat ready |
| ECS | ✅ Complete | ⚠️ Needs 3D components |
| Combat/AI | ✅ Complete | ✅ Logic layer unchanged |
| Network | ✅ Complete | ✅ Unchanged |

---

## 1. Architecture Decisions

### 1.1 Forward vs Deferred Rendering

**Decision: Forward Rendering**

| Approach | Pros | Cons |
|----------|------|------|
| Forward | Simple, MSAA-friendly, transparent-friendly | More light passes |
| Deferred | Efficient many-lights | Complex, G-buffer overhead, MSAA issues |

**Rationale:** MOBA arenas have limited light sources (sun + ~10 point lights). Forward rendering with light culling is sufficient and simpler to implement.

### 1.2 2D/3D Pipeline Coexistence

```
┌─────────────────────────────────────────────────────────────┐
│                    RENDER FRAME                              │
├─────────────────────────────────────────────────────────────┤
│ Pass 1: Shadow Map (depth-only, light POV)                   │
│ Pass 2: Opaque 3D (depth test on, depth write on)           │
│ Pass 3: Transparent 3D (depth test on, depth write off)     │
│ Pass 4: 2D UI Overlay (depth test off, existing quad pipe)  │
└─────────────────────────────────────────────────────────────┘
```

The existing quad pipeline will be preserved for UI rendering. New 3D pipelines will be added alongside.

### 1.3 Asset Format

**Decision: glTF 2.0**

| Format | Pros | Cons |
|--------|------|------|
| glTF 2.0 | Standard, PBR support, animation, binary variant | - |
| FBX | Autodesk standard | Proprietary, complex parser |
| OBJ | Simple | No animation, no materials |

**Library:** cgltf (single-header, MIT license, ~3K LOC)

---

## 2. New Component Definitions

### 2.1 Transform3D Component

```c
// New 3D transform replacing 2D Transform for 3D entities
typedef struct Transform3D {
    Vec3 position;           // World position
    Quat rotation;           // Quaternion rotation
    Vec3 scale;              // Non-uniform scale

    // Cached data (updated by transform system)
    Mat4 local_matrix;       // T * R * S
    Mat4 world_matrix;       // Parent chain applied
    bool dirty;              // Needs recalculation
} Transform3D;

#define COMPONENT_TRANSFORM3D  (COMPONENT_TYPE_COUNT + 0)
```

### 2.2 MeshRenderer Component

```c
typedef struct MeshRenderer {
    uint32_t mesh_id;        // Handle to GPU mesh
    uint32_t material_id;    // Handle to material

    // Bounding volume for culling
    Vec3 bounds_min;
    Vec3 bounds_max;
    float bounds_radius;     // Bounding sphere

    // Render flags
    uint8_t layer;           // Render layer (opaque, transparent)
    bool cast_shadows;
    bool receive_shadows;
    bool visible;
} MeshRenderer;

#define COMPONENT_MESH_RENDERER (COMPONENT_TYPE_COUNT + 1)
```

### 2.3 Camera Component

```c
typedef enum CameraProjection {
    CAMERA_PERSPECTIVE,
    CAMERA_ORTHOGRAPHIC
} CameraProjection;

typedef struct Camera {
    CameraProjection projection;
    float fov;               // Vertical FOV (radians) for perspective
    float near_plane;
    float far_plane;
    float ortho_size;        // Half-height for orthographic

    // Computed each frame
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection;

    // Frustum planes for culling (6 planes)
    Vec4 frustum_planes[6];

    bool is_active;          // Only one active camera
    uint8_t priority;        // Higher = preferred
} Camera;

#define COMPONENT_CAMERA (COMPONENT_TYPE_COUNT + 2)
```

### 2.4 Light Component

```c
typedef enum LightType {
    LIGHT_DIRECTIONAL,       // Sun/moon
    LIGHT_POINT,             // Torches, explosions
    LIGHT_SPOT               // Focused beams
} LightType;

typedef struct Light {
    LightType type;
    Vec3 color;              // RGB, HDR allowed
    float intensity;

    // Point/spot only
    float range;
    float attenuation;

    // Spot only
    float inner_cone;        // Full intensity angle
    float outer_cone;        // Falloff angle

    // Shadow mapping
    bool cast_shadows;
    uint32_t shadow_map_id;  // If casting shadows
} Light;

#define COMPONENT_LIGHT (COMPONENT_TYPE_COUNT + 3)
```

### 2.5 SkinnedMesh Component

```c
#define MAX_BONES_PER_MESH 64

typedef struct SkinnedMesh {
    uint32_t skeleton_id;    // Skeleton asset handle

    // Bone transforms (computed each frame)
    Mat4 bone_matrices[MAX_BONES_PER_MESH];
    uint32_t bone_count;

    // Animation state
    uint32_t current_anim_id;
    float anim_time;
    float anim_speed;
    bool anim_looping;

    // Blending for smooth transitions
    uint32_t blend_from_anim;
    float blend_factor;      // 0 = from, 1 = current
    float blend_duration;
} SkinnedMesh;

#define COMPONENT_SKINNED_MESH (COMPONENT_TYPE_COUNT + 4)
```

---

## 3. Renderer Architecture Changes

### 3.1 Depth Buffer Addition

Current state: Single color attachment, no depth testing.

**Required changes:**

```c
// Add to Renderer struct
VkImage depth_image;
VkDeviceMemory depth_memory;
VkImageView depth_view;
VkFormat depth_format;  // VK_FORMAT_D32_SFLOAT recommended

// Modified render pass creation
VkAttachmentDescription attachments[2] = {
    // Color attachment (existing)
    {
        .format = swapchain_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    },
    // Depth attachment (NEW)
    {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    }
};
```

### 3.2 Vertex Buffer Manager

```c
typedef struct Vertex3D {
    Vec3 position;           // 12 bytes
    Vec3 normal;             // 12 bytes
    Vec2 uv;                 // 8 bytes
    Vec4 tangent;            // 16 bytes (w = handedness)
} Vertex3D;                  // Total: 48 bytes

typedef struct SkinnedVertex {
    Vec3 position;           // 12 bytes
    Vec3 normal;             // 12 bytes
    Vec2 uv;                 // 8 bytes
    Vec4 tangent;            // 16 bytes
    uint8_t bone_ids[4];     // 4 bytes (max 4 bones per vertex)
    float bone_weights[4];   // 16 bytes
} SkinnedVertex;             // Total: 68 bytes

typedef struct GPUMesh {
    uint32_t id;
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    VkDeviceMemory memory;   // Single allocation for both
    uint32_t vertex_count;
    uint32_t index_count;
    Vec3 bounds_min;
    Vec3 bounds_max;
    bool is_skinned;
} GPUMesh;

typedef struct MeshManager {
    GPUMesh* meshes;
    uint32_t mesh_count;
    uint32_t mesh_capacity;
    HashMap mesh_lookup;     // name -> id
} MeshManager;
```

### 3.3 Uniform Buffer Organization

```c
// Per-frame global data (binding 0)
typedef struct GlobalUBO {
    Mat4 view;
    Mat4 projection;
    Mat4 view_projection;
    Vec4 camera_position;    // w unused
    Vec4 time;               // x=total, y=delta, z=sin(t), w=cos(t)
    Vec4 screen_size;        // x=width, y=height, z=1/w, w=1/h
} GlobalUBO;

// Per-object data (binding 1, dynamic)
typedef struct ObjectUBO {
    Mat4 model;
    Mat4 model_inverse_transpose;  // For normal transformation
    Vec4 tint_color;
} ObjectUBO;

// Light data (binding 2)
#define MAX_LIGHTS 16

typedef struct LightData {
    Vec4 position_type;      // xyz=position, w=type
    Vec4 direction_range;    // xyz=direction, w=range
    Vec4 color_intensity;    // xyz=color, w=intensity
    Vec4 cone_attenuation;   // x=inner, y=outer, z=attenuation, w=unused
} LightData;

typedef struct LightingUBO {
    LightData lights[MAX_LIGHTS];
    Vec4 ambient_color;      // xyz=color, w=intensity
    uint32_t light_count;
    uint32_t pad[3];
} LightingUBO;
```

### 3.4 Descriptor Set Layouts

```c
// Set 0: Global (per-frame)
// binding 0: GlobalUBO
// binding 1: Shadow map sampler

// Set 1: Material (per-material)
// binding 0: albedo texture
// binding 1: normal map
// binding 2: metallic-roughness map
// binding 3: material parameters UBO

// Set 2: Object (per-draw, dynamic)
// binding 0: ObjectUBO
// binding 1: bone matrices (skinned only)
```


---

## 4. New Systems Required

### 4.1 Camera System

```c
// src/arena/game/camera_system.h

typedef struct CameraController {
    Entity camera_entity;

    // MOBA-style camera settings
    Vec3 target_position;    // Point camera looks at
    float distance;          // Distance from target
    float pitch;             // Vertical angle (typically 55-65°)
    float yaw;               // Horizontal rotation

    // Smooth follow
    float follow_speed;
    Entity follow_target;    // Entity to follow (player champion)

    // Bounds
    Vec3 bounds_min;         // Map camera bounds
    Vec3 bounds_max;
} CameraController;

void camera_system_init(World* world);
void camera_system_update(World* world, float dt);
void camera_system_set_target(CameraController* ctrl, Vec3 target);
void camera_system_follow(CameraController* ctrl, Entity target);

// Frustum culling helpers
bool camera_is_visible(Camera* cam, Vec3 bounds_min, Vec3 bounds_max);
bool camera_is_sphere_visible(Camera* cam, Vec3 center, float radius);
```

### 4.2 Mesh Manager

```c
// src/renderer/mesh_manager.h

typedef struct MeshManager MeshManager;

MeshManager* mesh_manager_create(Renderer* renderer, Arena* arena);
void mesh_manager_destroy(MeshManager* mgr);

// Loading
uint32_t mesh_manager_load(MeshManager* mgr, const char* path);
uint32_t mesh_manager_load_gltf(MeshManager* mgr, const char* path);

// Access
GPUMesh* mesh_manager_get(MeshManager* mgr, uint32_t id);
GPUMesh* mesh_manager_get_by_name(MeshManager* mgr, const char* name);

// Built-in primitives
uint32_t mesh_manager_create_cube(MeshManager* mgr);
uint32_t mesh_manager_create_sphere(MeshManager* mgr, uint32_t segments);
uint32_t mesh_manager_create_plane(MeshManager* mgr, float size);
```

### 4.3 Material Manager

```c
// src/renderer/material_manager.h

typedef struct Material {
    uint32_t id;

    // PBR properties
    Vec4 albedo_color;       // Base color + alpha
    float metallic;
    float roughness;
    float ao;                // Ambient occlusion
    Vec3 emissive;

    // Texture IDs (0 = no texture, use property value)
    uint32_t albedo_tex;
    uint32_t normal_tex;
    uint32_t metallic_roughness_tex;
    uint32_t ao_tex;
    uint32_t emissive_tex;

    // Render state
    VkDescriptorSet descriptor_set;
    bool alpha_blend;
    bool double_sided;
} Material;

typedef struct MaterialManager MaterialManager;

MaterialManager* material_manager_create(Renderer* renderer);
void material_manager_destroy(MaterialManager* mgr);

uint32_t material_create(MaterialManager* mgr, const char* name);
uint32_t material_load_from_gltf(MaterialManager* mgr, cgltf_material* mat);
Material* material_get(MaterialManager* mgr, uint32_t id);
void material_bind(MaterialManager* mgr, uint32_t id, VkCommandBuffer cmd);
```

### 4.4 Animation System

```c
// src/arena/game/animation_system.h

typedef struct AnimationClip {
    uint32_t id;
    const char* name;
    float duration;

    // Per-bone keyframe data
    uint32_t channel_count;
    struct AnimationChannel {
        uint32_t bone_index;
        // Position keyframes
        float* pos_times;
        Vec3* pos_values;
        uint32_t pos_count;
        // Rotation keyframes
        float* rot_times;
        Quat* rot_values;
        uint32_t rot_count;
        // Scale keyframes
        float* scale_times;
        Vec3* scale_values;
        uint32_t scale_count;
    }* channels;
} AnimationClip;

typedef struct AnimationManager AnimationManager;

AnimationManager* animation_manager_create(Arena* arena);
void animation_manager_destroy(AnimationManager* mgr);

uint32_t animation_load(AnimationManager* mgr, const char* path);
void animation_play(World* world, Entity entity, uint32_t anim_id, bool loop);
void animation_blend_to(World* world, Entity entity, uint32_t anim_id, float duration);
void animation_system_update(World* world, float dt);
```

### 4.5 Lighting System

```c
// src/arena/game/lighting_system.h

typedef struct LightingSystem {
    // Shadow mapping
    VkImage shadow_map;
    VkImageView shadow_view;
    VkSampler shadow_sampler;
    VkFramebuffer shadow_framebuffer;
    uint32_t shadow_resolution;  // 2048 recommended

    // Light collection
    LightData collected_lights[MAX_LIGHTS];
    uint32_t light_count;

    // Directional light (sun) for shadows
    Entity sun_entity;
    Mat4 light_space_matrix;
} LightingSystem;

void lighting_system_init(Renderer* renderer);
void lighting_system_collect_lights(World* world);
void lighting_system_render_shadows(World* world, Renderer* renderer);
void lighting_system_update_ubo(LightingSystem* sys, LightingUBO* ubo);
```

### 4.6 Culling System

```c
// src/arena/game/culling_system.h

typedef struct CullResult {
    Entity* visible_entities;
    uint32_t count;
    uint32_t capacity;
} CullResult;

// Frustum culling against active camera
void culling_system_cull(World* world, Camera* camera, CullResult* result);

// Occlusion culling (optional, Phase 2+)
void culling_system_hierarchical_z(World* world, Camera* camera, CullResult* result);
```

---

## 5. Asset Pipeline

### 5.1 cgltf Integration

**Library:** cgltf v1.13+ (MIT license, header-only)
**Location:** `libs/cgltf/cgltf.h`

```c
// src/renderer/gltf_loader.h

typedef struct GLTFLoadResult {
    bool success;
    uint32_t mesh_count;
    uint32_t material_count;
    uint32_t animation_count;
    uint32_t* mesh_ids;
    uint32_t* material_ids;
    uint32_t* animation_ids;
    char error[256];
} GLTFLoadResult;

// Load entire glTF scene
GLTFLoadResult gltf_load(const char* path,
                         MeshManager* meshes,
                         MaterialManager* materials,
                         AnimationManager* animations,
                         Arena* scratch);

// Load specific components
uint32_t gltf_load_mesh(const char* path, const char* mesh_name, MeshManager* mgr);
uint32_t gltf_load_animation(const char* path, const char* anim_name, AnimationManager* mgr);
```

### 5.2 Texture Loading

stb_image is already included. Extend for PBR textures:

```c
// src/renderer/texture_manager.h

typedef enum TextureFormat {
    TEXTURE_FORMAT_RGBA8_SRGB,      // Albedo (sRGB)
    TEXTURE_FORMAT_RGBA8_LINEAR,    // Normal, metallic-roughness (linear)
    TEXTURE_FORMAT_R8,              // Single channel (AO)
} TextureFormat;

typedef struct Texture {
    uint32_t id;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkDeviceMemory memory;
    uint32_t width, height;
    uint32_t mip_levels;
} Texture;

uint32_t texture_load(TextureManager* mgr, const char* path, TextureFormat format);
uint32_t texture_load_from_memory(TextureManager* mgr, const uint8_t* data,
                                   uint32_t width, uint32_t height, TextureFormat format);
void texture_generate_mipmaps(TextureManager* mgr, uint32_t id);
```

### 5.3 Material Definition Format

Materials can be defined in JSON for easy iteration:

```json
{
  "name": "champion_armor",
  "albedo": [0.8, 0.2, 0.2, 1.0],
  "metallic": 0.7,
  "roughness": 0.3,
  "textures": {
    "albedo": "assets/textures/armor_albedo.png",
    "normal": "assets/textures/armor_normal.png",
    "metallic_roughness": "assets/textures/armor_mr.png"
  },
  "alpha_blend": false,
  "double_sided": false
}
```

### 5.4 Asset Packer Updates

Extend `tools/asset-packer` for 3D assets:

```
arena-asset-packer pack <output.pak> <input-dir>

Supported formats:
  .gltf, .glb    - 3D models with materials and animations
  .png, .jpg     - Textures (auto-mipmap generation)
  .json          - Material definitions
  .ogg, .wav     - Audio (existing)
```

---

## 6. Implementation Milestones

### 6.1 v0.8.0 - "3D Foundation" (2-3 weeks)

**Goal:** Render textured 3D meshes with basic camera control

**Deliverables:**

| Task | Estimated LOC | Priority |
|------|---------------|----------|
| Depth buffer implementation | ~100 | P0 |
| 3D mesh pipeline (shaders) | ~200 | P0 |
| Transform3D component | ~50 | P0 |
| MeshRenderer component | ~50 | P0 |
| Camera component + system | ~200 | P0 |
| Basic mesh loading (cgltf) | ~300 | P0 |
| Vertex buffer manager | ~200 | P0 |
| Single texture support | ~100 | P1 |
| **Total** | **~1,200** | |

**Acceptance Criteria:**
- [ ] Load and render a textured cube from glTF
- [ ] Camera orbits around scene with mouse
- [ ] Depth sorting works correctly
- [ ] Existing 2D UI still renders on top
- [ ] No Vulkan validation errors

**New Files:**
```
src/renderer/mesh_pipeline.c/h
src/renderer/mesh_manager.c/h
src/renderer/gltf_loader.c/h
src/renderer/shaders/mesh.vert
src/renderer/shaders/mesh.frag
src/arena/ecs/components/transform3d.h
src/arena/game/camera_system.c/h
libs/cgltf/cgltf.h
```


### 6.2 v0.9.0 - "Materials & Lighting" (2-3 weeks)

**Goal:** PBR materials, multi-texture support, basic lighting

**Deliverables:**

| Task | Estimated LOC | Priority |
|------|---------------|----------|
| Material system | ~300 | P0 |
| PBR shader implementation | ~250 | P0 |
| Multi-texture descriptor sets | ~150 | P0 |
| Light component | ~50 | P0 |
| Lighting system | ~200 | P0 |
| Directional shadow mapping | ~300 | P1 |
| Material manager | ~200 | P1 |
| **Total** | **~1,450** | |

**Acceptance Criteria:**
- [ ] Load glTF models with embedded PBR materials
- [ ] Directional light (sun) casts shadows
- [ ] Point lights illuminate scene
- [ ] Shadow mapping at 2048x2048 resolution
- [ ] Materials can be hot-reloaded

**New Files:**
```
src/renderer/material_manager.c/h
src/renderer/shaders/pbr.vert
src/renderer/shaders/pbr.frag
src/renderer/shaders/shadow.vert
src/renderer/shaders/shadow.frag
src/arena/game/lighting_system.c/h
```

### 6.3 v0.10.0 - "Animation & Polish" (2-3 weeks)

**Goal:** Skeletal animation, instancing, visual polish

**Deliverables:**

| Task | Estimated LOC | Priority |
|------|---------------|----------|
| SkinnedMesh component | ~100 | P0 |
| Bone hierarchy loading | ~200 | P0 |
| Animation clip loading | ~200 | P0 |
| Animation blending (slerp) | ~150 | P0 |
| Animation system | ~300 | P0 |
| GPU skinning shader | ~100 | P0 |
| Hardware instancing for minions | ~200 | P1 |
| Frustum culling optimization | ~150 | P1 |
| **Total** | **~1,400** | |

**Acceptance Criteria:**
- [ ] Champion model plays idle, walk, attack animations
- [ ] Smooth animation blending (< 0.3s transitions)
- [ ] 50+ minions rendered at 60 FPS via instancing
- [ ] Frustum culling reduces draw calls by > 50%

**New Files:**
```
src/arena/game/animation_system.c/h
src/renderer/shaders/skinned.vert
src/renderer/instancing.c/h
src/arena/game/culling_system.c/h
```

### 6.4 v1.0.0 - "Release" (Polish Phase)

**Goal:** Production-ready 3D MOBA

**Focus Areas:**
- Performance optimization (< 16ms frame time)
- Visual polish (particles, post-processing)
- Asset integration (champions, map, VFX)
- Bug fixes and stability

**Optional Enhancements:**
- [ ] Screen-space ambient occlusion (SSAO)
- [ ] Bloom post-processing
- [ ] Particle system
- [ ] LOD system for distant objects

---

## 7. Dependencies

### 7.1 Required Libraries

| Library | Version | License | Purpose | Size |
|---------|---------|---------|---------|------|
| cgltf | 1.13+ | MIT | glTF loading | ~3K LOC, header-only |
| stb_image | 2.28+ | MIT | Texture loading | Already included |

### 7.2 Optional Libraries

| Library | Version | License | Purpose | When Needed |
|---------|---------|---------|---------|-------------|
| JoltPhysics | 4.0+ | MIT | 3D physics | If physics required |
| meshoptimizer | 0.19+ | MIT | Mesh optimization | Asset pipeline |
| KTX-Software | 4.0+ | Apache 2.0 | Compressed textures | Memory optimization |

### 7.3 Shader Compilation

Existing glslc workflow remains. New shaders:

```bash
# Compile new 3D shaders
glslc src/renderer/shaders/mesh.vert -o build/shaders/mesh.vert.spv
glslc src/renderer/shaders/mesh.frag -o build/shaders/mesh.frag.spv
glslc src/renderer/shaders/pbr.vert -o build/shaders/pbr.vert.spv
glslc src/renderer/shaders/pbr.frag -o build/shaders/pbr.frag.spv
glslc src/renderer/shaders/shadow.vert -o build/shaders/shadow.vert.spv
glslc src/renderer/shaders/skinned.vert -o build/shaders/skinned.vert.spv
```

---

## 8. Risk Assessment

### 8.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Vulkan complexity explosion | Medium | High | Incremental implementation, validate each step |
| Performance regression | Medium | Medium | Profile early, budget for optimization time |
| Shader debugging difficulty | High | Medium | Use RenderDoc, validation layers, printf debugging |
| Animation edge cases | Medium | Low | Test with variety of glTF files from Mixamo |

### 8.2 Resource Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Asset creation burden | High | Medium | Use free assets (Mixamo, Kenney, Poly Haven) |
| Scope creep | Medium | High | Strict milestone definitions, defer nice-to-haves |
| Timeline slip | Medium | Medium | Buffer in estimates, prioritize P0 features |

### 8.3 Integration Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| 2D/3D pipeline conflict | Low | High | Keep pipelines independent, test UI overlay |
| ECS component bloat | Low | Medium | Use composition, profile memory usage |
| Network sync with 3D | Low | Low | Transform is already Vec3-based |

---

## 9. Free Asset Sources

### 9.1 Character Assets

| Source | Type | License | Notes |
|--------|------|---------|-------|
| **Mixamo** | Characters + Animations | Free (Adobe ID) | Rigged humanoids, 2K+ animations |
| Quaternius | Low-poly characters | CC0 | Good for minions |
| Kenney | Character packs | CC0 | Stylized, game-ready |

### 9.2 Environment Assets

| Source | Type | License | Notes |
|--------|------|---------|-------|
| **Kenney** | Modular kits | CC0 | Castles, nature, urban |
| Poly Haven | HDRIs, textures | CC0 | PBR-ready |
| ambientCG | PBR materials | CC0 | 2K/4K textures |

### 9.3 Recommended Starter Pack

```
assets/
├── characters/
│   ├── warrior.glb          # Mixamo character
│   ├── mage.glb             # Mixamo character
│   └── minion.glb           # Quaternius low-poly
├── animations/
│   ├── idle.glb
│   ├── walk.glb
│   ├── run.glb
│   └── attack.glb
├── environment/
│   ├── tower.glb            # Kenney castle kit
│   ├── ground_tile.glb
│   └── tree.glb
└── textures/
    ├── ground_albedo.png    # Poly Haven
    ├── ground_normal.png
    └── ground_mr.png
```

---

## 10. Timeline Summary

```
Week 1-3:   v0.8.0 - 3D Foundation
            ├── Depth buffer
            ├── Mesh pipeline
            ├── Camera system
            └── glTF loading (basic)

Week 4-6:   v0.9.0 - Materials & Lighting
            ├── PBR shaders
            ├── Material system
            ├── Lighting system
            └── Shadow mapping

Week 7-9:   v0.10.0 - Animation & Polish
            ├── Skeletal animation
            ├── Animation blending
            ├── Instancing
            └── Culling

Week 10+:   v1.0.0 - Release
            ├── Performance optimization
            ├── Asset integration
            ├── Bug fixes
            └── Optional: Post-processing
```

### Total Estimated Effort

| Milestone | LOC | Time | Complexity |
|-----------|-----|------|------------|
| v0.8.0 | ~1,200 | 2-3 weeks | Medium |
| v0.9.0 | ~1,450 | 2-3 weeks | High |
| v0.10.0 | ~1,400 | 2-3 weeks | High |
| v1.0.0 | ~500 | 1-2 weeks | Medium |
| **Total** | **~4,550** | **7-11 weeks** | |

---

## 11. Appendix: Shader Specifications

### 11.1 Basic Mesh Vertex Shader

```glsl
#version 450

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    vec4 camera_position;
    vec4 time;
    vec4 screen_size;
} global;

layout(set = 2, binding = 0) uniform ObjectUBO {
    mat4 model;
    mat4 model_inverse_transpose;
    vec4 tint_color;
} object;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec3 frag_world_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out mat3 frag_TBN;

void main() {
    vec4 world_pos = object.model * vec4(in_position, 1.0);
    frag_world_pos = world_pos.xyz;

    frag_normal = mat3(object.model_inverse_transpose) * in_normal;
    frag_uv = in_uv;

    // TBN matrix for normal mapping
    vec3 T = normalize(mat3(object.model) * in_tangent.xyz);
    vec3 N = normalize(frag_normal);
    vec3 B = cross(N, T) * in_tangent.w;
    frag_TBN = mat3(T, B, N);

    gl_Position = global.view_projection * world_pos;
}
```

### 11.2 PBR Fragment Shader (Simplified)

```glsl
#version 450

layout(set = 0, binding = 0) uniform GlobalUBO { /* ... */ } global;
layout(set = 0, binding = 1) uniform sampler2D shadow_map;

layout(set = 1, binding = 0) uniform sampler2D albedo_tex;
layout(set = 1, binding = 1) uniform sampler2D normal_tex;
layout(set = 1, binding = 2) uniform sampler2D metallic_roughness_tex;

layout(set = 2, binding = 0) uniform LightingUBO {
    vec4 lights[16];
    vec4 ambient;
    uint light_count;
} lighting;

layout(location = 0) in vec3 frag_world_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in mat3 frag_TBN;

layout(location = 0) out vec4 out_color;

// Simplified PBR (full implementation would include BRDF functions)
void main() {
    vec4 albedo = texture(albedo_tex, frag_uv);
    vec3 normal = texture(normal_tex, frag_uv).rgb * 2.0 - 1.0;
    normal = normalize(frag_TBN * normal);

    vec2 mr = texture(metallic_roughness_tex, frag_uv).bg;
    float metallic = mr.x;
    float roughness = mr.y;

    vec3 view_dir = normalize(global.camera_position.xyz - frag_world_pos);

    // Accumulate lighting
    vec3 Lo = vec3(0.0);
    // ... lighting calculations ...

    vec3 ambient = lighting.ambient.rgb * albedo.rgb;
    vec3 color = ambient + Lo;

    out_color = vec4(color, albedo.a);
}
```

---

## 12. Decision Log

| Date | Decision | Rationale | Alternatives Considered |
|------|----------|-----------|------------------------|
| 2026-03-08 | Forward rendering | Simpler, sufficient for MOBA | Deferred (too complex for now) |
| 2026-03-08 | glTF 2.0 format | Industry standard, free tools | FBX (proprietary), OBJ (limited) |
| 2026-03-08 | cgltf library | Header-only, MIT, minimal | tinygltf (heavier), custom (time) |
| 2026-03-08 | Parallel 2D/3D pipes | Preserves UI, lower risk | Full migration (breaking) |
| 2026-03-08 | Stylized PBR | Visual quality + simplicity | Full PBR (overkill), Unlit (too simple) |

---

*Document created for Arena Engine v0.8.0+ development planning.*