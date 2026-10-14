#ifndef RE4_RUNTIME_DISCOVERY_H
#define RE4_RUNTIME_DISCOVERY_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Several verified code sites may reference the same static cEmMgr.
   Uniqueness belongs to the operand, not to the instruction's location. */
static inline uint32_t re4_shared_operand(unsigned char *const *sites,unsigned count,
        size_t offset,uintptr_t base,uint32_t size,size_t bytes){
    uint32_t result=0;
    if(!count || count>64 || size<bytes)return 0;
    for(unsigned i=0;i<count;i++){
        uint32_t address;memcpy(&address,sites[i]+offset,4);
        if((address&3u) || address<base || address>base+size-bytes
            || (result && result!=address))return 0;
        result=address;
    }
    return result;
}
#endif
