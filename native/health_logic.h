#ifndef RE4_HEALTH_LOGIC_H
#define RE4_HEALTH_LOGIC_H
#include <math.h>
#include <stdint.h>
#define RE4_HEALTH_UNIT 1000000u
/* Two cumulative counters avoid losing hits/heals when TCP coalesces frames.
   Minecraft acknowledges them only after its server applies the corresponding delta. */
typedef struct {uint32_t generation,player,global,damage,healing;int hp,max,dead,initialized;unsigned mode;} HealthLedger;
static inline int re4_health_step(HealthLedger *s,uint32_t generation,uint32_t player,uint32_t global,
        int hp,int max,unsigned mode,float ratio,uint32_t ack_damage,uint32_t ack_healing,int *desired){
    if(!generation || !player || !global || hp<0 || max<=0 || max>32767 || hp>max || (mode!=1 && mode!=2)
        || !isfinite(ratio) || ratio<0 || ratio>1)return 0;
    if(!s->initialized || generation!=s->generation){
        *s=(HealthLedger){.generation=generation,.player=player,.global=global,.hp=hp,.max=max,.initialized=1,.dead=hp==0};
        ack_damage=ack_healing=0;
    }else{
        if(player!=s->player || global!=s->global)return 0; // another actor/room requires explicit arm
        float before=(float)s->hp/s->max,after=(float)hp/max;
        if(after<before)s->damage+=(uint32_t)lroundf((before-after)*RE4_HEALTH_UNIT);
        if(after>before)s->healing+=(uint32_t)lroundf((after-before)*RE4_HEALTH_UNIT);
        if(hp==0)s->dead=1;
    }
    uint32_t pending_damage=s->damage-ack_damage,pending_healing=s->healing-ack_healing;
    if(pending_damage>100*RE4_HEALTH_UNIT || pending_healing>100*RE4_HEALTH_UNIT)return 0;
    float target=mode==2?1.f:ratio+((float)pending_healing-(float)pending_damage)/RE4_HEALTH_UNIT;
    if(target<0)target=0;
    if(target>1)target=1;
    if(mode==1 && (s->dead || ratio==0))target=0;
    // Never resurrect an actor whose native HP already reached zero, even in creative.
    if(s->dead)target=0;
    int value=(int)lroundf(target*max);
    if(target>0 && !value)value=1;
    s->hp=value;s->max=max;s->mode=mode;if(!value)s->dead=1;*desired=value;return 1;
}
#endif
