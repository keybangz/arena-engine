#ifndef ARENA_MATH_H
#define ARENA_MATH_H

#include <math.h>
#include <stdbool.h>

// ============================================================================
// Constants
// ============================================================================

#define ARENA_PI        3.14159265358979323846f
#define ARENA_TAU       6.28318530717958647692f
#define ARENA_PI_2      1.57079632679489661923f
#define ARENA_DEG2RAD   (ARENA_PI / 180.0f)
#define ARENA_RAD2DEG   (180.0f / ARENA_PI)
#define ARENA_EPSILON   1e-6f

// ============================================================================
// Utility Functions
// ============================================================================

static inline float arena_minf(float a, float b) { return a < b ? a : b; }
static inline float arena_maxf(float a, float b) { return a > b ? a : b; }
static inline float arena_clampf(float v, float min, float max) {
    return arena_minf(arena_maxf(v, min), max);
}
static inline float arena_lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}
static inline float arena_signf(float v) {
    return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f);
}
static inline float arena_absf(float v) { return v < 0.0f ? -v : v; }
static inline bool arena_equalf(float a, float b) {
    return arena_absf(a - b) < ARENA_EPSILON;
}

// ============================================================================
// Vec2
// ============================================================================

typedef struct Vec2 {
    float x, y;
} Vec2;

static inline Vec2 vec2(float x, float y) { return (Vec2){x, y}; }
static inline Vec2 vec2_zero(void) { return (Vec2){0.0f, 0.0f}; }
static inline Vec2 vec2_one(void) { return (Vec2){1.0f, 1.0f}; }

static inline Vec2 vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x + b.x, a.y + b.y}; }
static inline Vec2 vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x - b.x, a.y - b.y}; }
static inline Vec2 vec2_mul(Vec2 a, Vec2 b) { return (Vec2){a.x * b.x, a.y * b.y}; }
static inline Vec2 vec2_scale(Vec2 v, float s) { return (Vec2){v.x * s, v.y * s}; }
static inline Vec2 vec2_negate(Vec2 v) { return (Vec2){-v.x, -v.y}; }

static inline float vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
static inline float vec2_length_sq(Vec2 v) { return vec2_dot(v, v); }
static inline float vec2_length(Vec2 v) { return sqrtf(vec2_length_sq(v)); }
static inline float vec2_distance(Vec2 a, Vec2 b) { return vec2_length(vec2_sub(a, b)); }

static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    if (len < ARENA_EPSILON) return vec2_zero();
    return vec2_scale(v, 1.0f / len);
}

static inline Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){arena_lerpf(a.x, b.x, t), arena_lerpf(a.y, b.y, t)};
}

static inline Vec2 vec2_rotate(Vec2 v, float angle) {
    float c = cosf(angle), s = sinf(angle);
    return (Vec2){v.x * c - v.y * s, v.x * s + v.y * c};
}

// ============================================================================
// Vec3
// ============================================================================

#define ARENA_VEC3_DEFINED
typedef struct Vec3 {
    float x, y, z;
} Vec3;

static inline Vec3 vec3(float x, float y, float z) { return (Vec3){x, y, z}; }
static inline Vec3 vec3_zero(void) { return (Vec3){0.0f, 0.0f, 0.0f}; }
static inline Vec3 vec3_one(void) { return (Vec3){1.0f, 1.0f, 1.0f}; }
static inline Vec3 vec3_up(void) { return (Vec3){0.0f, 1.0f, 0.0f}; }
static inline Vec3 vec3_right(void) { return (Vec3){1.0f, 0.0f, 0.0f}; }
static inline Vec3 vec3_forward(void) { return (Vec3){0.0f, 0.0f, -1.0f}; }

static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3 vec3_mul(Vec3 a, Vec3 b) { return (Vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }
static inline Vec3 vec3_scale(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
static inline Vec3 vec3_negate(Vec3 v) { return (Vec3){-v.x, -v.y, -v.z}; }

static inline float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float vec3_length_sq(Vec3 v) { return vec3_dot(v, v); }
static inline float vec3_length(Vec3 v) { return sqrtf(vec3_length_sq(v)); }
static inline float vec3_distance(Vec3 a, Vec3 b) { return vec3_length(vec3_sub(a, b)); }

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len < ARENA_EPSILON) return vec3_zero();
    return vec3_scale(v, 1.0f / len);
}

static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, float t) {
    return (Vec3){
        arena_lerpf(a.x, b.x, t),
        arena_lerpf(a.y, b.y, t),
        arena_lerpf(a.z, b.z, t)
    };
}

static inline Vec3 vec3_reflect(Vec3 v, Vec3 n) {
    return vec3_sub(v, vec3_scale(n, 2.0f * vec3_dot(v, n)));
}

// Include additional math types
#include "vec3.h"
#include "mat4.h"
#include "vec4.h"
#include "quat.h"

#endif
