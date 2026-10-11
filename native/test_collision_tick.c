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
typedef struct {uint32_t armed,stop;} Shared;
static Shared control_state={1,0},*state=&control_state;
typedef struct {uint32_t generation,armed,stamp;} ControlFrame;
typedef struct {uint32_t sequence,ticks,contacts,corrections,rejections,reason,active,generation;} CollisionTelemetry;
static CollisionTelemetry report,*collision_shared=&report;
static unsigned char player_data[0xB0];static uint32_t player_value,*player_slot=&player_value;
static CubeCommand published;
static int graphics_ready=1;
static int cube_collision_graphics_ready(void){return graphics_ready;}
static uint32_t now;static unsigned f8,async_calls,late_f8,late_change,bad_writable,late_slot,torn;
static uint32_t current_player(void){return player_value==(uint32_t)(uintptr_t)player_data?player_value:0;}
static int writable(const void *p,size_t n){assert(p==(void*)(player_data+0x94) && n==12);if(late_slot)player_value=0;return !bad_writable;}
static int GetAsyncKeyState(int key){assert(key==VK_F8);async_calls++;return(f8 || (late_f8 && async_calls==2))?0x8000:0;}
static uint32_t GetTickCount(void){if(late_change) {float value;memcpy(&value,player_data+0x94,4);value+=3;memcpy(player_data+0x94,&value,4);}return now;}
static int cube_command_read(CubeCommand *out){*out=published;return !torn;}
#include "collision_tick.h"
static void pose(float x,float y,float z){float p[3]={x,y,z};memcpy(player_data+0x94,p,12);}
static float coordinate(unsigned i){float v;memcpy(&v,player_data+0x94+4*i,4);return v;}
static ControlFrame reset(void){
    memset(&report,0,sizeof(report));state->armed=1;state->stop=0;collision_shared=&report;
    player_value=(uint32_t)(uintptr_t)player_data;assert((uintptr_t)player_data<=UINT32_MAX);
    collision_player=collision_generation=0;collision_tick_lock.held=0;now=1000;
    graphics_ready=1;f8=async_calls=late_f8=late_change=bad_writable=late_slot=torn=0;
    published=(CubeCommand){.enabled=1,.stamp=1000,.epoch=7,.mode=2,.count=1,.collisions=1,.arm_generation=1};
    float box[6]={0,0,-500,1000,1000,500};memcpy(published.boxes[0],box,sizeof(box));pose(-300,0,0);
    return(ControlFrame){.generation=1,.armed=1,.stamp=1000};
}
int main(void) {
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
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);now=1101;cube_collision_tick(&c,1,now);assert(report.reason==2 && coordinate(0)==100);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);published.origin[0]=1;cube_collision_tick(&c,1,now);assert(report.reason==2 && coordinate(0)==100);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);collision_tick_lock.held=1;cube_collision_tick(&c,1,now);assert(coordinate(0)==100 && !report.corrections);collision_tick_lock.held=0;
    c=reset();cube_collision_tick(&c,1,now);pose(NAN,0,0);cube_collision_tick(&c,1,now);assert(!report.active && report.reason==3 && !report.corrections);
    c=reset();cube_collision_tick(&c,1,now);pose(100,0,0);graphics_ready=0;cube_collision_tick(&c,1,now);assert(!report.active && !report.corrections && coordinate(0)==100);
    c=reset();collision_shared=NULL;cube_collision_tick(&c,1,now);assert(coordinate(0)==-300);
    puts("Actual collision callback: arm/lease/F8/generation, pointer/pose race checks, no Y writes, teleport rebaseline and reentrancy passed");
}
