#ifndef RE4_RUNTIME_POLICY_H
#define RE4_RUNTIME_POLICY_H
#include <stdint.h>
/* Pinned GLOBAL_WK::Routine0 and Flags_STATUS: FMV, events, death and
   camera transitions. CINESCO is also present during normal widescreen play;
   neither it nor a contextual action prompt proves an active cutscene. */
#define RE4_SCRIPTED_MASK (0x10000000u|0x08000000u|0x04000000u|0x00100000u|0x00001000u)
static inline unsigned re4_scene_code(unsigned routine,uint32_t flags){return routine==3 && !(flags&RE4_SCRIPTED_MASK)?0u:1u;}
/* Actual E capture from UHD1.1.0: Key_btn_on/trg=0x80400 (KEY_CK|KEY_A).
   The old synthetic combination omitted KEY_A and was not equivalent to E.
   No fire/aim; skip is separately gated to native movies/events. */
#define RE4_ACTION_BITS (UINT64_C(0x400)|UINT64_C(0x80000))
#define RE4_SKIP_BITS UINT64_C(0x20000000) /* KEY_EV_CANCEL */
#define RE4_DIALOG_MASK (UINT64_C(0xF000000)|UINT64_C(0xC00C0000))
#define RE4_BORROWED_BITS (RE4_ACTION_BITS|RE4_SKIP_BITS|RE4_DIALOG_MASK)
static inline uint64_t re4_dialog_bits(unsigned kind){
 switch(kind){case 2:return UINT64_C(0x1000000);case 3:return UINT64_C(0x2000000);case 4:return UINT64_C(0x8000000);case 5:return UINT64_C(0x4000000);case 6:return UINT64_C(0x80080000);case 7:return UINT64_C(0x40040000);default:return 0;}
}
static inline unsigned re4_dialog_flags(unsigned routine,uint32_t flags,uint32_t flags1){
 return routine==3 && (flags1&2u) && !(flags&(0x10000000u|0x08000000u|0x04000000u|0x00100000u));
}
#define RE4_ACTION_HOLD_MS 120u
static inline int re4_action_pulse(uint32_t now,uint32_t stamp,uint32_t generation,uint32_t request_generation,
    uint32_t id,uint32_t *last_generation,uint32_t *last_id,unsigned live,unsigned menu){
    if(!live || menu || !id || generation!=request_generation || now-stamp>=500)return 0;
    if(*last_generation!=generation){*last_generation=generation;*last_id=0;}
    if(id<=*last_id)return 0;
    *last_id=id;return 1;
}
#endif
