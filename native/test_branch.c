#include "branch_logic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    /* Actual read-only bytes/RVAs supplied by the user's RE4 UHD 1.1.0. */
    unsigned char jump[] = {0xE9, 0x52, 0xFE, 0x55, 0x00};
    uint32_t target = 0;
    assert(re4_relative_target(jump, 0x7509, 0xA00000, &target));
    assert(target == 0x567360 && jump[0] == 0xE9);
    unsigned char call[] = {0xE8, 0x52, 0xFE, 0x55, 0x00};
    assert(re4_relative_target(call, 0x7509, 0xA00000, &target));
    assert(target == 0x567360 && call[0] == 0xE8);
    unsigned char backward[] = {0xE9, 0xFB, 0xFE, 0xFF, 0xFF};
    assert(re4_relative_target(backward, 0x1000, 0x2000, &target) && target == 0xF00);
    jump[0] = 0x55;
    assert(!re4_relative_target(jump, 0x7509, 0xA00000, &target));
    assert(!re4_relative_target(call, 0x7509, 0x10000, &target));
    assert(!re4_relative_target(call, 0xA00000 - 4, 0xA00000, &target));
    unsigned char sites[7][5];
    const unsigned char *pointers[7]; uint32_t rvas[7];
    for (unsigned i=0;i<7;++i) {
        sites[i][0] = 0xE8; rvas[i] = 0x1000+32*i;
        uint32_t displacement = 0x1800-(rvas[i]+5);
        memcpy(sites[i]+1,&displacement,4); pointers[i] = sites[i];
    }
    assert(re4_shared_call_target(pointers,rvas,7,0x2000,&target) && target == 0x1800);
    assert(!re4_shared_call_target(pointers,rvas,0,0x2000,&target));
    sites[6][1]++; /* Six matching candidates must not outvote another target. */
    assert(!re4_shared_call_target(pointers,rvas,7,0x2000,&target));
    sites[6][1]--; sites[6][0] = 0xE9;
    assert(!re4_shared_call_target(pointers,rvas,7,0x2000,&target));
    sites[6][0] = 0xE8;
    assert(!re4_shared_call_target(pointers,rvas,7,0x1100,&target));
    puts("Native branch regression: observed E9 target, E8, negative displacement and invalid branches passed");
}
