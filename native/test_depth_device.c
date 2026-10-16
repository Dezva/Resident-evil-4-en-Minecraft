#include "test_cube_platform.h"
#include "block_depth.h"
#define D3DZB_TRUE 1
#define D3DRS_ZFUNC 23
typedef struct {IDirect3DDevice9 *inner;unsigned scene,depth_drawn,world_drawn,depth_seen,depth_world_compare,depth_gpu_valid,depth_gpu_checked,depth_stage;float depth_gpu[16],depth_gx[16];} CubeDevice;
static unsigned requested=1,world_requested=1,box_requested=1,refs,cubes,worlds,reads,offscreen,z_on=1,compare=4,camera_ready=1,gx_ready=1,gpu_ready=1,gpu_reads,viewport_ready=1,reject_draw;
static int depth_requested(uint32_t *unused){(void)unused;return requested;}
static int world_mesh_requested(void){return world_requested;}
static int cube_command_requested(void){return box_requested;}
static void world_mesh_release(CubeDevice *f){(void)f;}
static int cube_projection(float *out){float gx[16]={.75,0,0,0,0,1,0,0,0,0,-10.f/9990,-100000.f/9990,0,0,-1,0};if(!gx_ready)gx[5]=2;memcpy(out,gx,64);return 1;}
static int cube_camera(float *view,float *fov,uint32_t *player){memset(view,0,48);*fov=90;*player=42;return camera_ready;}
static unsigned wvp_ready;
static int depth_wvp_gpu(IDirect3DDevice9 *d,const float *gx,float fov,const D3DVIEWPORT9 *vp,unsigned compare_value,BlockProjection *out){
 (void)d;(void)gx;(void)compare_value;if(!wvp_ready)return 0;
 float p[16]={.75,0,0,0,0,1,0,0,0,0,-10000.f/9997,-1,0,0,-30000.f/9997,0};
 return block_projection(p,fov,(float)vp->Width,(float)vp->Height,out);
}
static void cube_render_device(CubeDevice *f){
 BlockProjection p;int valid=f->depth_gpu_valid?block_projection(f->depth_gpu,90,640,480,&p):block_gx_projection(f->depth_gx,90,640,480,f->depth_world_compare,&p);
 if(f->depth_stage!=2 || !valid){fputs("Rejected pass reached world renderer before projection validation\n",stderr);exit(43);}
 assert(fabsf(p.near_plane-(f->depth_gpu_valid?3:10))<.001);if(!reject_draw)f->depth_drawn=1;cubes++;
}
static void world_mesh_render(CubeDevice *f){assert(f->depth_stage==2);if(!reject_draw)f->world_drawn=1;worlds++;}
static HRESULT render_state(IDirect3DDevice9 *d,D3DRENDERSTATETYPE key,DWORD *out){(void)d;reads++;*out=key==D3DRS_ZFUNC?compare:key==D3DRS_ZENABLE?z_on:1;return S_OK;}
static ULONG surface_release(IDirect3DSurface9 *s){(void)s;assert(refs);return --refs;}
static TestSurfaceVtbl surfaces={surface_release};static IDirect3DSurface9 target={&surfaces},other={&surfaces};
static HRESULT get_target(IDirect3DDevice9 *d,DWORD n,IDirect3DSurface9 **out){(void)d;(void)n;*out=&target;refs++;return S_OK;}
static HRESULT get_back(IDirect3DDevice9 *d,UINT n,UINT b,D3DBACKBUFFER_TYPE t,IDirect3DSurface9 **out){(void)d;(void)n;(void)b;(void)t;*out=offscreen?&other:&target;refs++;return S_OK;}
static HRESULT viewport(IDirect3DDevice9 *d,D3DVIEWPORT9 *out){(void)d;*out=(D3DVIEWPORT9){0,0,640,480,0,viewport_ready?1:0};return S_OK;}
static HRESULT constants(IDirect3DDevice9 *d,UINT start,float *out,UINT count){(void)d;assert(!start && count==256);gpu_reads++;memset(out,0,256*16);float gpu[16]={.75,0,0,0,0,1,0,0,0,0,-10000.f/9997,-1,0,0,-30000.f/9997,0};if(gpu_ready)memcpy(out+16,gpu,64);return S_OK;}
static IDirect3DDevice9Vtbl methods={.GetRenderState=render_state,.GetRenderTarget=get_target,.GetBackBuffer=get_back,.GetViewport=viewport,.GetVertexShaderConstantF=constants};static IDirect3DDevice9 inner={&methods};
#include "depth_device.h"
int main(void){
 CubeDevice f={.inner=&inner,.scene=1};cube_depth_after_draw(&f);assert(cubes==1 && worlds==1 && !f.depth_stage && !refs && f.depth_gpu_valid);
 // Sky/clear passes can write Z with ALWAYS: they must not lock the world projection.
 unsigned old_cubes=cubes,old_worlds=worlds;
 for(unsigned c=1;c<=8;c++)if(c!=2 && c!=4 && c!=5 && c!=7){
  f=(CubeDevice){.inner=&inner,.scene=1};compare=c;
  cube_depth_after_draw(&f);assert(!f.depth_seen && cubes==old_cubes && worlds==old_worlds);
 }
 compare=4;f=(CubeDevice){.inner=&inner,.scene=1};cube_depth_after_draw(&f);assert(cubes==2 && worlds==2);
 unsigned prior=reads;cube_depth_after_draw(&f);assert(reads==prior);
 f=(CubeDevice){.inner=&inner,.scene=1};z_on=0;cube_depth_after_draw(&f);assert(!f.depth_seen && cubes==2);z_on=1;offscreen=1;cube_depth_after_draw(&f);assert(!f.depth_seen && !refs && cubes==2);
 offscreen=0;cube_depth_after_draw(&f);assert(cubes==3 && worlds==3 && !refs);
 f=(CubeDevice){.inner=&inner,.scene=1};world_requested=box_requested=0;prior=reads;cube_depth_after_draw(&f);assert(reads==prior);
 requested=0;world_requested=box_requested=1;cube_depth_after_draw(&f);assert(reads==prior);
 requested=1;unsigned old=cubes;
 f=(CubeDevice){.inner=&inner,.scene=1};camera_ready=0;cube_depth_after_draw(&f);assert(!f.depth_seen && !f.depth_gpu_checked && cubes==old);
 camera_ready=1;viewport_ready=0;cube_depth_after_draw(&f);assert(!f.depth_seen && !f.depth_gpu_checked && cubes==old);
 viewport_ready=1;cube_depth_after_draw(&f);assert(cubes==old+1 && f.depth_seen);
 f=(CubeDevice){.inner=&inner,.scene=1};gx_ready=gpu_ready=0;old=cubes;unsigned scans=gpu_reads;
 for(unsigned i=0;i<100;i++)cube_depth_after_draw(&f);
 assert(!f.depth_seen && cubes==old && gpu_reads==scans+1);
 gx_ready=1;cube_depth_after_draw(&f);assert(f.depth_seen && cubes==old+1 && !f.depth_gpu_valid && gpu_reads==scans+1);
 f=(CubeDevice){.inner=&inner,.scene=1};gpu_ready=1;reject_draw=1;old=cubes;cube_depth_after_draw(&f);prior=reads;scans=gpu_reads;
 for(unsigned i=0;i<10000;i++)cube_depth_after_draw(&f);
 assert(cubes==old+1 && reads==prior && gpu_reads==scans && f.depth_drawn && f.world_drawn);
 f=(CubeDevice){.inner=&inner,.scene=1};wvp_ready=1;old=cubes;scans=gpu_reads;cube_depth_after_draw(&f);
 assert(cubes==old+1 && f.depth_gpu_valid==4 && gpu_reads==scans); // preserve source4 through validation

}
