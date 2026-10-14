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
static uint32_t manager[4],enemy_manager,runtime_enemy_available,runtime_enemy_contacts,runtime_enemy_corrections,runtime_enemy_reason,runtime_enemy_scanned,runtime_enemy_eligible,runtime_enemy_capacity,runtime_enemy_stride,now=1000;
static unsigned char actors[300][0x408];
static unsigned char observed_pool[60][3592];static uint32_t other_managers[2][4];
static unsigned char *image_base=(unsigned char*)(uintptr_t)0x1000;static uint32_t image_size=0x10000;
static unsigned scripted,f8,bad,race;
static int runtime_scripted(void){return scripted;}
static int readable(const void *p,size_t n){return p && n;}
static int readable_span(const void *p,size_t n){return readable(p,n);}
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
    enemy_candidate_count=0;memset(enemy_candidates,0,sizeof(enemy_candidates));
    memset(actors,0,sizeof(actors));memset(enemy_references,0,sizeof(enemy_references));enemy_array=0;runtime_enemy_contacts=runtime_enemy_corrections=0;
    enemy_manager=(uint32_t)(uintptr_t)manager;assert((uintptr_t)manager<=UINT32_MAX && (uintptr_t)actors<=UINT32_MAX);
    manager[0]=0x1234;manager[1]=(uint32_t)(uintptr_t)actors;manager[2]=300;manager[3]=0x408;
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
        if(fail==4)manager[2]=4097;
        if(fail==5)bad=1;
        if(fail==6)race=1;
        if(fail==7)actors[0][0xFC]=3;
        enemy_collision_tick(&c,1,now);assert(x(0)==100 && !runtime_enemy_corrections && !collision_tick_lock.held);
    }
    c=reset();actors[299][0x100]=0x12;actors[299][0xFC]=1;uint32_t flags=1,vt=0x1234;int16_t hp=100;memcpy(actors[299],&vt,4);memcpy(actors[299]+4,&flags,4);memcpy(actors[299]+0x324,&hp,2);position(299,-300,8,0);enemy_collision_tick(&c,1,now);position(299,100,8,0);enemy_collision_tick(&c,1,now);assert(x(299)<-250 && runtime_enemy_capacity==300 && runtime_enemy_scanned==300);
    c=reset();position(0,500,8,0);enemy_collision_tick(&c,1,now);assert(x(0)<-250 || x(0)>1250); // placed around an enemy
    c=reset();enemy_collision_tick(&c,1,now);position(0,5000,8,0);enemy_collision_tick(&c,1,now);assert(x(0)==5000 && !state->stop);
    c=reset();memset(observed_pool,0,sizeof(observed_pool));
    manager[1]=(uint32_t)(uintptr_t)observed_pool;manager[2]=60;manager[3]=3592;
    for(unsigned i=0;i<2;i++){
        memcpy(other_managers[i],manager,16);other_managers[i][2]=i?380:100;other_managers[i][3]=i?988:468;
        enemy_candidates[i]=(uint32_t)(uintptr_t)other_managers[i];
    }
    enemy_candidates[2]=(uint32_t)(uintptr_t)manager;enemy_candidate_count=3;enemy_manager=0;
    uint32_t actor_vt=0x1234,actor_flags=1,actor_guid=99;int16_t actor_hp=100;float p[3]={-300,8,0};
    memcpy(observed_pool[0],&actor_vt,4);memcpy(observed_pool[0]+4,&actor_flags,4);memcpy(observed_pool[0]+0xf8,&actor_guid,4);
    observed_pool[0][0xfc]=1;observed_pool[0][0x100]=0x12;memcpy(observed_pool[0]+0x324,&actor_hp,2);memcpy(observed_pool[0]+0x94,p,12);
    enemy_collision_tick(&c,1,now);assert(enemy_manager==(uint32_t)(uintptr_t)manager && runtime_enemy_scanned==60 && runtime_enemy_eligible==1);
    p[0]=-240;memcpy(observed_pool[0]+0x94,p,12);now+=16;c.stamp=command.stamp=now;
    enemy_collision_tick(&c,1,now);memcpy(p,observed_pool[0]+0x94,12);assert(p[0]<-250 && p[1]==8 && runtime_enemy_corrections==1);
    memcpy(other_managers[0],manager,16);enemy_collision_tick(&c,1,now);
    assert(!enemy_manager && !runtime_enemy_available && runtime_enemy_reason==6 && runtime_enemy_corrections==1); // genuine typed ambiguity never chooses first
    puts("Enemy callback: repeated humanoid sweeps, merchant/dead/script/teleport exclusion, bounded manager, F8/lease/generation/race and Y preservation passed");
}
