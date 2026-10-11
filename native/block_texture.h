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
static HRESULT block_texture_create(IDirect3DDevice9 *device,IDirect3DTexture9 **out) {
    *out=NULL;IDirect3DTexture9 *texture=NULL;
    HRESULT hr=device->lpVtbl->CreateTexture(device,16,16,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,NULL);
    if (FAILED(hr) || !texture) {if(texture)texture->lpVtbl->Release(texture);return FAILED(hr)?hr:E_FAIL;}
    D3DLOCKED_RECT locked={0};
    hr=texture->lpVtbl->LockRect(texture,0,&locked,NULL,D3DLOCK_DISCARD);
    if (SUCCEEDED(hr)) {
        if (!locked.pBits || locked.Pitch<64 || locked.Pitch>65536) hr=E_FAIL;
        else for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) {
            uint32_t pixel=block_stone_pixel(x,y);
            memcpy((unsigned char*)locked.pBits+(size_t)y*(unsigned)locked.Pitch+x*4,&pixel,4);
        }
        HRESULT unlocked=texture->lpVtbl->UnlockRect(texture,0);
        if (SUCCEEDED(hr)) hr=unlocked;
    }
    if (FAILED(hr)) {texture->lpVtbl->Release(texture);return hr;}
    *out=texture;return S_OK;
}
#endif
