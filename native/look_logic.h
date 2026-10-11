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
        && isfinite(eye) && fabsf(eye) >= 40.f && fabsf(eye) <= 250.f;
}
/* Camera-point coordinates relative to the game's player matrix. */
static inline void re4_eye_point(float pitch, float eye, float *position, float *target) {
    float radians = pitch * (3.14159265358979323846f / 180.f);
    position[0] = 0; position[1] = eye; position[2] = 0;
    target[0] = 0; target[1] = eye - sinf(radians); target[2] = cosf(radians);
}
#endif
