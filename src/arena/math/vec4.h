#ifndef ARENA_VEC4_H
#define ARENA_VEC4_H

#include <math.h>
#include <stdint.h>

// Forward declaration of Vec3 (defined earlier in math.h)
#ifndef ARENA_VEC3_DEFINED
typedef struct Vec3 Vec3;
#endif

// ============================================================================
// Vec4 - 4D Vector (for homogeneous coordinates, colors, frustum planes, etc.)
// ============================================================================

typedef struct Vec4 {
    float x, y, z, w;
} Vec4;

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------

static inline Vec4 vec4(float x, float y, float z, float w) {
    return (Vec4){x, y, z, w};
}

static inline Vec4 vec4_zero(void) {
    return (Vec4){0.0f, 0.0f, 0.0f, 0.0f};
}

static inline Vec4 vec4_one(void) {
    return (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
}

// Create Vec4 from Vec3 with w component
static inline Vec4 vec4_from_vec3(Vec3 v, float w) {
    return (Vec4){v.x, v.y, v.z, w};
}

// Extract Vec3 from Vec4 (ignores w)
static inline Vec3 vec4_to_vec3(Vec4 v) {
    return (Vec3){v.x, v.y, v.z};
}

// Extract Vec3 with perspective divide (x/w, y/w, z/w)
static inline Vec3 vec4_to_vec3_perspective(Vec4 v) {
    float inv_w = 1.0f / v.w;
    return (Vec3){v.x * inv_w, v.y * inv_w, v.z * inv_w};
}

// ----------------------------------------------------------------------------
// Arithmetic Operations
// ----------------------------------------------------------------------------

static inline Vec4 vec4_add(Vec4 a, Vec4 b) {
    return (Vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static inline Vec4 vec4_sub(Vec4 a, Vec4 b) {
    return (Vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static inline Vec4 vec4_mul(Vec4 a, Vec4 b) {
    return (Vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

static inline Vec4 vec4_scale(Vec4 v, float s) {
    return (Vec4){v.x * s, v.y * s, v.z * s, v.w * s};
}

static inline Vec4 vec4_negate(Vec4 v) {
    return (Vec4){-v.x, -v.y, -v.z, -v.w};
}

// ----------------------------------------------------------------------------
// Vector Operations
// ----------------------------------------------------------------------------

static inline float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline float vec4_length_sq(Vec4 v) {
    return vec4_dot(v, v);
}

static inline float vec4_length(Vec4 v) {
    return sqrtf(vec4_length_sq(v));
}

static inline Vec4 vec4_normalize(Vec4 v) {
    float len = vec4_length(v);
    if (len < 1e-6f) return vec4_zero();
    return vec4_scale(v, 1.0f / len);
}

static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, float t) {
    return (Vec4){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
}

// ----------------------------------------------------------------------------
// Color Operations (Vec4 as RGBA)
// ----------------------------------------------------------------------------

// Create color from 0-255 values
static inline Vec4 vec4_color_rgba8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (Vec4){r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

// Create color from hex (0xRRGGBBAA)
static inline Vec4 vec4_color_hex(uint32_t hex) {
    return (Vec4){
        ((hex >> 24) & 0xFF) / 255.0f,
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f
    };
}

// Common colors
static inline Vec4 vec4_white(void) { return (Vec4){1.0f, 1.0f, 1.0f, 1.0f}; }
static inline Vec4 vec4_black(void) { return (Vec4){0.0f, 0.0f, 0.0f, 1.0f}; }
static inline Vec4 vec4_red(void)   { return (Vec4){1.0f, 0.0f, 0.0f, 1.0f}; }
static inline Vec4 vec4_green(void) { return (Vec4){0.0f, 1.0f, 0.0f, 1.0f}; }
static inline Vec4 vec4_blue(void)  { return (Vec4){0.0f, 0.0f, 1.0f, 1.0f}; }

// ----------------------------------------------------------------------------
// Frustum Plane Operations
// ----------------------------------------------------------------------------

// Normalize a frustum plane (xyz = normal, w = distance)
static inline Vec4 vec4_normalize_plane(Vec4 plane) {
    float len = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    if (len < 1e-6f) return vec4_zero();
    return vec4_scale(plane, 1.0f / len);
}

// Distance from point to plane (positive = in front, negative = behind)
static inline float vec4_plane_distance(Vec4 plane, Vec3 point) {
    return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

// ----------------------------------------------------------------------------
// Mat4 * Vec4 (defined here after Vec4 is complete)
// ----------------------------------------------------------------------------

// Forward declare Mat4 (already defined in mat4.h which is included before vec4.h)
#ifndef ARENA_MAT4_MUL_VEC4
#define ARENA_MAT4_MUL_VEC4

typedef struct Mat4 Mat4;

static inline Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){
        m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
        m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
        m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
        m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w
    };
}

#endif // ARENA_MAT4_MUL_VEC4

#endif

