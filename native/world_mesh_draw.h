#include "world_mesh_geometry.h"
typedef struct {WorldMeshCommand command;BlockSurfaceVertex vertices[WORLD_DRAW_VERTICES];IDirect3DTexture9 *texture;unsigned valid;uint32_t pixels[128*128];} WorldVisuals;
static void world_mesh_release(CubeDevice *f){
 WorldVisuals *p=f->world_visuals;f->world_visuals=NULL;if(p){if(p->texture)p->texture->lpVtbl->Release(p->texture);HeapFree(GetProcessHeap(),0,p);}
}
static void __attribute__((noinline)) world_mesh_render(CubeDevice *f){
 ControlFrame control={0};uint32_t now=GetTickCount();
 if(!world_mesh_requested() || runtime_scripted() || runtime_death() || !state || !state->armed || state->stop || !read_control_frame(&control) || !control.armed || (control.flags&3)!=3 || now-control.stamp>=500 || !current_player() || (GetAsyncKeyState(VK_F8)&0x8000) || f->escaped || f->resetting || f->gate.fatal || f->gate.lost){world_mesh_release(f);return;}
 if(!f->scene || f->world_drawn || !f->depth_seen)return;
 if(!f->world_visuals)f->world_visuals=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WorldVisuals));
 WorldVisuals *p=f->world_visuals;if(!p || !world_mesh_read(&p->command)){depth_world_report(3,0,0);return;}
 if(p->command.generation!=control.generation){depth_world_report(8,0,0);return;}
 IDirect3DDevice9 *d=f->inner;IDirect3DSurface9 *target=NULL,*back=NULL,*z=NULL;
 HRESULT hr=d->lpVtbl->GetRenderTarget(d,0,&target);if(FAILED(hr)||!target)return;
 hr=d->lpVtbl->GetBackBuffer(d,0,0,D3DBACKBUFFER_TYPE_MONO,&back);int matches=SUCCEEDED(hr)&&back&&target==back;
 if(back)back->lpVtbl->Release(back);
 target->lpVtbl->Release(target);if(!matches)return;
 hr=d->lpVtbl->GetDepthStencilSurface(d,&z);if(z)z->lpVtbl->Release(z);if(FAILED(hr)||!z)return;
 float view[12],fov;uint32_t player;D3DVIEWPORT9 vp;BlockProjection projection;
 if(!cube_camera(view,&fov,&player) || FAILED(d->lpVtbl->GetViewport(d,&vp)) || !isfinite(vp.MinZ+vp.MaxZ) || vp.MinZ<0 || vp.MaxZ>1 || vp.MinZ>=vp.MaxZ)return;
 int valid=f->depth_gpu_valid?block_projection(f->depth_gpu,fov,(float)vp.Width,(float)vp.Height,&projection):block_gx_projection(f->depth_gx,fov,(float)vp.Width,(float)vp.Height,f->depth_world_compare,&projection);
 if(!valid){depth_world_report(2,0,0);return;}
 unsigned count=world_mesh_project(p->command.vertices,p->command.count,view,&projection,&vp,p->vertices);if(!count){f->world_drawn=1;return;}
 IDirect3DStateBlock9 *saved=NULL;hr=d->lpVtbl->CreateStateBlock(d,D3DSBT_ALL,&saved);if(FAILED(hr)||!saved)return;
 hr=saved->lpVtbl->Capture(saved);
 if(SUCCEEDED(hr)){
  if(!p->texture)hr=d->lpVtbl->CreateTexture(d,128,128,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&p->texture,NULL);
  if(SUCCEEDED(hr)&&!p->texture)hr=E_FAIL;
  if(SUCCEEDED(hr)&&(!p->valid || memcmp(p->pixels,p->command.pixels,sizeof(p->pixels)))){
   D3DLOCKED_RECT lock={0};hr=p->texture->lpVtbl->LockRect(p->texture,0,&lock,NULL,D3DLOCK_DISCARD);
   if(SUCCEEDED(hr)){
    if(!lock.pBits || lock.Pitch<512 || lock.Pitch>65536)hr=E_FAIL;
    else for(unsigned y=0;y<128;y++)memcpy((uint8_t*)lock.pBits+y*(unsigned)lock.Pitch,p->command.pixels+y*128,512);
    HRESULT unlocked=p->texture->lpVtbl->UnlockRect(p->texture,0);if(SUCCEEDED(hr))hr=unlocked;
    if(SUCCEEDED(hr)){memcpy(p->pixels,p->command.pixels,sizeof(p->pixels));p->valid=1;}
   }
  }
  if(SUCCEEDED(hr))hr=overlay_prepare_state(d,(IDirect3DBaseTexture9*)p->texture,1,1);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ZENABLE,D3DZB_TRUE);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ZWRITEENABLE,TRUE);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ZFUNC,projection.reverse?D3DCMP_GREATEREQUAL:D3DCMP_LESSEQUAL);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_DEPTHBIAS,0);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_SLOPESCALEDEPTHBIAS,0);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ALPHATESTENABLE,TRUE);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ALPHAFUNC,D3DCMP_GREATER);
  if(SUCCEEDED(hr))hr=d->lpVtbl->SetRenderState(d,D3DRS_ALPHAREF,0);
  if(SUCCEEDED(hr))hr=d->lpVtbl->DrawPrimitiveUP(d,D3DPT_TRIANGLELIST,count/3,p->vertices,sizeof(BlockSurfaceVertex));
  HRESULT restored=saved->lpVtbl->Apply(saved);
  if(FAILED(restored)){f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);}
  else if(SUCCEEDED(hr)){f->world_drawn=1;depth_world_report(0,p->command.count,1);}
 }
 saved->lpVtbl->Release(saved);
 if(FAILED(hr) || f->gate.fatal){world_mesh_release(f);f->gate.blocked=control.generation;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)hr);if(state)InterlockedCompareExchange(&state->stop,7,0);}
}
