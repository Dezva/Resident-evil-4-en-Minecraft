#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "block_collision.h"
#ifndef _WIN32
#define __cdecl
#endif
static uint32_t thread_id=1;
static uint32_t GetCurrentThreadId(void){return thread_id;}
typedef int32_t LONG;
typedef struct {unsigned held;} SRWLOCK;
#define SRWLOCK_INIT {0}
static int TryAcquireSRWLockExclusive(SRWLOCK *p){if(p->held)return 0;p->held=1;return 1;}
static void AcquireSRWLockExclusive(SRWLOCK *p){assert(!p->held);p->held=1;}
static void ReleaseSRWLockExclusive(SRWLOCK *p){assert(p->held);p->held=0;}
static void MemoryBarrier(void){}
static void InterlockedIncrement(volatile LONG *p){++*p;}
typedef struct {uint32_t armed,stop,arm_id;} Shared;
typedef struct {uint32_t armed,generation,stamp,flags;float eye;} ControlFrame;
static Shared storage,*state=&storage;
static ControlFrame live_control;
static int read_control_frame(ControlFrame *out){*out=live_control;return 1;}
static unsigned char player[0x200],actors[3][0x408],extra_part[0x34];static uint32_t player_value,*player_slot=&player_value;
static uint32_t manager[4],enemy_manager,enemy_candidates[64];static unsigned enemy_candidate_count;
static unsigned char *image_base=(unsigned char*)(uintptr_t)0x1000;static uint32_t image_size=0x10000;
static uint32_t now;static unsigned scripted,f8,torn,bad,race,late_f8,polls,ray_miss,ray_calls,message_calls,damage_calls;
static int runtime_scripted(void){return scripted;}
static uint32_t current_player(void){return player_value;}
static int readable(const void *p,size_t n){return p && n;}
static int readable_span(const void *p,size_t n){return readable(p,n);}
static int writable(const void *p,size_t n){return p && n && !bad;}
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
static int original_damage(unsigned char *actor,int damage,int variance,unsigned flags){
 damage_calls++;assert(variance==0 || variance==12);int16_t before;memcpy(&before,actor+0x324,2);
 int16_t after=damage>=before?0:before-damage;if((flags&1) && !after)after=1;
 memcpy(actor+0x324,&after,2);return after;
}
static int engine_ray(float *start,float *end,unsigned char **pairs,unsigned max,float *hit,float *normal,int *material,unsigned weapon,unsigned flags){
 ray_calls++;assert(max==1 && weapon==2 && !flags);(void)normal;(void)material;if(ray_miss)return 0;
 pairs[0]=actors[0];pairs[1]=actors[0]+0x344;
 if(extra_part[0x24])pairs[1]=extra_part;
 float distance=0;for(unsigned i=0;i<3;i++){hit[i]=start[i]+.6f*(end[i]-start[i]);float d=hit[i]-start[i];distance+=d*d;}
 memcpy(pairs[1]+0xC,hit,12);memcpy(pairs[1]+0x28,&distance,4);return 1;
}
static void COMBAT_FASTCALL engine_message(void *address,void *edx,unsigned type,unsigned kind,unsigned weapon,const float *source,float distance,void *part){
 assert(!edx && type==0 && kind==10 && weapon==2);message_calls++;unsigned char *p=address;
 memset(p,0,0x1C);p[4]=1;p[5]=(uint8_t)kind;p[6]=(uint8_t)weapon;memcpy(p+8,source,12);memcpy(p+0x14,&distance,4);uint32_t ptr=(uint32_t)(uintptr_t)part;memcpy(p+0x18,&ptr,4);
}
static void consume(void){live_control.stamp=now;combat_damage_hook(actors[0],150,12,0);actors[0][0x32C]=actors[0][0x32D]=0;}
static ControlFrame reset(void){
 memset(&report,0,sizeof(report));combat_shared=&report;combat_lock.held=collision_tick_lock.held=0;combat_consumed=combat_generation=combat_ack=combat_hits=combat_kills=combat_attempts=0;combat_reason=1;
 memset(&combat_pending,0,sizeof(combat_pending));memset(combat_hurt,0,sizeof(combat_hurt));memset(actors,0,sizeof(actors));memset(extra_part,0,sizeof(extra_part));memset(player,0,sizeof(player));
 combat_engine_ready=combat_hazards_ready=1;combat_original_damage=original_damage;combat_native_ray=engine_ray;combat_native_message=engine_message;combat_queued=combat_expired=combat_boxes_broken=0;
 player_value=(uint32_t)(uintptr_t)player;assert((uintptr_t)player<=UINT32_MAX);*state=(Shared){1,0,7};now=1000;thread_id=1;scripted=f8=torn=bad=race=late_f8=polls=ray_miss=ray_calls=message_calls=damage_calls=0;
 enemy_manager=(uint32_t)(uintptr_t)manager;enemy_candidate_count=0;manager[0]=0x1234;manager[1]=(uint32_t)(uintptr_t)actors;manager[2]=3;manager[3]=0x408;
 for(unsigned i=0;i<3;i++){uint32_t vt=0x1234,flags=0x21,guid=10+i;int16_t value=1000;float pos[3]={2000-i*300.f,0,0};memcpy(actors[i],&vt,4);memcpy(actors[i]+4,&flags,4);memcpy(actors[i]+0xf8,&guid,4);memcpy(actors[i]+0x324,&value,2);memcpy(actors[i]+0x326,&value,2);memcpy(actors[i]+0x94,pos,12);actors[i][0x100]=0x12;actors[i][0xfc]=1;actors[i][0x102]=20;actors[i][0x368]=1;actors[i][0x36A]=6;}
 actors[1][0x100]=0x18;actors[2][0xfc]=3;
 published=(CubeCommand){.enabled=0};
 report.stamp=1000;report.enabled=1;report.generation=7;report.id=1;report.damage=100;report.scale=1000;report.reach=3;report.slot=0;report.guid=10;report.start[1]=report.end[1]=1000;report.end[0]=3000;
 live_control=(ControlFrame){.armed=1,.generation=7,.stamp=1000,.flags=3,.eye=1000};return live_control;
}
int main(void){
 // A distant arrow segment is not a distant melee swing. Native ray, identity,
 // pending-message confirmation, one-projectile ownership and stops still apply.
 for(unsigned fail=0;fail<14;fail++){
  ControlFrame shot=reset();float target[3]={14000,0,0};memcpy(actors[0]+0x94,target,12);
  report.kind=1;report.projectile=80;report.start[0]=12000;report.end[0]=15000;
  if(fail==1)report.kind=0;
  if(fail==2)report.projectile=0;
  if(fail==3)report.kind=2;
  if(fail==4)report.end[0]=19000;
  if(fail==5)f8=1;
  if(fail==6)ray_miss=1;
  if(fail==7)report.guid=99;
  if(fail==8)now=1500;
  if(fail==9)report.generation=8;
  if(fail==10){published=(CubeCommand){.enabled=1,.epoch=1,.mode=2,.stamp=now,.arm_generation=7,.count=1};float wall[6]={13000,0,-500,13100,2000,500};memcpy(published.boxes[0],wall,24);}
  if(fail==11){target[0]=67000;memcpy(actors[0]+0x94,target,12);report.start[0]=65000;report.end[0]=68000;}
  if(fail==12)report.request_sequence=1;
  if(fail==13)shot.flags=11;
  combat_tick(&shot,1,now);
  if(fail){assert(!message_calls && hp(0)==1000);continue;}
  assert(message_calls==1 && hp(0)==1000);consume();combat_tick(&shot,1,now);
  assert(hp(0)==900 && report.ack==1 && report.reason==0);
  now=1600;shot.stamp=report.stamp=live_control.stamp=now;report.id=2;
  combat_tick(&shot,1,now);assert(message_calls==1 && hp(0)==900); // replay, new request ID
  report.projectile=81;report.id=3;combat_tick(&shot,1,now);assert(message_calls==2);
 }
 {
  ControlFrame shot=reset();report.kind=1;report.damage=1;
  for(unsigned i=1;i<=130;i++){
   now=1000+600*i;shot.stamp=report.stamp=live_control.stamp=now;report.id=report.projectile=i;
   combat_tick(&shot,1,now);assert(message_calls==i);consume();combat_tick(&shot,1,now);
  }
  assert(combat_projectile_count==128 && combat_projectile_high==2);
  report.id=131;report.projectile=1;combat_tick(&shot,1,now);assert(message_calls==130); // eviction never permits replay
 }
 {
  ControlFrame low=reset();actors[0][0x100]=0x2A;float feet=-500;memcpy(player+0x98,&feet,4);
  report.start[1]=report.end[1]=-200;low.eye=300;live_control=low;
  combat_tick(&low,1,now);assert(message_calls==1 && hp(0)==1000 && !damage_calls);
 }
 // Trap handlers perform their own native transition without ApplyDamage.
 for(unsigned variant=0;variant<3;variant++)for(unsigned stage=0;stage<4;stage++){
  ControlFrame hazard=reset();actors[0][0x100]=0x2A;actors[0][0x101]=(uint8_t)variant;actors[0][0xFD]=variant?6:0;
  report.start[1]=report.end[1]=300;hazard.eye=300;live_control=hazard;
  combat_tick(&hazard,1,now);assert(message_calls==1 && !report.hits && hp(0)==1000 && !report.ack);
  int16_t zero=0;memcpy(actors[0]+0x324,&zero,2);
  if(stage>=1)actors[0][0xFD]=variant?7:3;
  if(stage>=2)actors[0][0x32C]=0;
  if(stage==3)actors[0][0x101]=99; // same address reused for another variant cannot acknowledge
  unsigned char after[0x408];memcpy(after,actors[0],sizeof(after));combat_tick(&hazard,1,now);
  assert(!memcmp(after,actors[0],sizeof(after)) && !report.kills && !report.boxes_broken && !damage_calls);
  assert(report.hits==(stage==2) && report.ack==(stage==2));
  if(stage==2){combat_tick(&hazard,1,now);assert(report.hits==1 && !combat_pending.active);}
 }
 for(unsigned gate=0;gate<5;gate++){
  ControlFrame hazard=reset();actors[0][0x100]=0x2A;report.start[1]=report.end[1]=300;hazard.eye=300;live_control=hazard;
  if(gate==0)combat_hazards_ready=0;
  if(gate==1)actors[0][0xFD]=3;
  if(gate==2){actors[0][0x101]=1;actors[0][0xFD]=7;}
  if(gate==3)f8=1;
  if(gate==4)scripted=1;
  combat_tick(&hazard,1,now);assert(!message_calls && hp(0)==1000 && !report.hits);
 }
 ControlFrame c=reset();combat_tick(&c,1,now);
 assert(hp(0)==1000 && actors[0][0xFC]==1 && report.hits==0 && report.ack==0 && combat_pending.active && report.pending_id==1 && ray_calls==1 && message_calls==1);
 assert(report.hit_part==6 && fabsf(report.hit_position[0]-1800)<.01f); // native ray owns the impact and bone
 for(unsigned i=0;i<20;i++)combat_tick(&c,1,now);
 assert(hp(0)==1000 && message_calls==1); // pending request is never resent
 consume();combat_tick(&c,1,now);assert(hp(0)==900 && hp(1)==1000 && hp(2)==1000 && report.reason==0 && report.hits==1 && report.ack==1);
 report.id=2;combat_tick(&c,1,now);assert(hp(0)==900 && report.reason==5 && report.ack==2);
 report.id=3;report.damage=300;combat_tick(&c,1,now);assert(hp(0)==900);consume();combat_tick(&c,1,now);assert(hp(0)==700 && report.hp_before==900 && report.hp_after==700);
 now=1600;c.stamp=report.stamp=live_control.stamp=now;report.id=4;combat_tick(&c,1,now);consume();combat_tick(&c,1,now);assert(hp(0)==400);
 now=2200;c.stamp=report.stamp=live_control.stamp=now;report.id=5;report.damage=500;combat_tick(&c,1,now);assert(hp(0)==400 && actors[0][0xFC]==1);consume();combat_tick(&c,1,now);assert(!hp(0) && report.kills==1 && actors[0][0xFC]==1); // RE4's caller, never the bridge, chooses death state
 for(unsigned fail=0;fail<24;fail++){
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
  if(fail==20)ray_miss=1;
  if(fail==21)combat_engine_ready=0;
  if(fail==22)actors[0][0x32C]=1;
  if(fail==23)actors[0][0x36A]=99;
  combat_tick(&c,complete,now);assert(hp(0)==(fail==9?999:1000) && !report.hits && !report.kills && !message_calls && !combat_lock.held);
 }
 c=reset();published=(CubeCommand){.enabled=1,.epoch=1,.mode=2,.stamp=now,.arm_generation=7,.count=1};float wall[6]={1000,0,-500,1100,2000,500};memcpy(published.boxes[0],wall,24);
 combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==3 && !message_calls);
 c=reset();published=(CubeCommand){.enabled=1,.epoch=1,.mode=2,.stamp=now,.arm_generation=7,.count=1};float edge_wall[6]={1770,0,-500,1790,2000,500};memcpy(published.boxes[0],edge_wall,24);
 combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==3 && ray_calls==1 && !message_calls); // block between broad-phase and actual native impact
 c=reset();actors[2][0xfc]=1;combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==2 && report.count==2);
 c=reset();float y=3000;memcpy(player+0x98,&y,4);report.start[1]=report.end[1]=4000;combat_tick(&c,1,now);assert(hp(0)==1000 && report.reason==2);
 c=reset();report.id=0;combat_tick(&c,1,now);assert(report.active && report.count==1 && hp(0)==1000 && !report.attempts && !ray_calls);
 c=reset();combat_tick(&c,1,now);now=1500;combat_tick(&c,1,now);assert(!combat_pending.active && !actors[0][0x32C] && hp(0)==1000 && report.expired==1 && report.ack==1);
 c=reset();combat_tick(&c,1,now);state->stop=1;consume();combat_tick(&c,1,now);assert(hp(0)==1000 && report.hits==0 && report.reason==4);
 c=reset();combat_tick(&c,1,now);uint32_t different=99;memcpy(actors[0]+0xF8,&different,4);assert(combat_damage_hook(actors[0],50,0,0)==950);assert(combat_ack==0 && !combat_hits); // foreign/recycled damage passes unchanged
 c=reset();combat_tick(&c,1,now);live_control.flags=11;consume();assert(hp(0)==1000 && !combat_hits); // menu arriving after submission blocks native damage
 c=reset();combat_tick(&c,1,now);thread_id=2;consume();combat_tick(&c,1,now);assert(hp(0)==900 && report.ack==1 && report.hits==1 && report.callback_thread_changed); // native update on a different thread must not receive zero damage
 c=reset();report.damage=1500;combat_tick(&c,1,now);combat_damage_hook(actors[0],150,0,1);combat_tick(&c,1,now);assert(hp(0)==0 && report.kills==1 && actors[0][0xFC]==1); // native nonlethal flag cannot make our Minecraft hit immortal
 c=reset();assert(combat_damage_hook(actors[1],1500,0,1)==1 && !combat_hits); // ordinary nonlethal native shots retain the policy
 c=reset();assert(combat_damage_hook(actors[1],50,0,0)==950 && !combat_hits); // ordinary native bullets untouched
 for(unsigned kind=0;kind<3;kind++){
  c=reset();actors[0][0x100]=(uint8_t)(kind==0?0x23:kind==1?0x53:0x58);report.start[1]=report.end[1]=300;c.eye=300;combat_tick(&c,1,now);assert(message_calls==1 && hp(0)==1000);consume();combat_tick(&c,1,now);assert(hp(0)==900 && report.target_kind==actors[0][0x100]);
 }
 // cEmBox native update can break without invoking the HP helper. The bridge
 // must wait for the observed broken state, never manufacture HP/routine writes.
 for(unsigned stage=0;stage<4;stage++){
  c=reset();actors[0][0x100]=0x53;actors[0][4]|=2;combat_tick(&c,1,now);
  assert(message_calls==1 && hp(0)==1000 && actors[0][0xFD]==0 && !report.hits);
  int16_t zero=0;memcpy(actors[0]+0x324,&zero,2);
  if(stage>=1)actors[0][0xFD]=1;
  if(stage>=2)actors[0][4]&=(uint8_t)~2u;
  if(stage>=3)actors[0][0x32C]=actors[0][0x32D]=0;
  unsigned char native_after[0x408];memcpy(native_after,actors[0],sizeof(native_after));
  combat_tick(&c,1,now);
  assert(!memcmp(native_after,actors[0],sizeof(native_after)));
  assert(report.hits==(stage==3) && report.boxes_broken==(stage==3));
  if(stage==3){assert(report.ack==1 && report.reason==0 && report.hp_before==1000 && !report.hp_after && !report.kills);combat_tick(&c,1,now);assert(report.boxes_broken==1);}
 }
 c=reset();actors[0][0x100]=0x53;actors[0][4]|=2;report.damage=1500;combat_tick(&c,1,now);
 combat_damage_hook(actors[0],150,0,1);assert(!hp(0) && !combat_ack && combat_pending.active==3 && !combat_hits);
 actors[0][0xFD]=1;actors[0][4]&=(uint8_t)~2u;actors[0][0x32C]=actors[0][0x32D]=0;
 combat_tick(&c,1,now);assert(report.ack==1 && report.boxes_broken==1 && !report.kills);
 c=reset();actors[0][0x100]=0x53;actors[0][4]|=2;report.damage=1500;combat_tick(&c,1,now);combat_damage_hook(actors[0],150,0,0);
 now=1500;combat_tick(&c,1,now);assert(!combat_pending.active && report.reason==8 && report.ack==1 && !report.boxes_broken && !hp(0) && actors[0][0xFD]==0); // no forced broken state, no indefinitely occupied queue
 c=reset();actors[0][0x100]=0x23;int16_t crow_hp=10;memcpy(actors[0]+0x324,&crow_hp,2);memcpy(actors[0]+0x326,&crow_hp,2);report.start[1]=report.end[1]=300;c.eye=300;
 combat_tick(&c,1,now);thread_id=2;combat_damage_hook(actors[0],5,0,1);combat_tick(&c,1,now);assert(!hp(0) && report.kills==1 && report.hp_before==10);
 for(unsigned bone=1;bone<=12;bone++){
  c=reset();uint32_t ptr=(uint32_t)(uintptr_t)extra_part;memcpy(actors[0]+0x374,&ptr,4);extra_part[0x24]=1;memcpy(extra_part+0x26,&bone,2);
  combat_tick(&c,1,now);assert(combat_pending.part==ptr && report.hit_part==bone);consume();assert(hp(0)==900);
 }
 puts("Native messages: asynchronous HP confirmation, actual ray/bone selection, crow/box/barrel, no routine writes, ordinary bullets, expiry/F8/identity, walls/reach/replay passed");
}
