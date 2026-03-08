#include "camera_system.h"
#include <math.h>

// ============================================================================
// Camera Controller
// ============================================================================

void camera_controller_init(CameraController* cc) {
    cc->mode = CAMERA_MODE_FOLLOW;
    cc->follow_target = ENTITY_NULL;
    cc->follow_offset = vec3(0, 15.0f, 10.0f);  // Behind and above
    cc->follow_smooth = 5.0f;
    
    cc->zoom_level = 1.0f;
    cc->zoom_min = 0.5f;
    cc->zoom_max = 2.0f;
    cc->zoom_speed = 0.1f;
    
    cc->pan_speed = 20.0f;
    cc->edge_pan_enabled = true;
    cc->edge_threshold = 20.0f;
    
    cc->bounds_enabled = false;
    cc->bounds_min = vec3(-100, -100, -100);
    cc->bounds_max = vec3(100, 100, 100);
}

// ============================================================================
// Camera Matrix Updates
// ============================================================================

void camera_update_matrices(Camera* cam, Transform3D* transform, float aspect_ratio) {
    // Update local matrix if dirty
    transform3d_update_local(transform);

    // Compute forward direction from rotation
    Vec3 forward = quat_forward(transform->rotation);
    Vec3 target = vec3_add(transform->position, forward);
    Vec3 up = quat_up(transform->rotation);

    // Create view matrix using look-at
    cam->view_matrix = mat4_look_at(transform->position, target, up);

    // Create projection matrix based on camera type
    if (cam->projection == CAMERA_PERSPECTIVE) {
        cam->projection_matrix = mat4_perspective(cam->fov, aspect_ratio,
                                                   cam->near_plane, cam->far_plane);
    } else {
        float half_height = cam->ortho_size;
        float half_width = half_height * aspect_ratio;
        cam->projection_matrix = mat4_ortho(-half_width, half_width,
                                            -half_height, half_height,
                                            cam->near_plane, cam->far_plane);
    }

    // Combine view and projection
    cam->view_projection = mat4_mul(cam->projection_matrix, cam->view_matrix);

    // Extract frustum planes for culling
    camera_extract_frustum_planes(cam);
}

void camera_set_isometric(Camera* cam, Transform3D* transform,
                          Vec3 focus_point, float distance, float pitch, float yaw) {
    // Calculate camera position from angles and distance
    float cos_pitch = cosf(pitch);
    float sin_pitch = sinf(pitch);
    float cos_yaw = cosf(yaw);
    float sin_yaw = sinf(yaw);
    
    Vec3 offset = {
        distance * cos_pitch * sin_yaw,
        distance * sin_pitch,
        distance * cos_pitch * cos_yaw
    };
    
    transform->position = vec3_add(focus_point, offset);
    
    // Create rotation using look-at logic
    transform->rotation = quat_look_at(transform->position, focus_point, vec3(0, 1, 0));
    transform->dirty = true;

    (void)cam;  // Camera updated separately via camera_update_matrices
}

// ============================================================================
// Frustum Culling
// ============================================================================

void camera_extract_frustum_planes(Camera* cam) {
    Mat4* m = &cam->view_projection;
    
    // Left plane
    cam->frustum_planes[0] = vec4(
        m->m[0][3] + m->m[0][0], m->m[1][3] + m->m[1][0],
        m->m[2][3] + m->m[2][0], m->m[3][3] + m->m[3][0]);
    // Right plane
    cam->frustum_planes[1] = vec4(
        m->m[0][3] - m->m[0][0], m->m[1][3] - m->m[1][0],
        m->m[2][3] - m->m[2][0], m->m[3][3] - m->m[3][0]);
    // Bottom plane
    cam->frustum_planes[2] = vec4(
        m->m[0][3] + m->m[0][1], m->m[1][3] + m->m[1][1],
        m->m[2][3] + m->m[2][1], m->m[3][3] + m->m[3][1]);
    // Top plane
    cam->frustum_planes[3] = vec4(
        m->m[0][3] - m->m[0][1], m->m[1][3] - m->m[1][1],
        m->m[2][3] - m->m[2][1], m->m[3][3] - m->m[3][1]);
    // Near plane
    cam->frustum_planes[4] = vec4(
        m->m[0][3] + m->m[0][2], m->m[1][3] + m->m[1][2],
        m->m[2][3] + m->m[2][2], m->m[3][3] + m->m[3][2]);
    // Far plane
    cam->frustum_planes[5] = vec4(
        m->m[0][3] - m->m[0][2], m->m[1][3] - m->m[1][2],
        m->m[2][3] - m->m[2][2], m->m[3][3] - m->m[3][2]);
    
    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        cam->frustum_planes[i] = vec4_normalize_plane(cam->frustum_planes[i]);
    }
}

bool camera_sphere_in_frustum(Camera* cam, Vec3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        float dist = vec4_plane_distance(cam->frustum_planes[i], center);
        if (dist < -radius) return false;  // Completely outside
    }
    return true;
}

