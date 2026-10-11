#ifndef RE4_BLOCK_TEXTURE_H
#define RE4_BLOCK_TEXTURE_H
/* Original procedural tile, not an extracted Minecraft/game asset. */
static inline uint32_t block_stone_pixel(unsigned x,unsigned y) {
    uint32_t seed=(x/2)*0x9E3779B9u+(y/2)*0x85EBCA6Bu+0x31415926u;
    seed^=seed>>16;seed*=0x7FEB352Du;seed^=seed>>15;
    unsigned value=140+(seed%61);
    if ((x+y)%7==0) value-=18;
    return 0xFF000000u|value*0x010101u;
}
static HRESULT block_texture_create_with_tiles(IDirect3DDevice9 *device,IDirect3DTexture9 **out,const CubeCommand *command) {
    unsigned size=command?256:16;
    *out=NULL;IDirect3DTexture9 *texture=NULL;
    HRESULT hr=device->lpVtbl->CreateTexture(device,size,size,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,NULL);
    if (FAILED(hr) || !texture) {if(texture)texture->lpVtbl->Release(texture);return FAILED(hr)?hr:E_FAIL;}
    D3DLOCKED_RECT locked={0};
    hr=texture->lpVtbl->LockRect(texture,0,&locked,NULL,D3DLOCK_DISCARD);
    if (SUCCEEDED(hr)) {
        if (!locked.pBits || locked.Pitch<(int)(size*4) || locked.Pitch>65536) hr=E_FAIL;
        else for(unsigned y=0;y<size;++y) for(unsigned x=0;x<size;++x) {
            unsigned tile=(y/32)*8+x/32,tx=x%32,ty=y%32;
            tx=tx<8?0:tx>23?15:tx-8;ty=ty<8?0:ty>23?15:ty-8;
            uint32_t pixel=command?command->pixels[tile<command->tile_count?tile:0][ty*16+tx]:block_stone_pixel(x,y);
            memcpy((unsigned char*)locked.pBits+(size_t)y*(unsigned)locked.Pitch+x*4,&pixel,4);
        }
        HRESULT unlocked=texture->lpVtbl->UnlockRect(texture,0);
        if (SUCCEEDED(hr)) hr=unlocked;
    }
    if (FAILED(hr)) {texture->lpVtbl->Release(texture);return hr;}
    *out=texture;return S_OK;
}
static HRESULT block_texture_create(IDirect3DDevice9 *device,IDirect3DTexture9 **out){return block_texture_create_with_tiles(device,out,NULL);}
#endif
