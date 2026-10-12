#include "hud_policy.h"
static HudCommand publication,*hud_command=&publication;
static int armed=1,stop;static struct {int armed;int32_t stop;} shared_state={1,0},*state=&shared_state;
typedef struct {uint32_t armed,stamp,flags,generation;} ControlFrame;
static int read_control_frame(ControlFrame *out){*out=(ControlFrame){armed,1000,3,1};return 1;}
static uint32_t current_player(void){return player_id;}
static unsigned hud_barriers;
static void MemoryBarrier(void){hud_barriers++;}
static int32_t InterlockedCompareExchange(int32_t *v,int32_t value,int32_t expected){int32_t old=*v;
        if(old==expected){*v=value;}stop=*v;return old;}
/* ACTUAL_HUD_READER */
#include "hud_draw.h"
static CubeDevice reset_hud(void){
    CubeDevice f=reset();command.style=3;publication=(HudCommand){.magic=0x52453448,.version=1,.sequence=2,.stamp=1000,.enabled=1,.generation=1};
    for(unsigned i=0;i<8192;i++)publication.pixels[i]=0xFF112233;
    state->armed=armed=1;state->stop=stop=0;return f;
}
int main(void){
    CubeDevice dormant=reset_hud();state->armed=0;unsigned old_barriers=hud_barriers;
    for(unsigned i=0;i<10000;i++)hud_render_device(&dormant);
    assert(hud_barriers==old_barriers && !creates && !texture_creates && !draws);
    CubeDevice f=reset_hud();hud_render_device(&f);
    assert(draws==2 && creates==1 && applies==1 && !backup_refs && texture_refs==1 && !surface_refs && !memcmp(&original,&current,sizeof(current)));
    for(unsigned y=0;y<32;y++){assert(tile[y*260]==0xFF112233);for(unsigned x=256;x<260;x++)assert(tile[y*260+x]==0xBABABABA);}
    assert(events[CUBE_DRAW]==0); // HUD may never mark an invisible scene's collision graphics ready.
    // Repeated publications/frames with identical pixels must not allocate or upload to the GPU.
    for(unsigned frame=0;frame<200;frame++){publication.sequence+=2;hud_render_device(&f);}
    assert(draws==402 && texture_creates==1 && texture_locks==1 && texture_unlocks==1 && texture_refs==1);
    publication.pixels[0]=0xFF445566;publication.sequence+=2;hud_render_device(&f);
    assert(texture_creates==1 && texture_locks==2 && tile[0]==0xFF445566 && !memcmp(&original,&current,sizeof(current)));
    hud_release_texture(&f);assert(!texture_refs && texture_releases==1 && !f.hud_pixels_valid);
    hud_render_device(&f);assert(texture_creates==2 && texture_locks==3 && texture_refs==1);
    publication.enabled=0;hud_render_device(&f);assert(!texture_refs);
    publication.enabled=1;hud_render_device(&f);publication.pixels[1]++;texture_lock_fail=1;hud_render_device(&f);
    assert(!texture_refs && stop && !memcmp(&original,&current,sizeof(current)));
    for(unsigned failure=1;failure<=48;failure++){
        f=reset_hud();setter_fail=failure;hud_render_device(&f);
        assert(applies==1 && !backup_refs && !texture_refs && !memcmp(&original,&current,sizeof(current)) && stop==1);
    }
    for(unsigned fail=0;fail<7;fail++){
        f=reset_hud();
        if(fail==0)texture_create_fail=1;
        if(fail==1)texture_null=1;
        if(fail==2)texture_lock_fail=1;
        if(fail==3)texture_unlock_fail=1;
        if(fail==4)texture_bad_pitch=1;
        if(fail==5)texture_null_bits=1;
        if(fail==6)draw_fail=1;
        hud_render_device(&f);assert(applies==1 && !texture_refs && !backup_refs && !memcmp(&original,&current,sizeof(current)) && stop==1);
    }
    f=reset_hud();apply_fail=1;hud_render_device(&f);assert(f.gate.fatal && stop && !texture_refs);
    for(unsigned gate=0;gate<9;gate++){
        f=reset_hud();
        if(gate==0)publication.sequence=3;
        if(gate==1)publication.generation=2;
        if(gate==2)publication.enabled=0;
        if(gate==3)now=1500;
        if(gate==4)f8=1;
        if(gate==5)armed=0;
        if(gate==6)offscreen=1;
        if(gate==7)f.escaped=1;
        if(gate==8)f.scene=0;
        hud_render_device(&f);assert(!creates && !draws && !texture_refs && !backup_refs && !surface_refs);
    }
    f=reset_hud();hud_render_device(&f);hud_reset_device(&f);assert(stop==1 && f.hud_blocked_generation==1);
    assert(!texture_refs && !f.hud_texture);
    puts("HUD: 200 unchanged frames reuse one GPU upload; changed pixels, reset/recreation, gates, 48 setter failures and restoration passed");
}
