#ifndef RE4_RUNTIME_POLICY_H
#define RE4_RUNTIME_POLICY_H
#include <stdint.h>
/* Pinned GLOBAL_WK::Routine0 and Flags_STATUS: FMV, events, death and
   camera transitions. CINESCO is also present during normal widescreen play;
   neither it nor a contextual action prompt proves an active cutscene. */
#define RE4_SCRIPTED_MASK (0x10000000u|0x08000000u|0x04000000u|0x00100000u|0x00001000u)
static inline unsigned re4_scene_code(unsigned routine,uint32_t flags){return routine==3 && !(flags&RE4_SCRIPTED_MASK)?0u:1u;}
/* UHD primary contextual input: check, pickup, action and confirmation.
   No fire/aim; skip is separately gated to native movies/events. */
#define RE4_ACTION_BITS (UINT64_C(0x400)|UINT64_C(0x4000000000)|UINT64_C(0x10000000000)|UINT64_C(0x80000000))
#define RE4_SKIP_BITS UINT64_C(0x20000000) /* KEY_EV_CANCEL */
#define RE4_BORROWED_BITS (RE4_ACTION_BITS|RE4_SKIP_BITS)
#define RE4_ACTION_HOLD_MS 120u
static inline int re4_action_pulse(uint32_t now,uint32_t stamp,uint32_t generation,uint32_t request_generation,
    uint32_t id,uint32_t *last_generation,uint32_t *last_id,unsigned live,unsigned menu){
    if(!live || menu || !id || generation!=request_generation || now-stamp>=500)return 0;
    if(*last_generation!=generation){*last_generation=generation;*last_id=0;}
    if(id<=*last_id)return 0;
    *last_id=id;return 1;
}
#endif
