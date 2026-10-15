/* Engine-owned damage: no HP/routine writes. Independently implemented from the
 * ABI/layout research in boshjerns/re4-boshjerns-sdk@0383241204842ae61bc4a93d6110c32e495f365d,
 * docs/game/weapons.md and fun-combat.md. Profile installation is fail-closed. */
#ifdef _WIN32
#define COMBAT_FASTCALL __attribute__((fastcall))
#else
#define COMBAT_FASTCALL
#endif
typedef int (__cdecl *CombatDamageFn)(unsigned char*,int,int,unsigned);
typedef int (__cdecl *CombatRayFn)(float*,float*,unsigned char**,unsigned,float*,float*,int*,unsigned,unsigned);
typedef void (COMBAT_FASTCALL *CombatMessageFn)(void*,void*,unsigned,unsigned,unsigned,const float*,float,void*);
static CombatDamageFn combat_original_damage;
static CombatRayFn combat_native_ray;
static CombatMessageFn combat_native_message;
static unsigned combat_engine_ready,combat_hook_reason=1;
static uint32_t combat_queued,combat_expired,combat_last_part,combat_target_kind;
static float combat_hit_position[3];
typedef struct {
 unsigned active;uint32_t actor,guid,vtable,generation,id,slot,damage,total_damage,stamp,thread,part;
 float source[3];
} CombatPending;
static CombatPending combat_pending;
static int combat_pending_identity(const CombatPending *p){
 unsigned char *actor=(unsigned char*)(uintptr_t)p->actor;
 return readable(actor,0x408) && !memcmp(&p->vtable,actor,4) && !memcmp(&p->guid,actor+0xF8,4) && combat_kind(actor[0x100]);
}
static int combat_owned_message(const CombatPending *p){
 unsigned char *actor=(unsigned char*)(uintptr_t)p->actor;
 return combat_pending_identity(p) && actor[0x32D]==10 && actor[0x32E]==2
  && !memcmp(actor+0x330,p->source,12) && !memcmp(actor+0x340,&p->part,4);
}
static void combat_cancel_pending(void){
 if(!combat_pending.active)return;
 if(combat_owned_message(&combat_pending)){
  unsigned char *actor=(unsigned char*)(uintptr_t)combat_pending.actor;
  if((actor[0x32C]&1u) && writable(actor+0x32C,2)){actor[0x32C]&=(uint8_t)~1u;actor[0x32D]=0;}
 }
 combat_ack=combat_pending.id;combat_reason=8;combat_hp_before=combat_hp_after=0;combat_expired++;combat_pending.active=0;
}
static int combat_session_valid(uint32_t generation,uint32_t now){
 ControlFrame current;
 return read_control_frame(&current) && current.armed && current.generation==generation && (current.flags&11u)==3u && now-current.stamp<500
  && state->armed && !state->stop && state->arm_id==generation && !runtime_scripted() && !(GetAsyncKeyState(VK_F8)&0x8000);
}
static int __cdecl combat_damage_hook(unsigned char *actor,int damage,int variance,unsigned flags){
 CombatPending p=combat_pending;
 /* A different actor/source/thread must retain the game's original damage. */
 if(!p.active || p.actor!=(uint32_t)(uintptr_t)actor || !combat_owned_message(&p))return combat_original_damage(actor,damage,variance,flags);
 if(p.thread!=GetCurrentThreadId())return combat_original_damage(actor,0,0,flags);
 combat_pending.active=0;int16_t before,after;memcpy(&before,actor+0x324,2);
 int valid=combat_session_valid(p.generation,GetTickCount()) && GetTickCount()-p.stamp<500 && before>0;
 /* Native ApplyDamage retains its difficulty/no-kill policy and return value.
    Only the request's owned message replaces its amount; ordinary bullets pass through. */
 int amount=valid?(p.damage>(uint32_t)before?before:(int)p.damage):0;
 int result=combat_original_damage(actor,amount,0,flags);
 if(!combat_pending_identity(&p))return result;
 memcpy(&after,actor+0x324,2);
 combat_ack=p.id;combat_target_guid=p.guid;combat_target_slot=p.slot;
 combat_hp_before=before>0?(uint32_t)before:0;combat_hp_after=after>0?(uint32_t)after:0;
 combat_reason=valid && after<before?0:4;
 if(combat_reason==0){
  combat_hits++;if(after<=0)combat_kills++;
  CombatHurt *hurt=&combat_hurt[p.slot];
  if(!hurt->valid || hurt->generation!=p.generation || hurt->guid!=p.guid || hurt->vtable!=p.vtable || GetTickCount()-hurt->stamp>=500)
   *hurt=(CombatHurt){.generation=p.generation,.guid=p.guid,.vtable=p.vtable,.stamp=p.stamp,.damage=p.total_damage,.valid=1};
  else hurt->damage=p.total_damage;
 }
 return result;
}
static unsigned char *combat_hit_part(const CombatShared *r,unsigned char *actor){
 unsigned char *pairs[40]={0};float start[3],end[3],hit[3]={0},normal[3]={0};int material=0;
 memcpy(start,r->start,12);memcpy(end,r->end,12);
 int count=combat_native_ray(start,end,pairs,1,hit,normal,&material,2,0);
 if(count!=1 || pairs[0]!=actor || !pairs[1])return NULL;
 /* Never accept a ray result pointing outside this actor's bounded hit-part list. */
 uint32_t address=(uint32_t)(uintptr_t)(actor+0x344),seen[32];unsigned n=0;
 while(address && n<32){
  for(unsigned i=0;i<n;i++)if(seen[i]==address)return NULL;
  seen[n++]=address;unsigned char *part=(unsigned char*)(uintptr_t)address;
  if((address&3u) || !readable(part,0x34))return NULL;
  uint16_t enabled;int16_t bone;memcpy(&enabled,part+0x24,2);memcpy(&bone,part+0x26,2);
  if(part==pairs[1]){
   if(!(enabled&1u) || bone<0 || bone>actor[0x102] || !writable(part+0xC,4) || !writable(part+0x28,4))return NULL;
   float point[3],distance;memcpy(point,part+0xC,12);memcpy(&distance,part+0x28,4);
   float length=0,actual=0,t=0;for(unsigned i=0;i<3;i++){
    if(!isfinite(point[i]))return NULL;
    float d=r->end[i]-r->start[i],offset=point[i]-r->start[i];length+=d*d;actual+=offset*offset;t+=offset*d;
   }
   if(!isfinite(distance) || distance<0 || actual>(r->scale*r->reach+1)*(r->scale*r->reach+1) || fabsf(distance-actual)>fmaxf(100.f,actual*.01f) || t<0 || t>length+1)return NULL;
   float error=0;for(unsigned i=0;i<3;i++){float d=point[i]-r->start[i]-(r->end[i]-r->start[i])*t/length;error+=d*d;}
   if(error>4)return NULL;
   combat_last_part=(uint32_t)bone;return part;
  }
  memcpy(&address,part+0x30,4);
 }
 return NULL;
}
static int combat_queue_message(const CombatShared *r,unsigned char *actor,unsigned char *part,uint32_t damage,uint32_t vt,uint32_t now){
 if(actor[0x32C] || actor[0x32D] || combat_pending.active || !writable(actor+0x328,0x1C))return 0;
 float distance;memcpy(&distance,part+0x28,4);
 combat_pending=(CombatPending){.active=1,.actor=(uint32_t)(uintptr_t)actor,.guid=r->guid,.vtable=vt,.generation=r->generation,.id=r->id,.slot=r->slot,.damage=damage,.total_damage=r->damage,.stamp=now,.thread=GetCurrentThreadId(),.part=(uint32_t)(uintptr_t)part};
 memcpy(combat_pending.source,r->start,12);
 combat_native_message(actor+0x328,NULL,0,10,2,r->start,distance,part);
 if(!combat_owned_message(&combat_pending) || !(actor[0x32C]&1u)){combat_pending.active=0;return 0;}
 memcpy(combat_hit_position,part+0xC,12);combat_target_kind=actor[0x100];combat_queued++;return 1;
}
