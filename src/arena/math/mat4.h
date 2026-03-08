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

// Note: mat4_mul_vec4 is defined in vec4.h after Vec4 is fully defined

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

// ----------------------------------------------------------------------------
// Matrix Inverse (using adjugate method)
// ----------------------------------------------------------------------------

static inline float mat4_determinant(Mat4 m) {
    float a = m.m[0][0], b = m.m[1][0], c = m.m[2][0], d = m.m[3][0];
    float e = m.m[0][1], f = m.m[1][1], g = m.m[2][1], h = m.m[3][1];
    float i = m.m[0][2], j = m.m[1][2], k = m.m[2][2], l = m.m[3][2];
    float M = m.m[0][3], n = m.m[1][3], o = m.m[2][3], p = m.m[3][3];

    float kp_lo = k * p - l * o;
    float jp_ln = j * p - l * n;
    float jo_kn = j * o - k * n;
    float ip_lm = i * p - l * M;
    float io_km = i * o - k * M;
    float in_jm = i * n - j * M;

    return a * (f * kp_lo - g * jp_ln + h * jo_kn)
         - b * (e * kp_lo - g * ip_lm + h * io_km)
         + c * (e * jp_ln - f * ip_lm + h * in_jm)
         - d * (e * jo_kn - f * io_km + g * in_jm);
}

static inline Mat4 mat4_inverse(Mat4 m) {
    float a = m.m[0][0], b = m.m[1][0], c = m.m[2][0], d = m.m[3][0];
    float e = m.m[0][1], f = m.m[1][1], g = m.m[2][1], h = m.m[3][1];
    float i = m.m[0][2], j = m.m[1][2], k = m.m[2][2], l = m.m[3][2];
    float M = m.m[0][3], n = m.m[1][3], o = m.m[2][3], p = m.m[3][3];

    float kp_lo = k * p - l * o, jp_ln = j * p - l * n, jo_kn = j * o - k * n;
    float ip_lm = i * p - l * M, io_km = i * o - k * M, in_jm = i * n - j * M;
    float gp_ho = g * p - h * o, fp_hn = f * p - h * n, fo_gn = f * o - g * n;
    float ep_hm = e * p - h * M, eo_gm = e * o - g * M, en_fm = e * n - f * M;
    float gl_hk = g * l - h * k, fl_hj = f * l - h * j, fk_gj = f * k - g * j;
    float el_hi = e * l - h * i, ek_gi = e * k - g * i, ej_fi = e * j - f * i;

    float det = a * (f * kp_lo - g * jp_ln + h * jo_kn)
              - b * (e * kp_lo - g * ip_lm + h * io_km)
              + c * (e * jp_ln - f * ip_lm + h * in_jm)
              - d * (e * jo_kn - f * io_km + g * in_jm);

    if (fabsf(det) < 1e-10f) return mat4_identity(); // Singular matrix

    float inv_det = 1.0f / det;

    Mat4 result;
    result.m[0][0] =  (f * kp_lo - g * jp_ln + h * jo_kn) * inv_det;
    result.m[0][1] = -(e * kp_lo - g * ip_lm + h * io_km) * inv_det;
    result.m[0][2] =  (e * jp_ln - f * ip_lm + h * in_jm) * inv_det;
    result.m[0][3] = -(e * jo_kn - f * io_km + g * in_jm) * inv_det;
    result.m[1][0] = -(b * kp_lo - c * jp_ln + d * jo_kn) * inv_det;
    result.m[1][1] =  (a * kp_lo - c * ip_lm + d * io_km) * inv_det;
    result.m[1][2] = -(a * jp_ln - b * ip_lm + d * in_jm) * inv_det;
    result.m[1][3] =  (a * jo_kn - b * io_km + c * in_jm) * inv_det;
    result.m[2][0] =  (b * gp_ho - c * fp_hn + d * fo_gn) * inv_det;
    result.m[2][1] = -(a * gp_ho - c * ep_hm + d * eo_gm) * inv_det;
    result.m[2][2] =  (a * fp_hn - b * ep_hm + d * en_fm) * inv_det;
    result.m[2][3] = -(a * fo_gn - b * eo_gm + c * en_fm) * inv_det;
    result.m[3][0] = -(b * gl_hk - c * fl_hj + d * fk_gj) * inv_det;
    result.m[3][1] =  (a * gl_hk - c * el_hi + d * ek_gi) * inv_det;
    result.m[3][2] = -(a * fl_hj - b * el_hi + d * ej_fi) * inv_det;
    result.m[3][3] =  (a * fk_gj - b * ek_gi + c * ej_fi) * inv_det;
    return result;
}

// Fast inverse for orthonormal matrices (rotation + translation only)
static inline Mat4 mat4_inverse_orthonormal(Mat4 m) {
    // For orthonormal matrices: inverse = transpose of rotation, negated translation
    Mat4 result = mat4_identity();
    // Transpose 3x3 rotation part
    result.m[0][0] = m.m[0][0]; result.m[0][1] = m.m[1][0]; result.m[0][2] = m.m[2][0];
    result.m[1][0] = m.m[0][1]; result.m[1][1] = m.m[1][1]; result.m[1][2] = m.m[2][1];
    result.m[2][0] = m.m[0][2]; result.m[2][1] = m.m[1][2]; result.m[2][2] = m.m[2][2];
    // Negate and transform translation
    Vec3 trans = {m.m[3][0], m.m[3][1], m.m[3][2]};
    result.m[3][0] = -vec3_dot((Vec3){result.m[0][0], result.m[1][0], result.m[2][0]}, trans);
    result.m[3][1] = -vec3_dot((Vec3){result.m[0][1], result.m[1][1], result.m[2][1]}, trans);
    result.m[3][2] = -vec3_dot((Vec3){result.m[0][2], result.m[1][2], result.m[2][2]}, trans);
    return result;
}

#endif
