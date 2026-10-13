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
/* FP follows Minecraft's newest gaze directly. A 30-degree queue made the
   displayed ray lag behind the placement ray after larger mouse turns. */
static inline float re4_take_view_yaw(float total,float *consumed){
    if(!isfinite(total) || !isfinite(*consumed) || fabsf(total)>10000000.f)return 0;
    float delta=total-*consumed;*consumed=total;
    return -delta*(3.14159265358979323846f/180.f);
}
static inline float re4_wrap_yaw(float yaw) {
    return remainderf(yaw, 6.2831853071795864769f);
}
/* RE4 yaw: forward=(sin(yaw),cos(yaw)); right=(-cos(yaw),sin(yaw)). */
static inline float re4_walk_heading(float gaze, unsigned buttons) {
    int forward = !!(buttons & 1u) - !!(buttons & 2u);
    int right = !!(buttons & 4u) - !!(buttons & 8u);
    return re4_wrap_yaw(gaze - atan2f((float)right,(float)forward));
}
/* CAMERA uses right/up/back columns; v_mat is its rigid inverse. */
static inline void re4_view_frame(const float *eye, float yaw, float pitch,
        float *camera, float *view, float *right, float *up, float *back) {
    float p = pitch * (3.14159265358979323846f / 180.f);
    float s = sinf(yaw), c = cosf(yaw), sp = sinf(p), cp = cosf(p);
    right[0] = -c; right[1] = 0; right[2] = s;
    up[0] = s*sp; up[1] = cp; up[2] = c*sp;
    back[0] = -s*cp; back[1] = sp; back[2] = -c*cp;
    for (int row=0;row<3;++row) {
        camera[4*row] = right[row]; camera[4*row+1] = up[row];
        camera[4*row+2] = back[row]; camera[4*row+3] = eye[row];
        view[row] = right[row]; view[4+row] = up[row]; view[8+row] = back[row];
    }
    view[3] = -(right[0]*eye[0]+right[1]*eye[1]+right[2]*eye[2]);
    view[7] = -(up[0]*eye[0]+up[1]*eye[1]+up[2]*eye[2]);
    view[11] = -(back[0]*eye[0]+back[1]*eye[1]+back[2]*eye[2]);
}
static inline int re4_valid_look(float total, float pitch, float eye, unsigned flags) {
    return !(flags & ~15u) && (!(flags&4u) || ((flags&3u)==3u && fabsf(total)<=180.f)) && (!(flags & 2u) || (flags & 1u))
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
    /* The caller adds world translation. A unit target loses float precision
       near RE4's +/-100000 coordinates; retain a 1000-unit sight line. */
    target[0] = 1000.f * sinf(yaw) * cosf(radians);
    target[1] = -1000.f * sinf(radians);
    target[2] = 1000.f * cosf(yaw) * cosf(radians);
    return 1;
}
static inline unsigned re4_smoothing_flags(unsigned flags, int first_person) {
    return first_person ? (flags & ~0x40u) : flags;
}
static inline int re4_distinct_model(unsigned object, unsigned previous) {
    return object && object != previous;
}
static inline int re4_model_slot_matches(unsigned saved_controller, unsigned controller,
        unsigned saved_object, unsigned object) {
    return controller && controller == saved_controller && object && object == saved_object;
}
/* cUnit drawing bits only: never clear Alive, Move or any collision state. */
#define RE4_MODEL_DRAW_BITS 0x12u
static inline unsigned re4_hide_model(unsigned flags) { return flags & ~RE4_MODEL_DRAW_BITS; }
static inline unsigned re4_restore_model(unsigned flags, unsigned saved) {
    return (flags & ~RE4_MODEL_DRAW_BITS) | (saved & RE4_MODEL_DRAW_BITS);
}
#endif
