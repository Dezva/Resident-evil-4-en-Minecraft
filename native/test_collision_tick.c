static inline int runtime_scripted(void){return 0;}
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_collision.h"
typedef uint32_t LONG;
typedef struct {unsigned held;} SRWLOCK;
#define SRWLOCK_INIT {0}
static int TryAcquireSRWLockExclusive(SRWLOCK *lock){if(lock->held)return 0;lock->held=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){assert(lock->held);lock->held=0;}
#define VK_F8 119
static void MemoryBarrier(void){}
static void InterlockedIncrement(LONG *p){++*p;}
static LONG InterlockedCompareExchange(LONG *p,LONG value,LONG expected){LONG old=*p;if(old==expected)*p=value;return old;}
typedef struct {uint32_t armed,stop,arm_id;} Shared;
static Shared control_state={1,0,1},*state=&control_state;
typedef struct {uint32_t generation,armed,stamp,flags,buttons;} ControlFrame;
typedef struct {uint32_t sequence,ticks,contacts,corrections,rejections,reason,active,generation;} CollisionTelemetry;
static CollisionTelemetry report,*collision_shared=&report;
static unsigned char player_data[0x200];static uint32_t player_value,*player_slot=&player_value;
static CubeCommand published;
static int graphics_ready=1;
static int cube_collision_graphics_ready(void){return graphics_ready;}
static uint32_t now;static unsigned f8,async_calls,late_f8,late_change,bad_writable,late_slot,torn;
static uint32_t current_player(void){return player_value==(uint32_t)(uintptr_t)player_data?player_value:0;}
static int writable(const void *p,size_t n){assert(p==(void*)(player_data+0x94) && n==12);if(late_slot)player_value=0;return !bad_writable;}
static int GetAsyncKeyState(int key){assert(key==VK_F8);async_calls++;return(f8 || (late_f8 && async_calls==2))?0x8000:0;}
static uint32_t GetTickCount(void){if(late_change) {float value;memcpy(&value,player_data+0x94,4);value+=3;memcpy(player_data+0x94,&value,4);}return now;}
static int cube_collision_command_read(CubeCommand *out){memcpy(out,&published,3168);return !torn;}
static float runtime_view_y,runtime_physics_y,runtime_support_skin=.1f,runtime_body_radius=250,runtime_body_height=1600;
static uint32_t runtime_vertical_valid,runtime_lift_generation=1,runtime_request_stamp=1000,runtime_actor_active,runtime_vertical_writes,runtime_vertical_rebase;static float runtime_actor_y,runtime_vertical_scale=1000;
#include "collision_tick.h"
static void pose(float x,float y,float z){float p[3]={x,y,z};memcpy(player_data+0x94,p,12);}
static float coordinate(unsigned i){float v;memcpy(&v,player_data+0x94+4*i,4);return v;}
static ControlFrame reset(void){
    memset(&report,0,sizeof(report));state->armed=1;state->stop=0;state->arm_id=1;collision_shared=&report;
    player_value=(uint32_t)(uintptr_t)player_data;assert((uintptr_t)player_data<=UINT32_MAX);
    vertical_actor_player=vertical_actor_generation=0;collision_player=collision_generation=0;collision_tick_lock.held=0;now=1000;
    runtime_view_y=runtime_physics_y=0;runtime_vertical_valid=runtime_actor_active=runtime_vertical_rebase=0;runtime_request_stamp=1000;runtime_support_skin=.1f;graphics_ready=1;f8=async_calls=late_f8=late_change=bad_writable=late_slot=torn=0;
    published=(CubeCommand){.enabled=1,.stamp=1000,.epoch=7,.mode=2,.count=1,.collisions=1,.arm_generation=1};
    float box[6]={0,0,-500,1000,1000,500};memcpy(published.boxes[0],box,sizeof(box));pose(-300,0,0);
    return(ControlFrame){.generation=1,.armed=1,.stamp=1000,.flags=3};
}
int main(void) {
    ControlFrame slope=reset();pose(500,0,0);published.boxes[0][4]=3000;
    runtime_physics_y=runtime_view_y=3000;runtime_vertical_valid=1;runtime_support_skin=10;cube_collision_tick(&slope,1,now);
    now+=16;slope.stamp=published.stamp=now;pose(530,-60,0);cube_collision_tick(&slope,1,now);
    assert(!report.corrections && coordinate(0)==530 && coordinate(2)==0 && coordinate(1)==3000); // native ground descends before the next vertical packet
    // Both directions and more than a support skin between vertical publications.
    for(unsigned frame=0;frame<16;frame++){
        now+=16;slope.stamp=published.stamp=now;pose(500+(frame%2)*30,frame%2?-180:90,0);
        cube_collision_tick(&slope,1,now);assert(!report.corrections && coordinate(1)==3000 && !state->stop);
    }
    ControlFrame landing=reset();pose(500,0,0);runtime_physics_y=1000;runtime_vertical_valid=1;runtime_view_y=900;runtime_support_skin=10;
    cube_collision_tick(&landing,1,now);assert(!report.corrections && coordinate(0)==500 && coordinate(2)==0);
    for(unsigned frame=0;frame<20;frame++){
        now+=16;landing.stamp=published.stamp=now;
        pose(500+(frame%2)*30,-3,0); // movement/native push on a sloping floor while the visual camera catches up
        cube_collision_tick(&landing,1,now);
        assert(!report.corrections && !state->stop && coordinate(2)==0 && coordinate(1)==1000);
    }
    published.count=2;float raised_wall[6]={1000,1000,-500,2000,2000,500};memcpy(published.boxes[1],raised_wall,24);
    now+=16;landing.stamp=published.stamp=now;pose(800,0,0);cube_collision_tick(&landing,1,now);
    assert(report.corrections==1 && coordinate(0)<750 && coordinate(1)==1000); // a real wall still blocks at the elevated feet
    landing=reset();pose(1400,0,0);runtime_physics_y=800;runtime_vertical_valid=1;runtime_view_y=1000;runtime_support_skin=10;
    cube_collision_tick(&landing,1,now);assert(!report.corrections && coordinate(0)==1400); // stepping off the edge is not pinned to the support
    ControlFrame embedded=reset();pose(500,8,0);
    cube_collision_tick(&embedded,1,now);
    assert(report.corrections==1 && (coordinate(0)<-250 || coordinate(0)>1250 || fabsf(coordinate(2))>750));
    assert(coordinate(1)==8 && !state->stop); // first reference starts inside a placed block
    embedded=reset();published.count=0;published.enabled=0;cube_collision_tick(&embedded,1,now);
    published.count=1;published.enabled=1;pose(500,8,0);now+=16;embedded.stamp=published.stamp=now;
    cube_collision_tick(&embedded,1,now);assert(report.corrections==1 && coordinate(1)==8);
    embedded=reset();cube_collision_tick(&embedded,1,now);
    pose(500,8,0);collision_previous[0]=500;collision_previous[1]=8; // block newly encloses stationary player
    cube_collision_tick(&embedded,1,now);assert(report.corrections==1 && coordinate(1)==8);
    ControlFrame paused=reset();cube_collision_tick(&paused,1,now);
    graphics_ready=0;paused.flags=11;paused.buttons=0;now+=1000;paused.stamp=published.stamp=now;
    cube_collision_tick(&paused,1,now);assert(!state->stop && collision_stamp==now);
    graphics_ready=1;paused.flags=3;pose(-240,0,0);now+=16;paused.stamp=published.stamp=now;
    cube_collision_tick(&paused,1,now);assert(!state->stop && report.corrections==1 && coordinate(0)<-250);
    ControlFrame c=reset();cube_collision_tick(&c,1,now);assert(report.active && report.reason==2 && !report.corrections);
    pose(100,8,80);cube_collision_tick(&c,1,now);assert(report.corrections==1 && coordinate(0)<-250 && coordinate(1)==8 && fabsf(coordinate(2)-80)<.001f && !collision_tick_lock.held);
    for(unsigned fail=0;fail<10;++fail){
        c=reset();cube_collision_tick(&c,1,now);pose(100,8,80);async_calls=0;
        if(fail==0)state->stop=1;
        if(fail==1)f8=1;
        if(fail==2)c.armed=0;
        if(fail==3)now=1500;
        if(fail==4)published.arm_generation=2;
        if(fail==5)torn=1;
        if(fail==6)bad_writable=1;
        if(fail==7)late_f8=1;
        if(fail==8)late_slot=1;
        if(fail==9)late_change=1;
        cube_collision_tick(&c,1,now);assert(!report.corrections && coordinate(0)==(fail==9?103:100) && coordinate(1)==8 && !collision_tick_lock.held);
    }
    c=reset();cube_collision_tick(&c,1,now);pose(1000,0,0);cube_collision_tick(&c,1,now);assert(report.reason==4 && coordinate(0)==1000);
    cube_collision_tick(&c,0,now);cube_collision_tick(&c,1,now);
    assert(report.reason==4 && report.rejections==1 && report.generation==1 && !report.active && state->stop); // cause survives dormant/torn callbacks without inflating failures
    c=reset();cube_collision_tick(&c,1,now);
    graphics_ready=0;now+=150;c.stamp=published.stamp=now;pose(322.5f,8,0);cube_collision_tick(&c,1,now);
    assert(!state->stop && !report.corrections);graphics_ready=1;cube_collision_tick(&c,1,now);
    assert(!state->stop && report.corrections==1 && coordinate(0)<-250 && coordinate(1)==8); // 4.15 units/ms; sweep retained safe pose
    c=reset();cube_collision_tick(&c,1,now);now+=150;c.stamp=published.stamp=now;pose(5000,0,0);cube_collision_tick(&c,1,now);
    assert(state->stop && report.reason==4 && !report.corrections && coordinate(0)==5000); // genuine teleport is not swept back
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);now=1101;cube_collision_tick(&c,1,now);assert(report.corrections==1 && coordinate(0)<-250);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);published.origin[0]=1;cube_collision_tick(&c,1,now);
    assert(report.reason==2 && report.corrections==1 && coordinate(0)<-249 && coordinate(1)==0); // new transform cannot adopt an embedded pose
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);collision_tick_lock.held=1;cube_collision_tick(&c,1,now);assert(coordinate(0)==100 && !report.corrections);collision_tick_lock.held=0;
    c=reset();cube_collision_tick(&c,1,now);pose(NAN,0,0);cube_collision_tick(&c,1,now);assert(!report.active && report.reason==3 && !report.corrections);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);graphics_ready=0;cube_collision_tick(&c,1,now);assert(!report.active && !report.corrections && coordinate(0)==100);
    // Continuous W with transient torn snapshots/graphics gaps must not rebaseline inside.
    c=reset();cube_collision_tick(&c,1,now);
    for(unsigned frame=1;frame<2000;frame++){
        now+=16;c.stamp=published.stamp=now;
        float advance=coordinate(0)+60;pose(advance,8,0);
        torn=frame%23==0;graphics_ready=frame%17!=0;
        cube_collision_tick(&c,1,now);
        assert(coordinate(1)==8 && !state->stop);
        if(!torn && graphics_ready)assert(coordinate(0)<-250);
    }
    assert(report.corrections>1800);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);torn=1;cube_collision_tick(&c,1,now);
    assert(coordinate(0)==100 && collision_player);torn=0;cube_collision_tick(&c,1,now);assert(coordinate(0)<-250);
    c=reset();cube_collision_tick(&c,1,now);runtime_view_y=runtime_physics_y=6000;runtime_vertical_valid=1;now+=16;c.stamp=published.stamp=now;cube_collision_tick(&c,1,now);
    assert(!state->stop && coordinate(1)==6000);pose(100,0,0);now+=16;c.stamp=published.stamp=now;cube_collision_tick(&c,1,now);
    assert(!state->stop && coordinate(0)==100 && coordinate(1)==6000); // stand above block: real elevated feet avoid horizontal contact
    for(unsigned fail=0;fail<18;fail++){
        c=reset();collision_shared=NULL;pose(100,8,80);runtime_vertical_valid=1;runtime_physics_y=1000;
        int complete=1;
        if(fail==0)state->stop=1;
        if(fail==1)f8=1;
        if(fail==2)c.armed=0;
        if(fail==3)state->armed=0;
        if(fail==4)now=1500;
        if(fail==5)runtime_request_stamp=500;
        if(fail==6)c.generation=2;
        if(fail==7)c.flags=0;
        if(fail==8)complete=0;
        if(fail==9)bad_writable=1;
        if(fail==10)late_f8=1;
        if(fail==11)late_slot=1;
        if(fail==12)late_change=1;
        if(fail==13)runtime_physics_y=NAN;
        if(fail==14)runtime_physics_y=20000100;
        if(fail==15)runtime_physics_y=129000;
        if(fail==16)state->arm_id=2;
        if(fail==17)runtime_vertical_valid=0;
        cube_collision_tick(&c,complete,now);
        assert(coordinate(0)==(fail==12?103:100) && coordinate(1)==8 && coordinate(2)==80 && !runtime_actor_active);
    }
    c=reset();collision_shared=NULL;runtime_vertical_valid=1;runtime_physics_y=-200;pose(100,8,80);
    unsigned char expected[sizeof(player_data)];memcpy(expected,player_data,sizeof(expected));memcpy(expected+0x98,&runtime_physics_y,4);
    cube_collision_tick(&c,1,now);assert(!memcmp(player_data,expected,sizeof(expected)) && runtime_actor_active); // writes only actual Y, even without any placed blocks
    now+=500;c.stamp=now;pose(100,-100,80);cube_collision_tick(&c,1,now);
    assert(coordinate(1)==-100 && !runtime_actor_active); // fresh input cannot keep an expired vertical command alive
    c=reset();collision_shared=NULL;runtime_vertical_valid=1;runtime_physics_y=420;pose(100,0,80);
    cube_collision_tick(&c,1,now);assert(coordinate(1)==420);
    runtime_vertical_valid=0;runtime_vertical_rebase=1;pose(100,500000,80);cube_collision_tick(&c,1,now);
    assert(coordinate(1)==500000 && !runtime_actor_active); // RE4 owns native stairs between jumps
    runtime_vertical_valid=1;runtime_physics_y=500420;cube_collision_tick(&c,1,now);
    assert(coordinate(1)==500420 && runtime_actor_active && !state->stop); // next jump rebases its bounded range at new native ground
    c=reset();collision_shared=NULL;cube_collision_tick(&c,1,now);assert(coordinate(0)==-300);
    puts("Actual collision callback: arm/lease/F8/generation, pointer/pose race checks, absolute Y ownership, teleport rebaseline and reentrancy passed");
}
