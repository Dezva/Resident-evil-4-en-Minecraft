/* Full GUI/hand layer: bounded shared pixels, heap storage, no giant stack copy. */
#include "overlay_policy.h"
typedef struct {IDirect3DTexture9 *texture;uint32_t revision,width,height;uint32_t pixels[OVERLAY_MAX_PIXELS];} OverlayDevice;
static void overlay_release(CubeDevice *f){
 OverlayDevice *p=f->overlay;f->overlay=NULL;
 if(p){if(p->texture)p->texture->lpVtbl->Release(p->texture);HeapFree(GetProcessHeap(),0,p);}
}
static void __attribute__((noinline)) overlay_render_device(CubeDevice *f){
 ControlFrame control={0};OverlayHeader h;uint32_t now=GetTickCount();
 if(!state || !state->armed || state->stop || !read_control_frame(&control) || !control.armed || (control.flags&3)!=3
   || !overlay_header_read(&h) || !h.enabled || h.generation!=control.generation || now-h.stamp>=500 || now-control.stamp>=500
   || !current_player() || (GetAsyncKeyState(VK_F8)&0x8000) || f->escaped || f->resetting || f->gate.fatal || f->gate.lost || f->hud_blocked_generation==h.generation){overlay_release(f);return;}
 if(!f->scene)return;
 f->hud_generation=h.generation;
 IDirect3DDevice9 *d=f->inner;IDirect3DSurface9 *target=NULL,*back=NULL;
 HRESULT hr=d->lpVtbl->GetRenderTarget(d,0,&target);if(FAILED(hr)||!target)return;
 hr=d->lpVtbl->GetBackBuffer(d,0,0,D3DBACKBUFFER_TYPE_MONO,&back);int matches=SUCCEEDED(hr)&&back&&back==target;
 if(back)back->lpVtbl->Release(back);
 target->lpVtbl->Release(target);if(!matches)return;
 D3DVIEWPORT9 vp;if(FAILED(d->lpVtbl->GetViewport(d,&vp)) || vp.Width<32 || vp.Height<32 || vp.Width>16384 || vp.Height>16384)return;
 overlay_command->header.viewport_width=vp.Width;overlay_command->header.viewport_height=vp.Height;
 overlay_command->header.hud_candidates=native_hud_candidates;overlay_command->header.hud_hidden=native_hud_unit_count;
 if(!f->overlay)f->overlay=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OverlayDevice));
 OverlayDevice *p=f->overlay;if(!p)return;
 int changed=!p->texture || p->revision!=h.revision || p->width!=h.width || p->height!=h.height;
 if(changed){
  MemoryBarrier();if(((volatile OverlayHeader*)&overlay_command->header)->sequence!=h.sequence)return;
  memcpy(p->pixels,overlay_command->pixels,(size_t)h.width*h.height*4);MemoryBarrier();
  if(((volatile OverlayHeader*)&overlay_command->header)->sequence!=h.sequence)return;
 }
 IDirect3DStateBlock9 *saved=NULL;hr=d->lpVtbl->CreateStateBlock(d,D3DSBT_ALL,&saved);if(FAILED(hr)||!saved)return;
 hr=saved->lpVtbl->Capture(saved);
 if(SUCCEEDED(hr)){
  if(p->texture && (p->width!=h.width || p->height!=h.height)){p->texture->lpVtbl->Release(p->texture);p->texture=NULL;}
  if(!p->texture)hr=d->lpVtbl->CreateTexture(d,h.width,h.height,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&p->texture,NULL);
  if(SUCCEEDED(hr)&&!p->texture)hr=E_FAIL;
  if(SUCCEEDED(hr)&&changed){
   D3DLOCKED_RECT lock={0};hr=p->texture->lpVtbl->LockRect(p->texture,0,&lock,NULL,D3DLOCK_DISCARD);
   if(SUCCEEDED(hr)){
    if(!lock.pBits || lock.Pitch<(int)(h.width*4) || lock.Pitch>65536)hr=E_FAIL;
    else for(unsigned y=0;y<h.height;y++)memcpy((uint8_t*)lock.pBits+y*(unsigned)lock.Pitch,p->pixels+y*h.width,h.width*4);
    HRESULT unlock=p->texture->lpVtbl->UnlockRect(p->texture,0);if(SUCCEEDED(hr))hr=unlock;
    if(SUCCEEDED(hr)){p->revision=h.revision;p->width=h.width;p->height=h.height;}
   }
  }
  // The outline uses the same latched native view as world blocks, never the MC render camera.
  if(SUCCEEDED(hr)&&h.selection){
   float view[12],local[12],fov;uint32_t actor;BlockVertex lines[24];unsigned n=0;
   if(cube_camera(view,&fov,&actor)&&block_scene_view(view,h.origin,local))n=block_lines(h.bounds,local,fov,(float)vp.Width,(float)vp.Height,lines);
   if(n){
    BlockVertex thick[72];unsigned count=0;
    for(unsigned i=0;i<n;i+=2){
     float dx=lines[i+1].x-lines[i].x,dy=lines[i+1].y-lines[i].y,len=sqrtf(dx*dx+dy*dy);
     if(len<.001f)continue;
     float nx=-dy/len,ny=dx/len;BlockVertex corners[4]={lines[i],lines[i+1],lines[i+1],lines[i]};
     for(unsigned j=0;j<4;j++){float sign=j<2?1.f:-1.f;corners[j].x+=vp.X+sign*nx;corners[j].y+=vp.Y+sign*ny;corners[j].color=0xFF000000;}
     const unsigned order[6]={0,1,2,0,2,3};for(unsigned j=0;j<6;j++)thick[count++]=corners[order[j]];
    }
    hr=overlay_prepare_state(d,NULL,0,0);if(SUCCEEDED(hr)&&count)hr=d->lpVtbl->DrawPrimitiveUP(d,D3DPT_TRIANGLELIST,count/3,thick,sizeof(*thick));
   }
  }
  if(SUCCEEDED(hr))hr=overlay_prepare_state(d,(IDirect3DBaseTexture9*)p->texture,1,1);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetSamplerState(d,0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetSamplerState(d,0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
  BlockSurfaceVertex quad[6];hud_quad(quad,(float)vp.X-.5f,(float)vp.Y-.5f,(float)vp.Width,(float)vp.Height);
  if(SUCCEEDED(hr))hr=d->lpVtbl->DrawPrimitiveUP(d,D3DPT_TRIANGLELIST,2,quad,sizeof(*quad));
  HRESULT restored=saved->lpVtbl->Apply(saved);if(FAILED(restored)){f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);hud_reset_device(f);}
 }
 saved->lpVtbl->Release(saved);if(FAILED(hr)){cube_record(CUBE_FAILURE,(uint32_t)hr);hud_reset_device(f);}
}
