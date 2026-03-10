#ifndef ARENA_COMPONENTS_3D_H
#define ARENA_COMPONENTS_3D_H

#include "../math/math.h"
#include "ecs.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// 3D Components for Arena Engine
// ============================================================================

// Maximum bones per skinned mesh
#define MAX_BONES_PER_MESH 64

// ----------------------------------------------------------------------------
// Transform3D - Full 3D transformation with matrix caching
// ----------------------------------------------------------------------------

typedef struct Transform3D {
    Vec3 position;           // World position
    Quat rotation;           // Quaternion rotation
    Vec3 scale;              // Non-uniform scale
    
    Mat4 local_matrix;       // Cached T * R * S
    Mat4 world_matrix;       // With parent chain applied (if parented)
    
    Entity parent;           // Parent entity (ENTITY_NULL if root)
    bool dirty;              // Needs matrix recalculation
} Transform3D;

// Initialize a Transform3D with default values
static inline void transform3d_init(Transform3D* t) {
    t->position = vec3_zero();
    t->rotation = quat_identity();
    t->scale = vec3_one();
    t->local_matrix = mat4_identity();
    t->world_matrix = mat4_identity();
    t->parent = ENTITY_NULL;
    t->dirty = true;
}

// Mark transform as needing update
static inline void transform3d_set_dirty(Transform3D* t) {
    t->dirty = true;
}

// Calculate local matrix from position, rotation, scale
static inline void transform3d_update_local(Transform3D* t) {
    if (!t->dirty) return;
    
    Mat4 translation = mat4_translate(t->position);
    Mat4 rotation = quat_to_mat4(t->rotation);
    Mat4 scale = mat4_scale(t->scale);
    
    // TRS order: scale first, then rotate, then translate
    t->local_matrix = mat4_mul(translation, mat4_mul(rotation, scale));
    t->dirty = false;
}

// Get forward vector (negative Z in local space)
static inline Vec3 transform3d_forward(Transform3D* t) {
    return quat_rotate_vec3(t->rotation, vec3(0, 0, -1));
}

// Get right vector (positive X in local space)
static inline Vec3 transform3d_right(Transform3D* t) {
    return quat_rotate_vec3(t->rotation, vec3(1, 0, 0));
}

// Get up vector (positive Y in local space)
static inline Vec3 transform3d_up(Transform3D* t) {
    return quat_rotate_vec3(t->rotation, vec3(0, 1, 0));
}

// ----------------------------------------------------------------------------
// MeshRenderer - Static 3D mesh rendering
// ----------------------------------------------------------------------------

typedef struct MeshRenderer {
    uint32_t mesh_id;        // Handle to GPU mesh
    uint32_t material_id;    // Handle to PBR material
    
    Vec3 bounds_min;         // Local AABB minimum
    Vec3 bounds_max;         // Local AABB maximum
    float bounds_radius;     // Bounding sphere radius
    
    uint8_t layer;           // Render layer (0=opaque, 1=transparent)
    bool cast_shadows;
    bool receive_shadows;
    bool visible;            // Culling flag
} MeshRenderer;

// Initialize a MeshRenderer with default values
static inline void mesh_renderer_init(MeshRenderer* mr) {
    mr->mesh_id = 0;
    mr->material_id = 0;
    mr->bounds_min = vec3_zero();
    mr->bounds_max = vec3_zero();
    mr->bounds_radius = 0.0f;
    mr->layer = 0;
    mr->cast_shadows = true;
    mr->receive_shadows = true;
    mr->visible = true;
}

// ----------------------------------------------------------------------------
// Camera - View and projection
// ----------------------------------------------------------------------------

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
    
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection;
    
    Vec4 frustum_planes[6];  // For frustum culling (normalized planes)
    
    bool is_active;          // Is this the active camera?
    uint8_t priority;        // Higher priority cameras render first
} Camera;

