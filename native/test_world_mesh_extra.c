static DWORD GetCurrentProcessId(void){return 42;}
static void MemoryBarrier(void){}
/* ACTUAL_WORLD_IPC */
static WorldMeshCommand publication;
static struct {int armed;int32_t stop;} shared_state={1,0},*state=&shared_state;
typedef struct {uint32_t armed,stamp,flags,generation;} ControlFrame;
static unsigned control_armed=1,control_flags=3,control_gen=9,control_complete=1,world_reports;
static int read_control_frame(ControlFrame *out){*out=(ControlFrame){control_armed,1000,control_flags,control_gen};return control_complete;}
static uint32_t current_player(void){return player_id;}
static int32_t InterlockedCompareExchange(int32_t *v,int32_t value,int32_t expected){int32_t old=*v;if(old==expected)*v=value;return old;}
static void depth_world_report(unsigned reason,unsigned quads,int drawn){(void)reason;(void)quads;world_reports+=!!drawn;}
#include "world_mesh_draw.h"
static CubeDevice setup_world(void){
 CubeDevice f=setup();f.depth_seen=f.depth_gpu_valid=1;memcpy(f.depth_gpu,projection,64);
 world_mesh_shared=&publication;memset(&publication,0,sizeof(publication));publication.magic=0x52453457;publication.version=1;publication.build=457;publication.pid=42;publication.sequence=2;publication.stamp=1000;publication.enabled=1;publication.generation=9;publication.count=1;
 const float corners[4][3]={{-10,-10,-60},{10,-10,-60},{10,10,-60},{-10,10,-60}};
 for(unsigned i=0;i<4;i++){memcpy(publication.vertices[i].xyz,corners[i],12);publication.vertices[i].uv[0]=(i==1 || i==2);publication.vertices[i].uv[1]=i>=2;publication.vertices[i].color=0xFFFFFFFF;}
 for(unsigned i=0;i<128*128;i++)publication.pixels[i]=0xFF112233;
 state->armed=control_armed=control_complete=1;control_flags=3;control_gen=9;state->stop=test_scripted=test_native_death=0;world_reports=0;return f;
}
int main(void){
 CubeDevice f=setup_world();world_mesh_render(&f);assert(draws==1 && f.world_drawn && world_reports==1 && texture_refs==1 && !surface_refs && !backup_refs && !memcmp(&current,&original,sizeof(current)));
 for(unsigned frame=0;frame<200;frame++){f.world_drawn=0;world_mesh_render(&f);}assert(draws==201 && texture_creates==1 && texture_locks==1);
 publication.pixels[0]++;f.world_drawn=0;world_mesh_render(&f);assert(texture_locks==2 && tile[0]==0xFF112234);
 world_mesh_release(&f);assert(allocations==frees && !texture_refs);
 for(unsigned failure=1;failure<=51;failure++){
  f=setup_world();setter_fail=failure;world_mesh_render(&f);assert(!draws && applies==1 && !backup_refs && !texture_refs && state->stop==7 && !memcmp(&current,&original,sizeof(current)));world_mesh_release(&f);assert(allocations==frees);
 }
 for(unsigned gate=0;gate<16;gate++){
  f=setup_world();
  if(gate==0)publication.sequence=3;
  if(gate==1)publication.generation=10;
  if(gate==2)publication.enabled=0;
  if(gate==3)now=1500;
  if(gate==4)f8=1;
  if(gate==5)control_armed=0;
  if(gate==6)offscreen=1;
  if(gate==7)f.escaped=1;
  if(gate==8)f.scene=0;
  if(gate==9)publication.vertices[0].xyz[0]=NAN;
  if(gate==10)publication.count=129;
  if(gate==11)test_scripted=1;
  if(gate==12)no_depth=1;
  if(gate==13)f.depth_gpu[5]=4;
  if(gate==14)control_flags=0;
  if(gate==15)control_complete=0;
  world_mesh_render(&f);assert(!draws && !creates && !texture_refs && !backup_refs && !surface_refs);world_mesh_release(&f);assert(allocations==frees);
 }
 f=setup_world();apply_fail=1;world_mesh_render(&f);assert(f.gate.fatal && state->stop==7 && !texture_refs);world_mesh_release(&f);assert(allocations==frees);
 f=setup_world();texture_lock_fail=1;world_mesh_render(&f);assert(state->stop==7 && !texture_refs && !memcmp(&current,&original,sizeof(current)));world_mesh_release(&f);assert(allocations==frees);
}
