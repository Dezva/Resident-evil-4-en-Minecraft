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
static inline HRESULT block_texture_upload(IDirect3DTexture9 *texture,const CubeCommand *command) {
    unsigned size=command?256:16;
    D3DLOCKED_RECT locked={0};
    HRESULT hr=texture->lpVtbl->LockRect(texture,0,&locked,NULL,D3DLOCK_DISCARD);
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
    return hr;
}
typedef struct {IDirect3DTexture9 *texture;unsigned style,tile_count;uint32_t pixels[BLOCK_MAX_TILES][256];} BlockTextureCache;
static inline void block_texture_cache_release(BlockTextureCache *cache){
    IDirect3DTexture9 *texture=cache->texture;cache->texture=NULL;cache->style=cache->tile_count=0;
    if(texture)texture->lpVtbl->Release(texture);
}
static inline HRESULT block_texture_cached(IDirect3DDevice9 *device,BlockTextureCache *cache,const CubeCommand *command){
    unsigned style=command?3:2,tiles=command?command->tile_count:0,size=command?256:16;
    if(cache->texture && cache->style!=style)block_texture_cache_release(cache);
    int changed=!cache->texture || (command && (tiles!=cache->tile_count || memcmp(cache->pixels,command->pixels,tiles*1024)));
    if(!changed)return S_OK;
    HRESULT hr=S_OK;
    if(!cache->texture){
        hr=device->lpVtbl->CreateTexture(device,size,size,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&cache->texture,NULL);
        if(SUCCEEDED(hr) && !cache->texture)hr=E_FAIL;
    }
    if(SUCCEEDED(hr))hr=block_texture_upload(cache->texture,command);
    if(FAILED(hr)){block_texture_cache_release(cache);return hr;}
    cache->style=style;cache->tile_count=tiles;
    if(command)memcpy(cache->pixels,command->pixels,tiles*1024);
    return S_OK;
}
#endif