// Initialize a Camera with default perspective settings
static inline void camera_init_perspective(Camera* cam, float fov, float near, float far) {
    cam->projection = CAMERA_PERSPECTIVE;
    cam->fov = fov;
    cam->near_plane = near;
    cam->far_plane = far;
    cam->ortho_size = 10.0f;
    cam->view_matrix = mat4_identity();
    cam->projection_matrix = mat4_identity();
    cam->view_projection = mat4_identity();
    cam->is_active = true;
    cam->priority = 0;
}

// Initialize a Camera with orthographic settings (good for isometric)
static inline void camera_init_ortho(Camera* cam, float size, float near, float far) {
    cam->projection = CAMERA_ORTHOGRAPHIC;
    cam->fov = 60.0f * ARENA_DEG2RAD;
    cam->near_plane = near;
    cam->far_plane = far;
    cam->ortho_size = size;
    cam->view_matrix = mat4_identity();
    cam->projection_matrix = mat4_identity();
    cam->view_projection = mat4_identity();
    cam->is_active = true;
    cam->priority = 0;
}

// ----------------------------------------------------------------------------
// Light - Scene illumination
// ----------------------------------------------------------------------------

typedef enum LightType {
    LIGHT_DIRECTIONAL,       // Sun/moon (infinite distance)
    LIGHT_POINT,             // Torches, explosions (radial falloff)
    LIGHT_SPOT               // Focused beams (cone)
} LightType;

typedef struct Light {
    LightType type;
    Vec3 color;              // RGB, HDR values allowed (>1.0)
    float intensity;         // Light strength multiplier

    float range;             // Point/spot: max distance
    float attenuation;       // Falloff rate

    float inner_cone;        // Spot: inner angle (radians)
    float outer_cone;        // Spot: outer angle (radians)

    bool cast_shadows;
    uint32_t shadow_map_id;  // Handle to shadow map texture
} Light;

// Initialize a directional light (like the sun)
static inline void light_init_directional(Light* l, Vec3 color, float intensity) {
    l->type = LIGHT_DIRECTIONAL;
    l->color = color;
    l->intensity = intensity;
    l->range = 0.0f;
    l->attenuation = 1.0f;
    l->inner_cone = 0.0f;
    l->outer_cone = 0.0f;
    l->cast_shadows = true;
    l->shadow_map_id = 0;
}

// Initialize a point light (like a torch)
static inline void light_init_point(Light* l, Vec3 color, float intensity, float range) {
    l->type = LIGHT_POINT;
    l->color = color;
    l->intensity = intensity;
    l->range = range;
    l->attenuation = 1.0f;
    l->inner_cone = 0.0f;
    l->outer_cone = 0.0f;
    l->cast_shadows = false;
    l->shadow_map_id = 0;
}

// ----------------------------------------------------------------------------
// SkinnedMesh - Skeletal animation
// ----------------------------------------------------------------------------

typedef struct SkinnedMesh {
    uint32_t skeleton_id;                    // Skeleton asset handle
    Mat4 bone_matrices[MAX_BONES_PER_MESH];  // Current bone transforms
    uint32_t bone_count;

    uint32_t current_anim_id;
    float anim_time;
    float anim_speed;
    bool anim_looping;

    // Animation blending
    uint32_t blend_from_anim;
    float blend_factor;      // 0 = from, 1 = current
    float blend_duration;
} SkinnedMesh;

// Initialize a SkinnedMesh
static inline void skinned_mesh_init(SkinnedMesh* sm) {
    sm->skeleton_id = 0;
    sm->bone_count = 0;
    sm->current_anim_id = 0;
    sm->anim_time = 0.0f;
    sm->anim_speed = 1.0f;
    sm->anim_looping = true;
    sm->blend_from_anim = 0;
    sm->blend_factor = 1.0f;
    sm->blend_duration = 0.0f;

    // Initialize all bone matrices to identity
    for (uint32_t i = 0; i < MAX_BONES_PER_MESH; i++) {
        sm->bone_matrices[i] = mat4_identity();
    }
}

#endif // ARENA_COMPONENTS_3D_H

