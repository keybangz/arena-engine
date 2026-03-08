#ifndef ARENA_CAMERA_SYSTEM_H
#define ARENA_CAMERA_SYSTEM_H

#include "../math/math.h"
#include "../ecs/ecs.h"
#include "../ecs/components_3d.h"

// ============================================================================
// Camera System for Arena Engine
// ============================================================================

// Isometric camera angles (MOBA standard)
#define ISOMETRIC_PITCH     (55.0f * ARENA_DEG2RAD)  // ~55° down from horizontal
#define ISOMETRIC_YAW       (45.0f * ARENA_DEG2RAD)  // 45° rotation (optional)

// Default camera settings
#define DEFAULT_FOV         (60.0f * ARENA_DEG2RAD)
#define DEFAULT_NEAR        0.1f
#define DEFAULT_FAR         1000.0f
#define DEFAULT_ORTHO_SIZE  20.0f

// Camera controller modes
typedef enum CameraMode {
    CAMERA_MODE_FREE,       // Free-fly camera (debug)
    CAMERA_MODE_FOLLOW,     // Follow target entity
    CAMERA_MODE_LOCKED      // Fixed position
} CameraMode;

// ----------------------------------------------------------------------------
// Camera Controller (attaches to entity with Camera + Transform3D)
// ----------------------------------------------------------------------------

typedef struct CameraController {
    CameraMode mode;
    
    // Follow mode settings
    Entity follow_target;
    Vec3 follow_offset;     // Offset from target in world space
    float follow_smooth;    // Smoothing factor (0 = instant, higher = smoother)
    
    // Zoom
    float zoom_level;       // 1.0 = default, < 1.0 = zoomed in, > 1.0 = zoomed out
    float zoom_min;
    float zoom_max;
    float zoom_speed;
    
    // Pan (edge scrolling)
    float pan_speed;
    bool edge_pan_enabled;
    float edge_threshold;   // Pixels from edge to trigger pan
    
    // Bounds
    Vec3 bounds_min;
    Vec3 bounds_max;
    bool bounds_enabled;
} CameraController;

// Initialize camera controller with defaults
void camera_controller_init(CameraController* cc);

// ----------------------------------------------------------------------------
// Camera Utility Functions
// ----------------------------------------------------------------------------

// Update camera matrices from Transform3D
void camera_update_matrices(Camera* cam, Transform3D* transform, float aspect_ratio);

// Update isometric camera from a top-down focus point
void camera_set_isometric(Camera* cam, Transform3D* transform,
                          Vec3 focus_point, float distance, float pitch, float yaw);

// Extract frustum planes from view-projection matrix (for culling)
void camera_extract_frustum_planes(Camera* cam);

// Test if a sphere is visible in the frustum
bool camera_sphere_in_frustum(Camera* cam, Vec3 center, float radius);

// Test if an AABB is visible in the frustum
bool camera_aabb_in_frustum(Camera* cam, Vec3 min, Vec3 max);

// ----------------------------------------------------------------------------
// Screen/World Conversion
// ----------------------------------------------------------------------------

// Convert screen coordinates to world ray
void camera_screen_to_ray(Camera* cam, Transform3D* transform,
                          float screen_x, float screen_y,
                          float screen_width, float screen_height,
                          Vec3* ray_origin, Vec3* ray_direction);

// Convert screen coordinates to world position on a plane (y = plane_y)
Vec3 camera_screen_to_world_plane(Camera* cam, Transform3D* transform,
                                   float screen_x, float screen_y,
                                   float screen_width, float screen_height,
                                   float plane_y);

// Convert world position to screen coordinates
Vec2 camera_world_to_screen(Camera* cam,
                            Vec3 world_pos,
                            float screen_width, float screen_height);

// ----------------------------------------------------------------------------
// Camera System (ECS)
// ----------------------------------------------------------------------------

// Update all cameras in the world (call once per frame)
void camera_system_update(World* world, float dt, float aspect_ratio);

// Get the active camera entity (highest priority with is_active=true)
Entity camera_system_get_active(World* world);

// Get view and projection matrices from active camera
bool camera_system_get_matrices(World* world, Mat4* view, Mat4* projection, Vec3* position);

#endif // ARENA_CAMERA_SYSTEM_H

