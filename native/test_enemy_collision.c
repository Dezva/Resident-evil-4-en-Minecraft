#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_collision.h"
typedef struct {unsigned held;} SRWLOCK;
static SRWLOCK collision_tick_lock;
static int TryAcquireSRWLockExclusive(SRWLOCK *p){if(p->held)return 0;p->held=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *p){assert(p->held);p->held=0;}
typedef struct {unsigned armed,stop;} Shared;
typedef struct {uint32_t armed,generation,stamp,flags;} ControlFrame;
static Shared storage={1,0},*state=&storage;
static CubeCommand collision_command,command;
static uint32_t manager[4],enemy_manager,runtime_enemy_available,runtime_enemy_contacts,runtime_enemy_corrections,now=1000;
static unsigned char actors[4][0x408];
static unsigned char *image_base=(unsigned char*)(uintptr_t)0x1000;static uint32_t image_size=0x10000;
static unsigned scripted,f8,bad,race;
static int runtime_scripted(void){return scripted;}
static int readable(const void *p,size_t n){return p && n;}
static int writable(const void *p,size_t n){(void)p;assert(n==12);if(race)manager[1]=0;return !bad;}
static uint32_t GetTickCount(void){return now;}
static int GetAsyncKeyState(int key){(void)key;return f8?0x8000:0;}
#define VK_F8 119
static int cube_collision_graphics_ready(void){return 1;}
static int cube_collision_command_read(CubeCommand *out){memcpy(out,&command,3168);return 1;}
#include "enemy_collision.h"
static void position(unsigned i,float x,float y,float z){float p[3]={x,y,z};memcpy(actors[i]+0x94,p,12);}
static float x(unsigned i){float value;memcpy(&value,actors[i]+0x94,4);return value;}
static ControlFrame reset(void){
    memset(actors,0,sizeof(actors));memset(enemy_references,0,sizeof(enemy_references));enemy_array=0;runtime_enemy_contacts=runtime_enemy_corrections=0;
    enemy_manager=(uint32_t)(uintptr_t)manager;assert((uintptr_t)manager<=UINT32_MAX && (uintptr_t)actors<=UINT32_MAX);
    manager[0]=0x1234;manager[1]=(uint32_t)(uintptr_t)actors;manager[2]=4;manager[3]=0x408;
    for(unsigned i=0;i<4;i++){uint32_t vt=0x1234,flags=3,guid=i+1;int16_t hp=100;memcpy(actors[i],&vt,4);memcpy(actors[i]+4,&flags,4);memcpy(actors[i]+0xF8,&guid,4);memcpy(actors[i]+0x324,&hp,2);actors[i][0xFC]=1;actors[i][0x100]=0x12;position(i,-300,8,i*1500.f);}
    actors[1][0x100]=0x18; // merchant must be untouched
    actors[2][0xFC]=3; // corpse
    command=(CubeCommand){.enabled=1,.collisions=1,.epoch=7,.stamp=1000,.arm_generation=7,.mode=2,.count=1};
    float box[6]={0,0,-500,1000,1000,5500};memcpy(command.boxes[0],box,24);
    state->armed=1;state->stop=0;now=1000;scripted=f8=bad=race=0;
    return(ControlFrame){.armed=1,.generation=7,.stamp=1000};
}
int main(void){
    ControlFrame c=reset();enemy_collision_tick(&c,1,now);
    for(unsigned tick=0;tick<1000;tick++){
        now+=16;c.stamp=command.stamp=now;position(0,x(0)+60,8,0);position(3,x(3)+60,8,4500);
        enemy_collision_tick(&c,1,now);assert(x(0)<-250 && x(3)<-250 && !state->stop);
    }
    assert(runtime_enemy_corrections==2000 && x(1)==-300 && x(2)==-300);
    float y;memcpy(&y,actors[0]+0x98,4);assert(y==8);
    for(unsigned fail=0;fail<8;fail++){
        c=reset();enemy_collision_tick(&c,1,now);position(0,100,8,0);
        if(fail==0)state->stop=1;
        if(fail==1)f8=1;
        if(fail==2)scripted=1;
        if(fail==3)command.arm_generation=8;
        if(fail==4)manager[2]=257;
        if(fail==5)bad=1;
        if(fail==6)race=1;
        if(fail==7)actors[0][0xFC]=3;
        enemy_collision_tick(&c,1,now);assert(x(0)==100 && !runtime_enemy_corrections && !collision_tick_lock.held);
    }
    c=reset();enemy_collision_tick(&c,1,now);position(0,5000,8,0);enemy_collision_tick(&c,1,now);assert(x(0)==5000 && !state->stop);
    puts("Enemy callback: repeated humanoid sweeps, merchant/dead/script/teleport exclusion, bounded manager, F8/lease/generation/race and Y preservation passed");
}
