#ifndef ARENA_MAT4_H
#define ARENA_MAT4_H

#include <math.h>

// ============================================================================
// Mat4 - 4x4 Column-Major Matrix
// ============================================================================
// Memory layout: m[col][row] for column-major (OpenGL/Vulkan compatible)
// m[0] = first column, m[1] = second column, etc.

typedef struct Mat4 {
    float m[4][4];
} Mat4;

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------

static inline Mat4 mat4_identity(void) {
    Mat4 result = {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}};
    return result;
}

static inline Mat4 mat4_zero(void) {
    Mat4 result = {{{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}};
    return result;
}

// ----------------------------------------------------------------------------
// Operations
// ----------------------------------------------------------------------------

static inline Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 result = mat4_zero();
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            result.m[c][r] = a.m[0][r] * b.m[c][0] +
                             a.m[1][r] * b.m[c][1] +
                             a.m[2][r] * b.m[c][2] +
                             a.m[3][r] * b.m[c][3];
        }
    }
    return result;
}

static inline Vec3 mat4_mul_vec3(Mat4 m, Vec3 v, float w) {
    return (Vec3){
        m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * w,
        m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * w,
        m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * w
    };
}

static inline Mat4 mat4_transpose(Mat4 m) {
    Mat4 result;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            result.m[c][r] = m.m[r][c];
        }
    }
    return result;
}

// ----------------------------------------------------------------------------
// Transform Constructors
// ----------------------------------------------------------------------------

static inline Mat4 mat4_translate(Vec3 t) {
    Mat4 result = mat4_identity();
    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;
    return result;
}

static inline Mat4 mat4_scale(Vec3 s) {
    Mat4 result = mat4_identity();
    result.m[0][0] = s.x;
    result.m[1][1] = s.y;
    result.m[2][2] = s.z;
    return result;
}

static inline Mat4 mat4_rotate_x(float angle) {
    float c = cosf(angle), s = sinf(angle);
    Mat4 result = mat4_identity();
    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    return result;
}

static inline Mat4 mat4_rotate_y(float angle) {
    float c = cosf(angle), s = sinf(angle);
    Mat4 result = mat4_identity();
    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[2][0] = s;
    result.m[2][2] = c;
    return result;
}

static inline Mat4 mat4_rotate_z(float angle) {
    float c = cosf(angle), s = sinf(angle);
    Mat4 result = mat4_identity();
    result.m[0][0] = c;
    result.m[0][1] = s;
    result.m[1][0] = -s;
    result.m[1][1] = c;
    return result;
}

// ----------------------------------------------------------------------------
// Projection Matrices
// ----------------------------------------------------------------------------

static inline Mat4 mat4_ortho(float left, float right, float bottom, float top,
                               float near, float far) {
    Mat4 result = mat4_identity();
    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[2][2] = -2.0f / (far - near);
    result.m[3][0] = -(right + left) / (right - left);
    result.m[3][1] = -(top + bottom) / (top - bottom);
    result.m[3][2] = -(far + near) / (far - near);
    return result;
}

static inline Mat4 mat4_perspective(float fov_y, float aspect, float near, float far) {
    float tan_half_fov = tanf(fov_y * 0.5f);
    Mat4 result = mat4_zero();
    result.m[0][0] = 1.0f / (aspect * tan_half_fov);
    result.m[1][1] = 1.0f / tan_half_fov;
    result.m[2][2] = -(far + near) / (far - near);
    result.m[2][3] = -1.0f;
    result.m[3][2] = -(2.0f * far * near) / (far - near);
    return result;
}

static inline Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(target, eye));
    Vec3 r = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(r, f);

    Mat4 result = mat4_identity();
    result.m[0][0] = r.x;  result.m[1][0] = r.y;  result.m[2][0] = r.z;
    result.m[0][1] = u.x;  result.m[1][1] = u.y;  result.m[2][1] = u.z;
    result.m[0][2] = -f.x; result.m[1][2] = -f.y; result.m[2][2] = -f.z;
    result.m[3][0] = -vec3_dot(r, eye);
    result.m[3][1] = -vec3_dot(u, eye);
    result.m[3][2] = vec3_dot(f, eye);
    return result;
}

#endif
