/* Independent, bounded 2D layer. No game memory/coordinates in its draw path. */
#include "hud_policy.h"
#include "overlay_state.h"
static void hud_reset_device(CubeDevice *f){
    f->hud_blocked_generation=f->hud_generation;
    if(f->hud_generation && state)InterlockedCompareExchange(&state->stop,1,0);
}
static void hud_quad(BlockSurfaceVertex *out,float x,float y,float w,float h){
    const unsigned order[6]={0,1,2,0,2,3};
    for(unsigned i=0;i<6;i++){unsigned n=order[i];float u=n==1 || n==2?1.f:0.f,v=n>=2?1.f:0.f;out[i]=(BlockSurfaceVertex){x+u*w,y+v*h,0,1,0xFFFFFFFF,u,v};}
}
static void hud_render_device(CubeDevice *f){
    HudCommand command={0};ControlFrame control={0};uint32_t now=GetTickCount();
    if(!state || !hud_command_read(&command) || !command.enabled || !read_control_frame(&control)
        || !state->armed || state->stop || !control.armed || (control.flags&3u)!=3u
        || command.generation!=control.generation || (uint32_t)(now-command.stamp)>=500 || (uint32_t)(now-control.stamp)>=500
        || !current_player() || GetAsyncKeyState(VK_F8)&0x8000)return;
    if(f->escaped || f->resetting || f->gate.fatal || f->gate.lost || f->hud_blocked_generation==command.generation || !f->scene)return;
    f->hud_generation=command.generation;
    IDirect3DDevice9 *device=f->inner;IDirect3DSurface9 *target=NULL,*back=NULL;
    HRESULT hr=device->lpVtbl->GetRenderTarget(device,0,&target);
    if(FAILED(hr) || !target)return;
    hr=device->lpVtbl->GetBackBuffer(device,0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    int matches=SUCCEEDED(hr) && back && target==back;
    if(SUCCEEDED(hr) && back)back->lpVtbl->Release(back);
    target->lpVtbl->Release(target);if(!matches)return;
    D3DVIEWPORT9 vp;hr=device->lpVtbl->GetViewport(device,&vp);
    if(FAILED(hr) || vp.Width<32 || vp.Height<32 || vp.Width>16384 || vp.Height>16384)return;
    IDirect3DStateBlock9 *backup=NULL;IDirect3DTexture9 *texture=NULL;
    hr=device->lpVtbl->CreateStateBlock(device,D3DSBT_ALL,&backup);
    if(FAILED(hr) || !backup)return;
    hr=backup->lpVtbl->Capture(backup);
    if(SUCCEEDED(hr)){
        hr=device->lpVtbl->CreateTexture(device,256,32,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,NULL);
        if(SUCCEEDED(hr) && !texture)hr=E_FAIL;
        if(SUCCEEDED(hr)){
            D3DLOCKED_RECT locked={0};hr=texture->lpVtbl->LockRect(texture,0,&locked,NULL,D3DLOCK_DISCARD);
            if(SUCCEEDED(hr)){
                if(!locked.pBits || locked.Pitch<1024 || locked.Pitch>65536)hr=E_FAIL;
                else for(unsigned y=0;y<32;y++)memcpy((unsigned char*)locked.pBits+y*(unsigned)locked.Pitch,command.pixels+y*256,1024);
                HRESULT unlocked=texture->lpVtbl->UnlockRect(texture,0);if(SUCCEEDED(hr))hr=unlocked;
            }
        }
        if(SUCCEEDED(hr))hr=overlay_prepare_state(device,(IDirect3DBaseTexture9*)texture,1,1);
        float scale=fmaxf(1.f,fminf(4.f,floorf((float)vp.Height/240.f)));
        scale=fminf(scale,(float)vp.Width/256.f);
        BlockSurfaceVertex bar[6];hud_quad(bar,vp.X+(vp.Width-256.f*scale)/2,vp.Y+vp.Height-32.f*scale,256.f*scale,32.f*scale);
        if(SUCCEEDED(hr))hr=device->lpVtbl->DrawPrimitiveUP(device,D3DPT_TRIANGLELIST,2,bar,sizeof(*bar));
        // White crosshair, with texture disabled only inside this captured state.
        if(SUCCEEDED(hr))hr=device->lpVtbl->SetTexture(device,0,NULL);
        if(SUCCEEDED(hr))hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        if(SUCCEEDED(hr))hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        if(SUCCEEDED(hr))hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
        if(SUCCEEDED(hr))hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
        BlockSurfaceVertex cross[12];float cx=vp.X+vp.Width/2.f,cy=vp.Y+vp.Height/2.f;
        hud_quad(cross,cx-5*scale,cy-.5f*scale,10*scale,scale);hud_quad(cross+6,cx-.5f*scale,cy-5*scale,scale,10*scale);
        if(SUCCEEDED(hr))hr=device->lpVtbl->DrawPrimitiveUP(device,D3DPT_TRIANGLELIST,4,cross,sizeof(*cross));
        HRESULT restored=backup->lpVtbl->Apply(backup);
        if(FAILED(restored)){f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);hud_reset_device(f);}
    }
    if(texture)texture->lpVtbl->Release(texture);
    backup->lpVtbl->Release(backup);
    if(FAILED(hr)){cube_record(CUBE_FAILURE,(uint32_t)hr);hud_reset_device(f);}
}
