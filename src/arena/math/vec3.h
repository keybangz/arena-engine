#ifndef ARENA_VEC3_H
#define ARENA_VEC3_H

// Vec3 is defined in math.h
// This file exists for additional Vec3 utilities if needed

// ----------------------------------------------------------------------------
// Additional Vec3 Utilities
// ----------------------------------------------------------------------------

// Project vector a onto vector b
static inline Vec3 vec3_project(Vec3 a, Vec3 b) {
    float b_len_sq = vec3_length_sq(b);
    if (b_len_sq < ARENA_EPSILON) return vec3_zero();
    return vec3_scale(b, vec3_dot(a, b) / b_len_sq);
}

// Reject vector a from vector b (perpendicular component)
static inline Vec3 vec3_reject(Vec3 a, Vec3 b) {
    return vec3_sub(a, vec3_project(a, b));
}

// Angle between two vectors (in radians)
static inline float vec3_angle(Vec3 a, Vec3 b) {
    float len_a = vec3_length(a);
    float len_b = vec3_length(b);
    if (len_a < ARENA_EPSILON || len_b < ARENA_EPSILON) return 0.0f;
    float cos_angle = vec3_dot(a, b) / (len_a * len_b);
    return acosf(arena_clampf(cos_angle, -1.0f, 1.0f));
}

// Move towards target by max_distance
static inline Vec3 vec3_move_towards(Vec3 current, Vec3 target, float max_distance) {
    Vec3 diff = vec3_sub(target, current);
    float dist = vec3_length(diff);
    if (dist <= max_distance || dist < ARENA_EPSILON) {
        return target;
    }
    return vec3_add(current, vec3_scale(diff, max_distance / dist));
}

#endif
