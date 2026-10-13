static inline int runtime_scripted(void){return 0;}
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "health_logic.h"
typedef int32_t LONG;
#define MemoryBarrier() ((void)0)
#define VK_F8 119
static void InterlockedIncrement(volatile LONG *p){++*p;}
typedef struct {unsigned armed,stop;} Shared;
typedef struct {uint32_t generation,armed,stamp,flags;} ControlFrame;
static Shared memory,*state=&memory;
static unsigned char globals[0x6000],actor[0x400];
static uint32_t slot,gs,*player_slot=&slot,*global_slot=&gs,now;
static unsigned bad,f8,race;
static uint32_t current_player(void){return slot;}
static int readable(const void *p,size_t n){return p && n;}
static int writable(const void *p,size_t n){if(bad)return 0;assert(n==4 || n==2);return p==(void*)(globals+0x4FB4) || p==(void*)(actor+0x324) || p==(void*)(actor+0xFC);}
static int GetAsyncKeyState(int key){assert(key==VK_F8);return f8?0x8000:0;}
static uint32_t GetTickCount(void){if(race)slot=0;return now;}
/* ACTUAL_HEALTH_TYPES */
/* ACTUAL_HEALTH_TICK */
static HealthShared report;
static int hp(void){int16_t value;memcpy(&value,globals+0x4FB4,2);return value;}
static void pose_hp(int value){int16_t current=(int16_t)value;memcpy(globals+0x4FB4,&current,2);}
static ControlFrame reset(void){
 memset(globals,0,sizeof(globals));memset(actor,0,sizeof(actor));memset(&report,0,sizeof(report));memset(&health_ledger,0,sizeof(health_ledger));
 health_shared=&report;state->armed=1;state->stop=0;bad=f8=race=0;now=1000;
 slot=(uint32_t)(uintptr_t)actor;gs=(uint32_t)(uintptr_t)globals;assert((uintptr_t)actor<=UINT32_MAX && (uintptr_t)globals<=UINT32_MAX);
 globals[0x20]=3;actor[0xFC]=1;pose_hp(1000);int16_t maximum=1000;memcpy(globals+0x4FB6,&maximum,2);
 report.enabled=1;report.generation=1;report.mode=1;report.ratio=1;report.stamp=1000;
 return(ControlFrame){.generation=1,.armed=1,.stamp=1000,.flags=3};
}
int main(void){
 ControlFrame c=reset();health_tick(&c,1,now);assert(report.active && report.reason==0 && hp()==1000 && report.max==1000 && !(report.sequence&1));
 pose_hp(800);health_tick(&c,1,now);assert(report.damage==200000 && hp()==800 && actor[0xFC]==1);
 for(unsigned i=0;i<60;i++){health_tick(&c,1,now);}assert(report.damage==200000 && hp()==800);
 report.ratio=.8f;report.ack_damage=200000;health_tick(&c,1,now);assert(hp()==800);
 report.ratio=.9f;health_tick(&c,1,now);assert(hp()==900 && report.healing==0);
 for(unsigned fail=0;fail<10;fail++){
  c=reset();report.ratio=.5f;
  if(fail==0)state->stop=1;
  if(fail==1)f8=1;
  if(fail==2)c.armed=0;
  if(fail==3)now=1500;
  if(fail==4)report.request_sequence=1;
  if(fail==5)bad=1;
  if(fail==6)report.generation=2;
  if(fail==7)globals[0x4FC8]=2;
  if(fail==8)globals[0x20]=2;
  if(fail==9)race=1;
  health_tick(&c,1,now);assert(hp()==1000 && !report.active && !report.writes);
 }
 // Minecraft lethal health -> both HP mirrors zero and native Die routine once.
 c=reset();health_tick(&c,1,now);report.ratio=0;actor[0xFD]=8;health_tick(&c,1,now);
 int16_t mirror;memcpy(&mirror,actor+0x324,2);assert(!hp() && !mirror && report.dead && actor[0xFC]==3 && actor[0xFD]==0);
 actor[0xFD]=7;health_tick(&c,1,now);assert(actor[0xFD]==7); // never restart native death animation
 // Native death already in progress is exported even if the actor disappears.
 c=reset();health_tick(&c,1,now);pose_hp(0);slot=0;globals[0x20]=2;

 health_tick(&c,1,now);assert(report.dead && report.damage==1000000 && report.hp==0 && !report.active);
 // Actor replacement cannot inherit a live health write or revive a corpse.
 c=reset();health_tick(&c,1,now);slot+=4;report.ratio=.5f;health_tick(&c,1,now);assert(hp()==1000 && !report.active);
 puts("Actual health callback: HP/death writes, identity/role/routine, F8/lease/race/torn guards and lethal event retention passed");
}
