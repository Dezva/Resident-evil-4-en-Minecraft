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
#endif
