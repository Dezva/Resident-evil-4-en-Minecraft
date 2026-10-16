static unsigned compare=8,queries;
static int world_mesh_requested(void){return 0;}
static void world_mesh_release(CubeDevice *f){(void)f;}
static void world_mesh_render(CubeDevice *f){(void)f;}
static int cube_projection(float *out){float gx[16]={.75,0,0,0,0,1,0,0,0,0,-10.f/9990,-100000.f/9990,0,0,-1,0};memcpy(out,gx,64);return 1;}
static HRESULT render_state_read(IDirect3DDevice9 *d,D3DRENDERSTATETYPE key,DWORD *out){(void)d;queries++;*out=key==D3DRS_ZFUNC?compare:1;return S_OK;}
static HRESULT shader_constants(IDirect3DDevice9 *d,UINT start,float *out,UINT count){(void)d;assert(!start && count==256);memset(out,0,count*16);return S_OK;}
static int depth_wvp_gpu(IDirect3DDevice9 *d,const float *gx,float fov,const D3DVIEWPORT9 *vp,unsigned compare,BlockProjection *out){(void)d;(void)gx;(void)fov;(void)vp;(void)compare;(void)out;return 0;}
#include "depth_device.h"
int main(void){
 CubeDevice f=setup();f.depth_seen=0;scene_active=0;command.collisions=1;command.arm_generation=9;
 table.GetRenderState=render_state_read;table.GetVertexShaderConstantF=shader_constants;
 player_value=(uint32_t)(uintptr_t)player_data;assert((uintptr_t)player_data<=UINT32_MAX);pose(-300,0,-60);
 ControlFrame c={.armed=1,.generation=9,.stamp=now};state->armed=1;state->stop=0;
 compare=8;cube_depth_after_draw(&f);assert(!f.depth_seen && !scene_active);
 compare=4;cube_depth_after_draw(&f);assert(scene_active && f.depth_seen);cube_collision_tick(&c,1,now);
 for(unsigned frame=0;frame<200;frame++){
  now+=16;c.stamp=command.stamp=now;f.depth_seen=f.depth_drawn=f.world_drawn=f.depth_gpu_valid=f.depth_gpu_checked=0;
  shift=frame<100?100000:0;
  compare=8;for(unsigned i=0;i<25;i++)cube_depth_after_draw(&f);
  assert(!f.depth_seen && scene_active);
  compare=4;cube_depth_after_draw(&f);assert(scene_active && f.depth_drawn);
  unsigned prior=queries;for(unsigned i=0;i<100;i++)cube_depth_after_draw(&f);assert(queries==prior);
  pose(coordinate(0)+66.4f,8,-60);cube_collision_tick(&c,1,now);
  assert(report.active && !state->stop && coordinate(0)<-250 && coordinate(1)==8);
 }
 assert(report.corrections==200 && !report.rejections);
 // Actual loss of a valid frame still retains the safe reference and stops at500ms.
 camera_invalid=1;cube_render_device(&f);assert(!scene_active);cube_collision_tick(&c,1,now);
 now+=600;c.stamp=command.stamp=now;camera_invalid=0;f.depth_seen=f.depth_drawn=f.world_drawn=f.depth_gpu_valid=f.depth_gpu_checked=0;
 cube_depth_after_draw(&f);assert(scene_active);cube_collision_tick(&c,1,now);assert(state->stop==3 && !report.active);
 block_texture_cache_release(&f.block_texture);assert(allocations==frees && !backup_refs && !surface_refs);
}
