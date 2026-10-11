#ifndef RE4_BRANCH_LOGIC_H
#define RE4_BRANCH_LOGIC_H
#include <stdint.h>
#include <string.h>
/* CALL rel32 and tail-JMP rel32 share the displacement layout. Keep the opcode. */
static inline int re4_relative_target(const unsigned char *instruction, uint32_t rva,
        uint32_t image_size, uint32_t *target_rva) {
    if (image_size < 5 || rva > image_size - 5
            || (instruction[0] != 0xE8 && instruction[0] != 0xE9)) return 0;
    uint32_t displacement;
    memcpy(&displacement, instruction + 1, 4);
    uint32_t target = rva + 5 + displacement;
    if (target >= image_size) return 0;
    *target_rva = target;
    return 1;
}
/* Multiple render callers are valid only if every CALL has one shared target.
   Never accept a majority, skip an invalid candidate or choose the first. */
static inline int re4_shared_call_target(const unsigned char *const *calls,
        const uint32_t *rvas, unsigned count, uint32_t size, uint32_t *target) {
    if (!count || count > 32) return 0;
    uint32_t common = 0;
    for (unsigned i=0;i<count;++i) {
        uint32_t next;
        if (calls[i][0] != 0xE8 || !re4_relative_target(calls[i],rvas[i],size,&next)
                || (i && next != common)) return 0;
        common = next;
    }
    *target = common;
    return 1;
}
/* Explicit UHD1.1.0 layout observed in the user's read-only probe. Reference
   re4_tweaks Game.cpp identifies CameraCurrentProjection at VA0x59F3A0,
   i.e. RVA0x19F3A0 with its preferred image base0x400000. Never use a vote.
   Require the entire seven-call layout and both E9 thunk destinations. */
static inline int re4_observed_projection_profile(const uint32_t *calls,
        const uint32_t *thunks, const uint32_t *destinations, unsigned count,
        unsigned *selected, unsigned *selected_count, uint32_t *original) {
    static const uint32_t observed[] = {0x1e7110,0x1e8697,0x1e8f4a,0x1ebb47,0x1f43d3,0x1f7bad,0x205dad};
    if (count != 7) return 0;
    unsigned seen = 0, result_count = 0;
    for (unsigned i=0;i<count;++i) {
        unsigned j;
        for (j=0;j<7 && calls[i]!=observed[j];++j) {}
        if (j==7 || (seen & (1u<<j))) return 0;
        seen |= 1u<<j;
        if (j==5) {
            if (thunks[i]!=0x344f || destinations[i]!=0x202f60) return 0;
        } else {
            if (thunks[i]!=0xe395 || destinations[i]!=0x19f3a0) return 0;
            selected[result_count++] = i;
        }
    }
    if (seen!=0x7f || result_count!=6) return 0;
    *selected_count = result_count; *original = 0x19f3a0;
    return 1;
}
#endif
