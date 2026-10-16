#include "combat_policy.h"
static CombatShared *combat_shared;static HANDLE combat_mapping;
static SRWLOCK combat_lock=SRWLOCK_INIT;
static uint32_t combat_consumed;
/* One projectile can enqueue at most one native impact per control generation.
   Bounded by monotonically increasing Minecraft entity IDs, no evictable replay ring. */
static uint32_t combat_projectile_high,combat_projectile_ids[128];
static unsigned combat_projectile_count;
static uint32_t combat_generation,combat_ack,combat_reason=1,combat_target_guid,combat_target_slot,combat_hp_before,combat_hp_after,combat_hits,combat_kills,combat_attempts;
static CombatActor combat_catalog[COMBAT_MAX_ACTORS];static uint32_t combat_count,combat_pool[4],combat_manager;
typedef struct {uint32_t generation,guid,vtable,stamp,damage;unsigned valid;} CombatHurt;
static CombatHurt combat_hurt[4096];
static unsigned combat_hazards_ready;
static int combat_kind(unsigned id){return enemy_kind(id) || id==0x23 || (id==0x2A && combat_hazards_ready) || id==0x53 || id==0x55 || id==0x56 || id==0x58;}
#include "combat_engine.h"
static void combat_publish(unsigned active,uint32_t now){
 if(!combat_shared)return;
 InterlockedIncrement(&combat_shared->sequence);MemoryBarrier();
 memcpy(combat_shared->hit_position,combat_hit_position,12);combat_shared->target_kind=combat_target_kind;
 combat_shared->callback_seen=combat_callback_seen;combat_shared->callback_owned=combat_callback_owned;combat_shared->callback_thread_changed=combat_callback_thread_changed;combat_shared->last_native_flags=combat_last_native_flags;combat_shared->last_message=combat_last_message;combat_shared->callback_foreign=combat_callback_foreign;
 combat_shared->boxes_broken=combat_boxes_broken;
 combat_shared->engine_ready=combat_engine_ready;combat_shared->queued=combat_queued;combat_shared->expired=combat_expired;combat_shared->hit_part=combat_last_part;combat_shared->hook_reason=combat_hook_reason;combat_shared->pending_id=combat_pending.active?combat_pending.id:0;
 combat_shared->observed_stamp=now;combat_shared->observed_generation=combat_generation;combat_shared->ack=combat_ack;combat_shared->reason=combat_reason;
 combat_shared->target_guid=combat_target_guid;combat_shared->target_slot=combat_target_slot;combat_shared->hp_before=combat_hp_before;combat_shared->hp_after=combat_hp_after;
 combat_shared->hits=combat_hits;combat_shared->kills=combat_kills;combat_shared->attempts=combat_attempts;combat_shared->active=active;combat_shared->count=active?combat_count:0;
 if(active)memcpy(combat_shared->actors,combat_catalog,combat_count*sizeof(CombatActor));
 MemoryBarrier();InterlockedIncrement(&combat_shared->sequence);
}
static int combat_collect(const float *feet,float scale){
 combat_count=0;combat_manager=enemy_select_manager();if(!combat_manager || !readable((void*)(uintptr_t)combat_manager,16))return 0;
 memcpy(combat_pool,(void*)(uintptr_t)combat_manager,16);
 if(!re4_cem_pool_header_valid(combat_pool,(uintptr_t)image_base,image_size) || !readable_span((void*)(uintptr_t)combat_pool[1],(size_t)combat_pool[2]*combat_pool[3]))return 0;
 for(uint32_t i=0;i<combat_pool[2];i++){
  unsigned char *p=(unsigned char*)(uintptr_t)(combat_pool[1]+i*combat_pool[3]);uint32_t vt,flags,guid;int16_t hp,max;float pos[3];
  memcpy(&vt,p,4);memcpy(&flags,p+4,4);memcpy(&guid,p+0xF8,4);memcpy(&hp,p+0x324,2);memcpy(&max,p+0x326,2);memcpy(pos,p+0x94,12);
  if((flags&0x601u)!=1 || !combat_kind(p[0x100]) || (p[0xFC]!=1 && p[0xFC]!=2) || hp<=0 || max<hp
   || vt<(uintptr_t)image_base || vt>=(uintptr_t)image_base+image_size)continue;
  float distance=0;unsigned valid=1;for(unsigned a=0;a<3;a++){if(!isfinite(pos[a]) || fabsf(pos[a])>20000000)valid=0;float d=pos[a]-feet[a];distance+=d*d;}
  if(!valid || distance>(64*scale+2000)*(64*scale+2000))continue;
  if(combat_count==COMBAT_MAX_ACTORS)return 0; // never publish an incomplete nearby target set
  if(p[0x100]==0x2A && (p[0xFC]!=1 || ((p[0x101]==1 || p[0x101]==2)?p[0xFD]!=6:(p[0xFD]!=0 && p[0xFD]!=5))))continue;
  CombatActor *out=&combat_catalog[combat_count++];*out=(CombatActor){.slot=i,.guid=guid,.kind=p[0x100],.hp=(uint32_t)hp,.max=(uint32_t)max,.radius=p[0x100]==0x2A?1000:p[0x100]>=0x50?700: p[0x100]==0x23?350:250,.height=p[0x100]==0x23?600:1800};memcpy(out->position,pos,12);
 }
 return !memcmp(combat_pool,(void*)(uintptr_t)combat_manager,16);
}
static int combat_blocks_clear(const CombatShared *r,float hit){
 if(!TryAcquireSRWLockExclusive(&collision_tick_lock))return 0;
 int clear=0;CubeCommand *c=&collision_command;
 if(!cube_collision_command_read(c))goto done;
 if(!c->enabled){clear=1;goto done;}
 if(c->mode!=2 || c->arm_generation!=r->generation || GetTickCount()-c->stamp>=500 || c->count>CUBE_MAX_BOXES || block_collision_count(c)>c->count)goto done;
 if(block_collision_count(c) && !block_collision_scene_valid(c))goto done;
 {float start[3],end[3];block_local_point(c->origin,r->start,start);block_local_point(c->origin,r->end,end);
  for(unsigned i=0;i<block_collision_count(c);i++){float t;if(combat_ray_box(start,end,c->boxes[i],&t) && t<=hit+.00001f)goto done;}}
 clear=1;
 done:ReleaseSRWLockExclusive(&collision_tick_lock);return clear;
}
static void combat_tick(const ControlFrame *control,int complete,uint32_t now){
 if(!combat_shared || !TryAcquireSRWLockExclusive(&combat_lock))return;
 CombatShared r={0};int stable=0;unsigned active=0;
 if(complete){combat_observe_box(now);combat_observe_hazard(now);}
 if((combat_pending.active==1 || combat_pending.active==3) && (now-combat_pending.stamp>=500 || !complete || !combat_session_valid(combat_pending.generation,now)))combat_cancel_pending();
 if(!combat_engine_ready){combat_reason=6;goto done;}
 for(unsigned i=0;i<3;i++){LONG before=combat_shared->request_sequence;if(before&1)continue;MemoryBarrier();memcpy((char*)&r+20,(const char*)combat_shared+20,60);memcpy(&r.kind,&combat_shared->kind,8);MemoryBarrier();if(before==combat_shared->request_sequence){stable=1;break;}}
 if(!stable || !complete || !state->armed || !control->armed || state->stop || (control->flags&11u)!=3u || control->generation!=state->arm_id
  || !r.enabled || r.enabled!=1 || r.generation!=control->generation || now-control->stamp>=500 || now-r.stamp>=500 || runtime_scripted() || (GetAsyncKeyState(VK_F8)&0x8000))goto done;
 if(!isfinite(r.scale) || r.scale<40 || r.scale>2500)goto done;
 uint32_t player=current_player();float feet[3];if(!player || !readable((void*)(uintptr_t)(player+0x94),12))goto done;memcpy(feet,(void*)(uintptr_t)(player+0x94),12);
 for(unsigned i=0;i<3;i++)if(!isfinite(feet[i]) || fabsf(feet[i])>20000000)goto done;
 if(combat_generation!=control->generation){combat_cancel_pending();combat_generation=control->generation;combat_consumed=combat_ack=0;combat_projectile_high=combat_projectile_count=0;combat_reason=1;combat_hp_before=combat_hp_after=0;memset(combat_hurt,0,sizeof(combat_hurt));}
 if(!combat_collect(feet,r.scale))goto done;
 active=1;
 if(!r.id || r.id>2147483647u || r.id<=combat_consumed)goto done;
 if(combat_pending.active)goto done;
 uint32_t previous_ack=combat_ack;combat_consumed=r.id;combat_ack=r.id;combat_attempts++;combat_hp_before=combat_hp_after=0;combat_target_guid=combat_target_slot=0;combat_reason=4;
 if(!combat_request_valid(&r))goto done;
 float origin_distance=0;for(unsigned i=0;i<3;i++){float d=r.start[i]-feet[i]-(i==1?control->eye:0);origin_distance+=d*d;}
 if(!isfinite(control->eye) || control->eye<40 || control->eye>2500 || origin_distance>(r.kind?64*64:1)*r.scale*r.scale)goto done;
 if(r.kind){
  if(r.projectile<=combat_projectile_high)goto done;
  for(unsigned i=0;i<combat_projectile_count;i++)if(combat_projectile_ids[i]==r.projectile)goto done;
 }
 CombatActor *target=NULL;float nearest=2;
 CombatRayHit projectile_hit={0};
 if(r.kind){
  // The requested actor is a broad candidate, not a replacement for RE4's
  // nearest real hit. Large trap/crate boxes must not steal a shot from it.
  unsigned known=0;for(unsigned i=0;i<combat_count;i++)if(combat_catalog[i].slot==r.slot && combat_catalog[i].guid==r.guid)known=1;
  combat_reason=4;if(!known)goto done;
  projectile_hit=combat_trace(&r);
  for(unsigned i=0;i<combat_count;i++)if(projectile_hit.actor==(unsigned char*)(uintptr_t)(combat_pool[1]+combat_catalog[i].slot*combat_pool[3])){target=&combat_catalog[i];break;}
  combat_reason=2;if(!target)goto done;
  r.slot=target->slot;r.guid=target->guid;
  // Real-part occlusion is checked below; no approximate AABB veto here.
  nearest=0;
 }else{
  for(unsigned i=0;i<combat_count;i++){CombatActor *a=&combat_catalog[i];float b[6]={a->position[0]-a->radius,a->position[1]-(a->kind==0x2A?400:0),a->position[2]-a->radius,a->position[0]+a->radius,a->position[1]+a->height,a->position[2]+a->radius},t;if(combat_ray_box(r.start,r.end,b,&t) && t<nearest){target=a;nearest=t;}}
  combat_reason=2;if(!target || target->slot!=r.slot || target->guid!=r.guid)goto done;
 }
 combat_reason=3;if(!combat_blocks_clear(&r,nearest))goto done;
 unsigned char *p=(unsigned char*)(uintptr_t)(combat_pool[1]+target->slot*combat_pool[3]);uint32_t vt,flags,guid;int16_t hp,max;uint8_t routine[4];float pose[3];
 memcpy(&vt,p,4);memcpy(&flags,p+4,4);memcpy(&guid,p+0xF8,4);memcpy(&hp,p+0x324,2);memcpy(&max,p+0x326,2);memcpy(routine,p+0xFC,4);memcpy(pose,p+0x94,12);
 combat_reason=4;
 if(vt<(uintptr_t)image_base || vt>=(uintptr_t)image_base+image_size || (flags&0x601u)!=1 || p[0x100]!=target->kind || (routine[0]!=1 && routine[0]!=2)
  || guid!=target->guid || hp!=(int16_t)target->hp || max!=(int16_t)target->max || memcmp(pose,target->position,12))goto done;
 uint32_t damage=r.damage;CombatHurt *hurt=&combat_hurt[target->slot];
 if(hurt->valid && hurt->generation==r.generation && hurt->guid==guid && hurt->vtable==vt && now-hurt->stamp<500){
  combat_reason=5;if(damage<=hurt->damage)goto done;damage-=hurt->damage;
 }
 combat_reason=7;if(p[0x32C] || p[0x32D])goto done;
 combat_reason=2;unsigned char *part=r.kind?combat_validate_part(&r,p,projectile_hit.part):combat_hit_part(&r,p);if(!part)goto done;
 combat_reason=3;float hit_distance,length=0;memcpy(&hit_distance,part+0x28,4);for(unsigned i=0;i<3;i++){float d=r.end[i]-r.start[i];length+=d*d;}
 if(!combat_blocks_clear(&r,sqrtf(hit_distance/length)))goto done;
 combat_reason=4;
 if(state->stop || !state->armed || state->arm_id!=r.generation || GetTickCount()-control->stamp>=500 || GetTickCount()-r.stamp>=500 || runtime_scripted()
  || (GetAsyncKeyState(VK_F8)&0x8000) || *player_slot!=player || memcmp(feet,(void*)(uintptr_t)(player+0x94),12)
  || memcmp(combat_pool,(void*)(uintptr_t)combat_manager,16) || memcmp(&vt,p,4) || memcmp(&flags,p+4,4) || memcmp(&guid,p+0xF8,4)
  || memcmp(&hp,p+0x324,2) || memcmp(&max,p+0x326,2) || memcmp(routine,p+0xFC,4) || p[0x100]!=target->kind || memcmp(pose,p+0x94,12))goto done;
 if(r.kind){
  // A geometric miss performs no impact and may continue to a later segment.
  // Any queued/rejected impact still retires the projectile permanently.
  if(combat_projectile_count==128){
   uint32_t oldest=combat_projectile_ids[0];unsigned index=0;
   for(unsigned i=1;i<128;i++)if(combat_projectile_ids[i]<oldest){oldest=combat_projectile_ids[i];index=i;}
   combat_projectile_high=oldest;combat_projectile_ids[index]=r.projectile;
  }else combat_projectile_ids[combat_projectile_count++]=r.projectile;
 }
 if(!combat_queue_message(&r,p,part,damage,vt,now)){combat_reason=7;goto done;}
 /* Consumption and acknowledgement are separate: durability waits for native HP
    confirmation in ApplyDamage, never for merely submitting a message. */
 combat_ack=previous_ack;combat_reason=9;

 done:combat_publish(active,now);ReleaseSRWLockExclusive(&combat_lock);
}
static void combat_ipc_open(void){
 wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-Combat-%lu",(unsigned long)GetCurrentProcessId());
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CombatShared),name);if(!map)return;
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(map);return;}
 CombatShared *p=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));if(!p){CloseHandle(map);return;}
 memset(p,0,sizeof(*p));p->magic=COMBAT_MAGIC;p->version=4;p->build=457;p->pid=GetCurrentProcessId();p->reason=1;combat_shared=p;combat_mapping=map;
}

#include "combat_install.h"
