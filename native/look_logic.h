#ifndef RE4_LOOK_LOGIC_H
#define RE4_LOOK_LOGIC_H
#include <math.h>
/* Cumulative samples survive the latest-frame slot and are consumed only once. */
static inline float re4_take_yaw(float total, float *consumed) {
    if (!isfinite(total) || !isfinite(*consumed) || fabsf(total) > 10000000.f) return 0;
    float delta = total - *consumed;
    if (delta > 30.f) delta = 30.f;
    if (delta < -30.f) delta = -30.f;
    *consumed += delta;
    return -delta * (3.14159265358979323846f / 180.f);
}
static inline float re4_wrap_yaw(float yaw) {
    return remainderf(yaw, 6.2831853071795864769f);
}
static inline int re4_valid_look(float total, float pitch, float eye, unsigned flags) {
    return !(flags & ~3u) && (!(flags & 2u) || (flags & 1u))
        && isfinite(total) && fabsf(total) <= 10000000.f
        && isfinite(pitch) && fabsf(pitch) <= 85.f
        && isfinite(eye) && fabsf(eye) >= 40.f && fabsf(eye) <= 2500.f;
}
/* World-axis eye matrix: yaw belongs to the sight line, never to an orbit offset. */
static inline int re4_eye_frame(const float *feet, float yaw, float pitch, float eye,
        float *matrix, float *position, float *target) {
    if (!isfinite(yaw) || fabsf(yaw) > 100000000.f) return 0;
    for (int i = 0; i < 3; ++i)
        if (!isfinite(feet[i]) || fabsf(feet[i]) > 100000000.f) return 0;
    float radians = pitch * (3.14159265358979323846f / 180.f);
    for (int i = 0; i < 12; ++i) matrix[i] = 0;
    matrix[0] = matrix[5] = matrix[10] = 1;
    matrix[3] = feet[0]; matrix[7] = feet[1] + eye; matrix[11] = feet[2];
    position[0] = position[1] = position[2] = 0;
    target[0] = sinf(yaw) * cosf(radians);
    target[1] = -sinf(radians);
    target[2] = cosf(yaw) * cosf(radians);
    return 1;
}
/* cUnit drawing bits only: never clear Alive, Move or any collision state. */
#define RE4_MODEL_DRAW_BITS 0x12u
static inline unsigned re4_hide_model(unsigned flags) { return flags & ~RE4_MODEL_DRAW_BITS; }
static inline unsigned re4_restore_model(unsigned flags, unsigned saved) {
    return (flags & ~RE4_MODEL_DRAW_BITS) | (saved & RE4_MODEL_DRAW_BITS);
}
#endif
