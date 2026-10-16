#ifndef RE4_COMBAT_POLICY_H
#define RE4_COMBAT_POLICY_H
#include <stdint.h>
#include <math.h>
#define COMBAT_MAX_ACTORS 128u
#define COMBAT_SIZE 5332u
#define COMBAT_MAGIC 0x52453446u
/* Native cEm +0x324/+0x326 and identity fields from pinned SDK. */
typedef struct {uint32_t slot,guid,kind,hp,max;float position[3],radius,height;} CombatActor;
typedef struct {
 uint32_t magic,version,build,pid;volatile LONG request_sequence;
 uint32_t stamp,enabled,generation,id,damage;float scale,reach;uint32_t slot,guid;float start[3],end[3];
 volatile LONG sequence;uint32_t observed_stamp,observed_generation,ack,reason,target_guid,target_slot,hp_before,hp_after,hits,kills,attempts,active,count;
 CombatActor actors[COMBAT_MAX_ACTORS];
 uint32_t engine_ready,queued,expired,hit_part,hook_reason,pending_id;
 float hit_position[3];uint32_t target_kind;
 uint32_t callback_seen,callback_owned,callback_thread_changed,last_native_flags,last_message,callback_foreign;
 uint32_t boxes_broken;
 /* Request extension, covered by request_sequence, never by report sequence. */
 uint32_t kind,projectile;
} CombatShared;
_Static_assert(sizeof(CombatActor)==40,"Combat actor");
_Static_assert(sizeof(CombatShared)==COMBAT_SIZE,"Combat v4");
static inline int combat_ray_box(const float *start,const float *end,const float *box,float *fraction){
 float low=0,high=1;
 for(unsigned i=0;i<3;i++){float d=end[i]-start[i];if(fabsf(d)<.000001f){if(start[i]<box[i] || start[i]>box[i+3])return 0;}else{float a=(box[i]-start[i])/d,b=(box[i+3]-start[i])/d;if(a>b){float tmp=a;a=b;b=tmp;}low=fmaxf(low,a);high=fminf(high,b);if(low>high)return 0;}}
 *fraction=low;return 1;
}
static inline int combat_request_valid(const CombatShared *r){
 if(r->kind>1 || r->projectile>2147483647u || (r->kind==0 && r->projectile) || (r->kind==1 && !r->projectile))return 0;
 if(!r->id || r->id>2147483647u || r->slot>4095 || !r->damage || r->damage>50000 || !isfinite(r->scale+r->reach) || r->scale<40 || r->scale>2500 || r->reach<=0 || r->reach>6)return 0;
 float distance=0;for(unsigned i=0;i<3;i++){if(!isfinite(r->start[i]+r->end[i]) || fabsf(r->start[i])>20000000 || fabsf(r->end[i])>20000000)return 0;float d=r->end[i]-r->start[i];distance+=d*d;}
 return distance>.000001f && distance<=(r->scale*r->reach+1)*(r->scale*r->reach+1);
}
#endif
