#ifndef RE4_CONTROL_LOGIC_H
#define RE4_CONTROL_LOGIC_H
#include <stdint.h>
#define RE4_MAGIC 0x52453443u
#define RE4_TTL_MS 500u
#define RE4_MOVEMENT_MASK 15u
#define RE4_RUN 64u /* KEY_RUN in the pinned UHD SDK/pad.h reference. */
#define RE4_CONTROL_MASK (RE4_MOVEMENT_MASK | RE4_RUN)
/* Reuse a complete frame only within its original lease; never extend stamp.
   Live disarm/stop dominate a cached armed frame, even during a torn write. */
static inline int re4_cached_frame_usable(unsigned have, uint32_t now,
        uint32_t stamp, unsigned cached_armed, unsigned live_armed, unsigned stop) {
    return have && cached_armed && live_armed && !stop && (uint32_t)(now-stamp)<RE4_TTL_MS;
}
/* Wrap-safe tick arithmetic; reject torn writes, expired input and opposite keys. */
static inline uint32_t re4_buttons(uint32_t sequence_before, uint32_t sequence_after,
        uint32_t now, uint32_t stamp, uint32_t armed, uint32_t stop, uint32_t buttons) {
    if ((sequence_before & 1u) || sequence_before != sequence_after || !armed || stop
            || (uint32_t)(now - stamp) >= RE4_TTL_MS || (buttons & ~RE4_CONTROL_MASK)) return 0;
    if ((buttons & 3u) == 3u) buttons &= ~3u;
    if ((buttons & 12u) == 12u) buttons &= ~12u;
    if (!(buttons & RE4_MOVEMENT_MASK)) buttons &= ~RE4_RUN;
    return buttons;
}
static inline uint32_t re4_relative_input(uint32_t buttons) {
    return (buttons & RE4_MOVEMENT_MASK) ? (1u | (buttons & RE4_RUN)) : 0u;
}
#endif
