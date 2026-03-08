#ifndef ARENA_QUAT_H
#define ARENA_QUAT_H

#include <math.h>

// ============================================================================
// Quat - Quaternion for Rotations
// ============================================================================
// Stored as (x, y, z, w) where w is the scalar component

typedef struct Quat {
    float x, y, z, w;
} Quat;

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------

static inline Quat quat(float x, float y, float z, float w) {
    return (Quat){x, y, z, w};
}

static inline Quat quat_identity(void) {
    return (Quat){0.0f, 0.0f, 0.0f, 1.0f};
}

static inline Quat quat_from_axis_angle(Vec3 axis, float angle) {
    float half_angle = angle * 0.5f;
    float s = sinf(half_angle);
    Vec3 n = vec3_normalize(axis);
    return (Quat){n.x * s, n.y * s, n.z * s, cosf(half_angle)};
}

static inline Quat quat_from_euler(float pitch, float yaw, float roll) {
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f),   sy = sinf(yaw * 0.5f);
    float cr = cosf(roll * 0.5f),  sr = sinf(roll * 0.5f);
    return (Quat){
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
    };
}

// ----------------------------------------------------------------------------
// Operations
// ----------------------------------------------------------------------------

static inline float quat_length_sq(Quat q) {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

static inline float quat_length(Quat q) {
    return sqrtf(quat_length_sq(q));
}

static inline Quat quat_normalize(Quat q) {
    float len = quat_length(q);
    if (len < ARENA_EPSILON) return quat_identity();
    float inv = 1.0f / len;
    return (Quat){q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

static inline Quat quat_conjugate(Quat q) {
    return (Quat){-q.x, -q.y, -q.z, q.w};
}

static inline Quat quat_inverse(Quat q) {
    float len_sq = quat_length_sq(q);
    if (len_sq < ARENA_EPSILON) return quat_identity();
    float inv = 1.0f / len_sq;
    return (Quat){-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv};
}

static inline Quat quat_mul(Quat a, Quat b) {
    return (Quat){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

static inline float quat_dot(Quat a, Quat b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    Vec3 qv = {q.x, q.y, q.z};
    Vec3 uv = vec3_cross(qv, v);
    Vec3 uuv = vec3_cross(qv, uv);
    uv = vec3_scale(uv, 2.0f * q.w);
    uuv = vec3_scale(uuv, 2.0f);
    return vec3_add(vec3_add(v, uv), uuv);
}

// ----------------------------------------------------------------------------
// Interpolation
// ----------------------------------------------------------------------------

static inline Quat quat_lerp(Quat a, Quat b, float t) {
    return (Quat){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
}

static inline Quat quat_slerp(Quat a, Quat b, float t) {
    float dot = quat_dot(a, b);

    // If negative dot, negate one quaternion to take shorter path
    if (dot < 0.0f) {
        b = (Quat){-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }

    // If quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        return quat_normalize(quat_lerp(a, b, t));
    }

    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sin_theta;
    float wb = sinf(t * theta) / sin_theta;

    return (Quat){
        a.x * wa + b.x * wb,
        a.y * wa + b.y * wb,
        a.z * wa + b.z * wb,
        a.w * wa + b.w * wb
    };
}

// ----------------------------------------------------------------------------
// Conversion to Matrix
// ----------------------------------------------------------------------------

static inline Mat4 quat_to_mat4(Quat q) {
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    Mat4 result = mat4_identity();
    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy + wz);
    result.m[0][2] = 2.0f * (xz - wy);
    result.m[1][0] = 2.0f * (xy - wz);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz + wx);
    result.m[2][0] = 2.0f * (xz + wy);
    result.m[2][1] = 2.0f * (yz - wx);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);
    return result;
}

// ----------------------------------------------------------------------------
// Look At (create quaternion that looks from eye to target)
// ----------------------------------------------------------------------------

static inline Quat quat_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 forward = vec3_normalize(vec3_sub(target, eye));
    Vec3 right = vec3_normalize(vec3_cross(up, forward));
    Vec3 actual_up = vec3_cross(forward, right);

    // Build rotation matrix from axes
    float m00 = right.x, m01 = right.y, m02 = right.z;
    float m10 = actual_up.x, m11 = actual_up.y, m12 = actual_up.z;
    float m20 = forward.x, m21 = forward.y, m22 = forward.z;

    float trace = m00 + m11 + m22;
    Quat q;

    if (trace > 0) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m12 - m21) * s;
        q.y = (m20 - m02) * s;
        q.z = (m01 - m10) * s;
    } else if (m00 > m11 && m00 > m22) {
        float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
        q.w = (m12 - m21) / s;
        q.x = 0.25f * s;
        q.y = (m10 + m01) / s;
        q.z = (m20 + m02) / s;
    } else if (m11 > m22) {
        float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
        q.w = (m20 - m02) / s;
        q.x = (m10 + m01) / s;
        q.y = 0.25f * s;
        q.z = (m21 + m12) / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
        q.w = (m01 - m10) / s;
        q.x = (m20 + m02) / s;
        q.y = (m21 + m12) / s;
        q.z = 0.25f * s;
    }

    return quat_normalize(q);
}

#endif
