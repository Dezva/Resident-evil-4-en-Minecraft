/* Bounded PE32 import lookup. Does not scan heaps, execute pointers or write. */
#ifndef RE4_CAPTURE_IMPORT_H
#define RE4_CAPTURE_IMPORT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef const unsigned char *(*CaptureRead)(void *,uint32_t,size_t);
static const unsigned char *capture_bytes(CaptureRead read,void *ctx,uint32_t size,uint32_t offset,size_t bytes) {
    if (offset>size || bytes>size-offset) return NULL;
    return read(ctx,offset,bytes);
}
static uint16_t capture_u16(const unsigned char *p) {uint16_t n;memcpy(&n,p,2);return n;}
static uint32_t capture_u32(const unsigned char *p) {uint32_t n;memcpy(&n,p,4);return n;}
static int capture_name(CaptureRead read,void *ctx,uint32_t size,uint32_t offset,const char *name,int insensitive) {
    size_t length=strlen(name);
    const unsigned char *p=capture_bytes(read,ctx,size,offset,length+1);
    if (!p) return 0;
    for (size_t i=0;i<=length;++i) {
        unsigned char a=p[i],b=(unsigned char)name[i];
        if (insensitive && a>='A' && a<='Z') a=(unsigned char)(a+32);
        if (a!=b) return 0;
    }
    return 1;
}
/* 1 unique unchanged import; 2 not imported; 3 invalid/conflicting/ambiguous.
 * Output is an RVA into the main image, never an address inferred from data. */
static int capture_find_import(CaptureRead read,void *ctx,uint32_t size,uint32_t expected,uint32_t *slot) {
    const unsigned char *p=capture_bytes(read,ctx,size,0,64);
    if (!p || capture_u16(p)!=0x5a4d) return 3;
    uint32_t pe=capture_u32(p+0x3c);
    p=capture_bytes(read,ctx,size,pe,248);
    if (!p || capture_u32(p)!=0x4550 || capture_u16(p+4)!=0x14c
            || capture_u16(p+20)!=224 || capture_u16(p+24)!=0x10b
            || capture_u32(p+80)!=size || capture_u32(p+116)<2) return 3;
    uint32_t imports=capture_u32(p+128),length=capture_u32(p+132);
    if (!imports && !length) return 2;
    if (!imports || length<20 || length>131072 || imports>size || length>size-imports) return 3;
    unsigned matches=0;uint32_t candidate=0;int terminated=0;
    for (uint32_t i=0;i<length/20 && i<256;++i) {
        p=capture_bytes(read,ctx,size,imports+i*20,20);if (!p) return 3;
        uint32_t names=capture_u32(p),name=capture_u32(p+12),iat=capture_u32(p+16);
        if (!names && !name && !iat && !capture_u32(p+4) && !capture_u32(p+8)) {terminated=1;break;}
        if (!capture_name(read,ctx,size,name,"d3d9.dll",1)) continue;
        if (!names || !iat || (names&3u) || (iat&3u) || names>=size || iat>=size) return 3;
        int end=0;
        for (unsigned j=0;j<4096;++j) {
            uint32_t delta=j*4;
            if (delta>size-names || delta>size-iat) return 3;
            p=capture_bytes(read,ctx,size,names+delta,4);if (!p) return 3;
            uint32_t entry=capture_u32(p);
            p=capture_bytes(read,ctx,size,iat+delta,4);if (!p) return 3;
            if (!entry) {end=1;break;}
            if ((entry&0x80000000u) || entry>size || size-entry<2) continue;
            if (capture_name(read,ctx,size,entry+2,"Direct3DCreate9",0)) {
                if (capture_u32(p)!=expected) return 3;
                matches++;candidate=iat+delta;
            }
        }
        if (!end) return 3;
    }
    if (!terminated || matches>1) return 3;
    if (!matches) return 2;
    *slot=candidate;return 1;
}
#endif
