#include "hud_policy.h"
#include "overlay_policy.h"
static HudCommand legacy,*hud_command=&legacy;
static OverlayShared image,*overlay_command=&image;static uint32_t native_hud_candidates=3,native_hud_unit_count=2;
static int armed=1,stop;static struct {int armed;int32_t stop;} shared_state={1,0},*state=&shared_state;
static uint32_t input_stamp=1000,input_generation=1;
typedef struct {uint32_t armed,stamp,flags,generation;} ControlFrame;
static int read_control_frame(ControlFrame *out){*out=(ControlFrame){armed,input_stamp,3,input_generation};return 1;}
static uint32_t current_player(void){return player_id;}
static unsigned hud_barriers,torn;
static void MemoryBarrier(void){hud_barriers++;if(torn)image.header.sequence+=2;}
static int32_t InterlockedCompareExchange(int32_t *v,int32_t value,int32_t expected){int32_t old=*v;if(old==expected)*v=value;stop=*v;return old;}
/* ACTUAL_HUD_READER */
#include "hud_draw.h"
static CubeDevice setup(void){
 CubeDevice f=reset();command.style=3;image.header=(OverlayHeader){.magic=0x5245344f,.version=1,.sequence=2,.stamp=1000,.enabled=1,.generation=1,.width=256,.height=32,.flags=1,.revision=1};
 for(unsigned i=0;i<8192;i++)image.pixels[i]=0x80445566;
 state->armed=armed=1;state->stop=stop=0;torn=0;input_stamp=1000;input_generation=1;return f;
}
int main(void){
 CubeDevice f=setup();hud_render_device(&f);
 assert(draws==1 && texture_locks==1 && !memcmp(&original,&current,sizeof(current)) && !backup_refs && !surface_refs);
 assert(image.header.viewport_width==640 && image.header.viewport_height==480 && image.header.hud_candidates==3 && image.header.hud_hidden==2);
 for(unsigned i=0;i<200;i++){image.header.sequence+=2;hud_render_device(&f);}assert(texture_locks==1 && texture_creates==1 && draws==201);
 image.header.revision++;image.pixels[0]++;hud_render_device(&f);assert(texture_locks==2);
 // A producer's large pixel copy must reuse the last valid texture, not recreate/drop it.
 unsigned prior_draws=draws,prior_creates=texture_creates;image.header.sequence|=1;
 for(unsigned i=0;i<1000;i++){now=1100;hud_render_device(&f);}
 assert(draws==prior_draws+1000 && texture_creates==prior_creates && texture_locks==2 && !stop);
 now=1000;image.header.sequence++; // Re-enable a complete producer snapshot.
 prior_draws=draws;torn=1;hud_render_device(&f);assert(draws==prior_draws+1 && texture_creates==prior_creates);torn=0;
 image.header.flags=0;image.header.selection=1;image.header.origin[2]=-600;
 float box[6]={-50,-50,-50,50,50,50};memcpy(image.header.bounds,box,sizeof(box));prior_draws=draws;hud_render_device(&f);assert(draws==prior_draws+2 && texture_locks==2); // black outline + image
 hud_release_texture(&f);assert(!texture_refs && allocations==frees);
 for(unsigned error=1;error<=45;error++){
  f=setup();setter_fail=error;hud_render_device(&f);assert(applies==1 && !memcmp(&original,&current,sizeof(current)) && stop && !texture_refs && allocations==frees);
 }
 f=setup();apply_fail=1;hud_render_device(&f);assert(f.gate.fatal && stop && !texture_refs && allocations==frees);
 for(unsigned gate=0;gate<8;gate++){
  f=setup();if(gate==0)torn=1;if(gate==1)image.header.width=1601;if(gate==2)image.header.generation=2;if(gate==3)now=1500;if(gate==4)f8=1;if(gate==5)armed=0;if(gate==6)offscreen=1;if(gate==7)f.escaped=1;
  hud_render_device(&f);assert(!draws && !texture_refs && !backup_refs && !surface_refs);hud_release_texture(&f);assert(allocations==frees);
 }
 f=setup();hud_render_device(&f);hud_reset_device(&f);assert(stop && f.hud_blocked_generation==1 && !f.overlay && !texture_refs && allocations==frees);
 for(unsigned gate=0;gate<6;gate++){
  f=setup();hud_render_device(&f);unsigned previous=draws;image.header.sequence|=1;
  if(gate==0){now=1500;input_stamp=1500;}
  if(gate==1)f8=1;
  if(gate==2)state->armed=0;
  if(gate==3)state->stop=1;
  if(gate==4)armed=0;
  if(gate==5)input_generation=2;
  hud_render_device(&f);assert(draws==previous && !f.overlay && !texture_refs && allocations==frees);
 }
 puts("Overlay: 200 cached frames, native-view black selection, telemetry, alpha/linear state, 45 setter failures, torn/TTL/F8/identity and reset cleanup passed");
}