bool camera_aabb_in_frustum(Camera* cam, Vec3 min, Vec3 max) {
    for (int i = 0; i < 6; i++) {
        Vec4 plane = cam->frustum_planes[i];

        // Find the positive vertex (furthest along plane normal)
        Vec3 p = {
            (plane.x > 0) ? max.x : min.x,
            (plane.y > 0) ? max.y : min.y,
            (plane.z > 0) ? max.z : min.z
        };

        if (vec4_plane_distance(plane, p) < 0) return false;
    }
    return true;
}

// ============================================================================
// Screen/World Conversion
// ============================================================================

void camera_screen_to_ray(Camera* cam, Transform3D* transform,
                          float screen_x, float screen_y,
                          float screen_width, float screen_height,
                          Vec3* ray_origin, Vec3* ray_direction) {
    // Convert screen coords to NDC (-1 to 1)
    float ndc_x = (2.0f * screen_x / screen_width) - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_y / screen_height);  // Flip Y

    // Inverse view-projection matrix
    Mat4 inv_vp = mat4_inverse(cam->view_projection);

    // Near and far points in NDC
    Vec4 near_ndc = vec4(ndc_x, ndc_y, 0.0f, 1.0f);
    Vec4 far_ndc = vec4(ndc_x, ndc_y, 1.0f, 1.0f);

    // Transform to world space
    Vec4 near_world = mat4_mul_vec4(inv_vp, near_ndc);
    Vec4 far_world = mat4_mul_vec4(inv_vp, far_ndc);

    // Perspective divide
    Vec3 near_pos = vec4_to_vec3_perspective(near_world);
    Vec3 far_pos = vec4_to_vec3_perspective(far_world);

    *ray_origin = near_pos;
    *ray_direction = vec3_normalize(vec3_sub(far_pos, near_pos));

    (void)transform; // Transform already baked into view matrix
}

Vec3 camera_screen_to_world_plane(Camera* cam, Transform3D* transform,
                                   float screen_x, float screen_y,
                                   float screen_width, float screen_height,
                                   float plane_y) {
    Vec3 ray_origin, ray_dir;
    camera_screen_to_ray(cam, transform, screen_x, screen_y,
                         screen_width, screen_height, &ray_origin, &ray_dir);

    // Intersect ray with horizontal plane at y = plane_y
    // Ray: P = O + t*D
    // Plane: P.y = plane_y
    // Solve: O.y + t*D.y = plane_y
    // t = (plane_y - O.y) / D.y

    if (fabsf(ray_dir.y) < 1e-6f) {
        // Ray parallel to plane
        return ray_origin;
    }

    float t = (plane_y - ray_origin.y) / ray_dir.y;

    // Only accept hits in front of camera
    if (t < 0) t = 0;

    return vec3_add(ray_origin, vec3_scale(ray_dir, t));
}

Vec2 camera_world_to_screen(Camera* cam,
                            Vec3 world_pos,
                            float screen_width, float screen_height) {
    // Transform to clip space
    Vec4 clip = mat4_mul_vec4(cam->view_projection, vec4_from_vec3(world_pos, 1.0f));

    // Perspective divide to NDC
    if (fabsf(clip.w) < 1e-6f) {
        return vec2(-1, -1);  // Behind camera
    }

    Vec3 ndc = {clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};

    // NDC to screen
    return vec2(
        (ndc.x + 1.0f) * 0.5f * screen_width,
        (1.0f - ndc.y) * 0.5f * screen_height  // Flip Y
    );
}

// ============================================================================
// ECS Camera System
// ============================================================================

void camera_system_update(World* world, float dt, float aspect_ratio) {
    (void)dt;  // Could be used for smoothing

    // Iterate all entities with Camera and Transform3D
    uint32_t entity_count = world_entity_count(world);
    for (uint32_t i = 1; i <= entity_count; i++) {
        // Try to get components - world_get_* will validate entity
        Entity entity = entity_make(i, 0);  // Generation doesn't matter for component access

        Camera* cam = world_get_camera(world, entity);
        Transform3D* transform = world_get_transform3d(world, entity);

        if (cam && transform) {
            camera_update_matrices(cam, transform, aspect_ratio);
        }
    }
}

Entity camera_system_get_active(World* world) {
    Entity best = ENTITY_NULL;
    uint8_t best_priority = 0;

    uint32_t entity_count = world_entity_count(world);
    for (uint32_t i = 1; i <= entity_count; i++) {
        Entity entity = entity_make(i, 0);

        Camera* cam = world_get_camera(world, entity);
        if (cam && cam->is_active) {
            if (entity_is_null(best) || cam->priority > best_priority) {
                best = entity;
                best_priority = cam->priority;
            }
        }
    }

    return best;
}

bool camera_system_get_matrices(World* world, Mat4* view, Mat4* projection, Vec3* position) {
    Entity active = camera_system_get_active(world);
    if (entity_is_null(active)) return false;

    Camera* cam = world_get_camera(world, active);
    Transform3D* transform = world_get_transform3d(world, active);

    if (!cam || !transform) return false;

    *view = cam->view_matrix;
    *projection = cam->projection_matrix;
    *position = transform->position;

    return true;
}

