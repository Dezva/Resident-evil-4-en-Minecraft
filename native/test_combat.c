#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_collision.h"
typedef int32_t LONG;
typedef struct {unsigned held;} SRWLOCK;
#define SRWLOCK_INIT {0}
static int TryAcquireSRWLockExclusive(SRWLOCK *p){if(p->held)return 0;p->held=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *p){assert(p->held);p->held=0;}
static void MemoryBarrier(void){}
static void InterlockedIncrement(volatile LONG *p){++*p;}
typedef struct {uint32_t armed,stop,arm_id;} Shared;
typedef struct {uint32_t armed,generation,stamp,flags;float eye;} ControlFrame;
static Shared storage,*state=&storage;
static unsigned char player[0x200],actors[3][0x408];static uint32_t player_value,*player_slot=&player_value;
static uint32_t manager[4],enemy_manager,enemy_candidates[64];static unsigned enemy_candidate_count;
static unsigned char *image_base=(unsigned char*)(uintptr_t)0x1000;static uint32_t image_size=0x10000;
static uint32_t now;static unsigned scripted,f8,torn,bad,race,late_f8,polls;
static int runtime_scripted(void){return scripted;}
static uint32_t current_player(void){return player_value;}
static int readable(const void *p,size_t n){return p && n;}
static int readable_span(const void *p,size_t n){return readable(p,n);}
static int writable(const void *p,size_t n){(void)p;assert(n==2 || n==4);return !bad;}
static uint32_t GetTickCount(void){if(race){uint32_t value=999;memcpy(actors[0]+0x324,&value,2);}return now;}
#define VK_F8 119
static int GetAsyncKeyState(int key){assert(key==VK_F8);polls++;return f8 || (late_f8 && polls>=2)?0x8000:0;}
static SRWLOCK collision_tick_lock=SRWLOCK_INIT;static CubeCommand collision_command,published;
static int cube_collision_command_read(CubeCommand *out){memcpy(out,&published,3168);return !torn;}
#include "runtime_discovery.h"
/* ACTUAL_ENEMY_HELPERS */
/* ACTUAL_COMBAT */
static CombatShared report;
static int hp(unsigned i){int16_t n;memcpy(&n,actors[i]+0x324,2);return n;}
static ControlFrame reset(void){
 memset(&report,0,sizeof(report));combat_shared=&report;combat_lock.held=collision_tick_lock.held=0;combat_generation=combat_ack=combat_hits=combat_kills=combat_attempts=0;combat_reason=1;
 memset(combat_hurt,0,sizeof(combat_hurt));memset(actors,0,sizeof(actors));memset(player,0,sizeof(player));
 player_value=(uint32_t)(uintptr_t)player;assert((uintptr_t)player<=UINT32_MAX);*state=(Shared){1,0,7};now=1000;scripted=f8=torn=bad=race=late_f8=polls=0;
 enemy_manager=(uint32_t)(uintptr_t)manager;enemy_candidate_count=0;manager[0]=0x1234;manager[1]=(uint32_t)(uintptr_t)actors;manager[2]=3;manager[3]=0x408;
 for(unsigned i=0;i<3;i++){uint32_t vt=0x1234,flags=1,guid=10+i;int16_t value=1000;float pos[3]={2000-i*300.f,0,0};memcpy(actors[i],&vt,4);memcpy(actors[i]+4,&flags,4);memcpy(actors[i]+0xf8,&guid,4);memcpy(actors[i]+0x324,&value,2);memcpy(actors[i]+0x326,&value,2);memcpy(actors[i]+0x94,pos,12);actors[i][0x100]=0x12;actors[i][0xfc]=1;}
 actors[1][0x100]=0x18;actors[2][0xfc]=3;
 published=(CubeCommand){.enabled=0};
 report.stamp=1000;report.enabled=1;report.generation=7;report.id=1;report.damage=100;report.scale=1000;report.reach=3;report.slot=0;report.guid=10;report.start[1]=report.end[1]=1000;report.end[0]=3000;
 return(ControlFrame){.armed=1,.generation=7,.stamp=1000,.flags=3,.eye=1000};
}
int main(void){
 ControlFrame c=reset();combat_tick(&c,1,now);assert(hp(0)==900 && hp(1)==1000 && hp(2)==1000 && report.reason==0 && report.hits==1 && report.ack==1 && report.count==1);
 for(unsigned i=0;i<20;i++){combat_tick(&c,1,now);}
 assert(hp(0)==900 && report.hits==1);
 report.id=2;combat_tick(&c,1,now);assert(hp(0)==900 && report.reason==5 && report.ack==2);
 report.id=3;report.damage=300;combat_tick(&c,1,now);assert(hp(0)==700 && report.hp_before==900 && report.hp_after==700);
 now=1600;c.stamp=report.stamp=now;report.id=4;combat_tick(&c,1,now);assert(hp(0)==400);
 now=2200;c.stamp=report.stamp=now;report.id=5;report.damage=500;
 unsigned char expected[sizeof(actors[0])];memcpy(expected,actors[0],sizeof(expected));expected[0x324]=expected[0x325]=0;memset(expected+0xfc,0,4);expected[0xfc]=3;
 combat_tick(&c,1,now);assert(!hp(0) && report.kills==1 && report.count==0 && !memcmp(expected,actors[0],sizeof(expected)));
 for(unsigned fail=0;fail<20;fail++){
  c=reset();int complete=1;
  if(fail==0)state->stop=1;
  if(fail==1)f8=1;
  if(fail==2)scripted=1;
  if(fail==3)c.flags=11;
  if(fail==4)c.generation=8;
  if(fail==5)report.generation=8;
  if(fail==6)now=1500;
  if(fail==7)report.request_sequence=1;
  if(fail==8)bad=1;
  if(fail==9)race=1;
  if(fail==10)manager[2]=4097;
  if(fail==11)report.damage=50001;
  if(fail==12)report.end[0]=7000;
  if(fail==13)report.guid=999;
  if(fail==14)complete=0;
  if(fail==15)late_f8=1;
  if(fail==16)report.start[1]=report.end[1]=5000;
  if(fail==17)report.scale=NAN;
  if(fail==18){float nan=NAN;memcpy(player+0x98,&nan,4);}
  if(fail==19)torn=1;
  combat_tick(&c,complete,now);assert(hp(0)==(fail==9?999:1000) && !report.hits && !report.kills && !combat_lock.held);
 }
 c=reset();published=(CubeCommand){.enabled=1,.epoch=1,.mode=2,.stamp=now,.arm_generation=7,.count=1};float wall[6]={1000,0,-500,1100,2000,500};memcpy(published.boxes[0],wall,24);
 combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==3);
 c=reset();published=(CubeCommand){.enabled=1,.epoch=1,.mode=2,.stamp=now,.arm_generation=7,.count=1};published.origin[3]=1.57079632679f;float rotated[6]={-500,0,-1100,500,2000,-1000};memcpy(published.boxes[0],rotated,24);
 combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==3);
 c=reset();actors[2][0xfc]=1;combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==2 && report.count==2); // nearer actor intercepts; never substitute another target
 c=reset();float y=3000;memcpy(player+0x98,&y,4);report.start[1]=report.end[1]=4000;combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==2); // height and reach apply in all three axes
 c=reset();combat_tick(&c,1,now);now=1600;c.stamp=report.stamp=now;report.id=2;report.guid=20;memcpy(actors[0]+0xf8,&report.guid,4);combat_tick(&c,1,now);assert(hp(0)==800); // recycled slot gets its own hurt window
 c=reset();report.id=0;combat_tick(&c,1,now);assert(report.active && report.count==1 && hp(0)==1000 && !report.attempts); // catalogue alone never attacks
 puts("Melee callback: native target/HP/death, replay/iframe/differential damage, identity, walls, 3D reach, F8/menu/TTL/races and read-only catalogue passed");
}
