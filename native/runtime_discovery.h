#ifndef RE4_RUNTIME_DISCOVERY_H
#define RE4_RUNTIME_DISCOVERY_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Each operand still originates in the pinned SDK signature. Never select a
   pool by scan order or by a hardcoded RVA. */
static inline unsigned re4_distinct_operands(unsigned char *const *sites,unsigned count,
        size_t offset,uintptr_t base,uint32_t size,size_t bytes,uint32_t *targets,unsigned capacity){
    unsigned used=0;
    if(!count || count>64 || size<bytes || !capacity)return 0;
    for(unsigned i=0;i<count;i++){
        uint32_t address;memcpy(&address,sites[i]+offset,4);
        if((address&3u) || address<base || address>base+size-bytes)return 0;
        unsigned j=0;for(;j<used;j++)if(targets[j]==address)break;
        if(j==used){if(used==capacity)return 0;targets[used++]=address;}
    }
    return used;
}
static inline int re4_cem_pool_header_valid(const uint32_t *header,uintptr_t base,uint32_t size){
    uint32_t vtable=header[0],array=header[1],count=header[2],stride=header[3];
    return vtable>=base && vtable<base+size && count>0 && count<=4096
        && stride>=0x408 && stride<=65536 && !(stride&3u) && array>=0x10000 && !(array&3u)
        && array<=UINT32_MAX-count*stride && (size_t)count*stride<=64u*1024u*1024u;
}
#endif
