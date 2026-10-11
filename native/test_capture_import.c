#include <assert.h>
#include <stdio.h>
#include "capture_import.h"
static unsigned char image[65536],original[65536];
static unsigned reads;
static uint32_t denied;
static CaptureImportDetails details;
static const unsigned char *read_image(void *ctx,uint32_t rva,size_t n) {
    (void)ctx;reads++;
    assert(rva<=sizeof(image) && n<=sizeof(image)-rva);
    if (denied && rva<=denied && n>denied-rva) return NULL;
    return image+rva;
}
static void word(uint32_t rva,uint16_t v) {memcpy(image+rva,&v,2);}
static void dword(uint32_t rva,uint32_t v) {memcpy(image+rva,&v,4);}
static void setup(void) {
    memset(image,0,sizeof(image));reads=denied=0;
    word(0,0x5a4d);dword(0x3c,0x80);dword(0x80,0x4550);
    word(0x84,0x14c);word(0x94,224);word(0x98,0x10b);
    dword(0xd0,sizeof(image));dword(0xf4,16);dword(0x100,0x400);dword(0x104,40);
    dword(0x400,0x600);dword(0x40c,0x500);dword(0x410,0x700);
    strcpy((char *)image+0x500,"D3D9.DLL");
    dword(0x600,0x800);dword(0x700,0x12345678);
    strcpy((char *)image+0x802,"Direct3DCreate9");
}
static int find(uint32_t *slot) {return capture_find_import(read_image,NULL,sizeof(image),0x12345678,slot,&details);}
int main(void) {
    uint32_t slot=99;
    setup();memcpy(original,image,sizeof(image));assert(find(&slot)==1 && slot==0x700);
    assert(!memcmp(original,image,sizeof(image))); // Lookup must not modify the game.
    setup();dword(0x700,0x87654321);assert(find(&slot)==3 && details.reason==15 && details.slot==0x700 && details.observed==0x87654321 && details.expected==0x12345678);
    setup();dword(0x604,0x800);dword(0x704,0x12345678);assert(find(&slot)==3);
    setup();dword(0x400,0);assert(find(&slot)==3 && details.reason==13);
    setup();dword(0x600,0x8000007f);assert(find(&slot)==2);
    setup();dword(0x100,0);dword(0x104,0);assert(find(&slot)==2);
    setup();dword(0x100,0xfffffff0);assert(find(&slot)==3);
    setup();dword(0x600,0xffffffff);assert(find(&slot)==2);
    setup();denied=0x700;assert(find(&slot)==3);
    setup();word(0x98,0x20b);assert(find(&slot)==3);
    setup();dword(0x3c,0xfffffff0);assert(find(&slot)==3);
    setup();dword(0x104,20);assert(find(&slot)==3); // No terminating descriptor.
    setup();dword(0x410,0x701);assert(find(&slot)==3);
    setup();dword(0x600,65535);assert(find(&slot)==2);
    setup();dword(0x400,65532);dword(65532,0x800);assert(find(&slot)==3);
    setup();dword(0x400,0x1000);dword(0x410,0x6000);
    for (unsigned i=0;i<4096;++i) {dword(0x1000+i*4,0x80000001);dword(0x6000+i*4,1);}
    assert(find(&slot)==3 && reads<10000); // Bounded unterminated thunk table.
    puts("Capture import checks passed: unique PE32 import, untouched image, bounds, guarded/changed/ambiguous/ordinal imports and traversal limits");
}
