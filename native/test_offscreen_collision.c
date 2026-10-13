/* Actual renderer + actual collision callback: visibility must not gate physics. */
#define main renderer_fixture_main
#include "test_cube_draw.c"
#undef main
typedef struct {unsigned held;} SRWLOCK;
#define SRWLOCK_INIT {0}
static int TryAcquireSRWLockExclusive(SRWLOCK *lock){if(lock->held)return 0;lock->held=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){assert(lock->held);lock->held=0;}
static void MemoryBarrier(void){}
static void InterlockedIncrement(LONG *p){++*p;}
static LONG InterlockedCompareExchange(LONG *p,LONG value,LONG expected){LONG old=*p;if(old==expected)*p=value;return old;}
typedef struct {uint32_t armed;LONG stop;} Shared;
static Shared control_state={1,0},*state=&control_state;
typedef struct {uint32_t generation,armed,stamp;} ControlFrame;
typedef struct {LONG sequence;uint32_t ticks,contacts,corrections,rejections,reason,active,generation;} CollisionTelemetry;
static CollisionTelemetry report,*collision_shared=&report;
static unsigned char player_data[0xB0];static uint32_t player_value,*player_slot=&player_value;
static uint32_t current_player(void){return player_value;}
static int writable(const void *p,size_t n){assert(p==player_data+0x94 && n==12);return 1;}
static int cube_collision_command_read(CubeCommand *out){memcpy(out,&command,3168);return 1;}
static int cube_collision_graphics_ready(void){return scene_active;}
#include "collision_tick.h"
static void pose(float x,float y,float z){float p[3]={x,y,z};memcpy(player_data+0x94,p,12);}
static float coordinate(unsigned axis){float p;memcpy(&p,player_data+0x94+axis*4,4);return p;}
int main(void){
    CubeDevice f=reset();scene_active=0;
    command.mode=2;command.style=1;command.count=1;command.origin[2]=-600;command.collisions=1;command.arm_generation=1;
    float wall[6]={0,0,-500,1000,1000,500};memcpy(command.boxes[0],wall,sizeof(wall));
    player_value=(uint32_t)(uintptr_t)player_data;assert((uintptr_t)player_data<=UINT32_MAX);pose(-300,0,-600);
    ControlFrame c={.armed=1,.generation=1,.stamp=now};
    cube_render_device(&f);assert(scene_active && draws==1);cube_collision_tick(&c,1,now);
    shift=100000; // All block vertices outside viewport, but scene/camera/lease remain valid.
    for(unsigned frame=0;frame<200;frame++){
        now+=16;c.stamp=command.stamp=now;
        if(frame%7==0)cube_record(CUBE_REJECT,CUBE_OFF); // transient/torn pass must be recoverable even offscreen
        cube_render_device(&f);assert(scene_active && draws==1 && events[CUBE_PASS_SKIP]);
        pose(coordinate(0)+66.4f,8,-600);cube_collision_tick(&c,1,now);
        assert(report.active && !state->stop && coordinate(0)<-250 && coordinate(1)==8);
    }
    assert(report.corrections==200 && !report.rejections);
    shift=0;cube_render_device(&f);cube_collision_tick(&c,1,now);
    assert(draws==2 && !state->stop && report.active);
    camera_invalid=1;cube_render_device(&f);assert(!scene_active);cube_collision_tick(&c,1,now);assert(!report.active);
    block_texture_cache_release(&f.block_texture);assert(allocations==frees && !backup_refs && !surface_refs);
    puts("Offscreen renderer/physics integration: 200 frames, transient pass recovery, continuous sprint collision, return to view and invalid camera rejection passed");
}
