#include <assert.h>
#include <stdio.h>
#include "health_logic.h"
int main(void){
    HealthLedger s={0};int value;
    assert(re4_health_step(&s,1,4,8,500,1000,1,1,0,0,&value) && value==1000);
    // Ganado damage, delayed/repeated acknowledgements and multiple coalesced hits.
    assert(re4_health_step(&s,1,4,8,800,1000,1,1,0,0,&value) && value==800 && s.damage==200000);
    for(int i=0;i<60;i++)assert(re4_health_step(&s,1,4,8,value,1000,1,1,0,0,&value) && value==800 && s.damage==200000);
    assert(re4_health_step(&s,1,4,8,600,1000,1,.8f,200000,0,&value) && value==600 && s.damage==400000);
    assert(re4_health_step(&s,1,4,8,600,1000,1,.6f,400000,0,&value) && value==600);
    // Minecraft food regeneration -> Leon; native herb -> Minecraft counter.
    assert(re4_health_step(&s,1,4,8,600,1000,1,.7f,400000,0,&value) && value==700 && s.healing==0);
    assert(re4_health_step(&s,1,4,8,1000,1000,1,.7f,400000,0,&value) && value==1000 && s.healing==300000);
    assert(re4_health_step(&s,1,4,8,1000,1000,1,1,400000,300000,&value) && value==1000);
    // Creative resists ordinary HP damage; switching to survival preserves MC hearts.
    assert(re4_health_step(&s,1,4,8,800,1000,2,.8f,400000,300000,&value) && value==1000);
    assert(re4_health_step(&s,1,4,8,1000,1000,1,.8f,600000,300000,&value) && value==800);
    assert(!re4_health_step(&s,1,5,8,800,1000,1,.8f,600000,300000,&value));
    assert(!re4_health_step(&s,1,4,9,800,1000,1,.8f,600000,300000,&value));
    assert(!re4_health_step(&s,1,4,8,800,1000,1,NAN,600000,300000,&value));
    assert(!re4_health_step(&s,1,4,8,800,1000,1,.8f,600001,300000,&value));
    // Counter wrap, without replaying damage; zero hearts latches native zero.
    s.damage=UINT32_MAX-10;
    assert(re4_health_step(&s,1,4,8,700,1000,1,.8f,UINT32_MAX-10,300000,&value) && value==700);
    assert(s.damage==99989);
    assert(re4_health_step(&s,1,4,8,700,1000,1,0,s.damage,300000,&value) && value==0 && s.dead);
    assert(re4_health_step(&s,1,4,8,1000,1000,1,1,s.damage,300000,&value) && value==0);
    // New explicit arm can attach a reloaded actor; never resurrect native HP=0.
    assert(re4_health_step(&s,2,5,9,1000,1000,1,.5f,0,0,&value) && value==500);
    assert(re4_health_step(&s,2,5,9,0,1000,1,.5f,0,0,&value) && value==0 && s.dead && s.damage==500000);
    assert(re4_health_step(&s,3,5,9,0,1000,2,1,0,0,&value) && value==0);
    // Native max-HP upgrade must compare percentages, not count the added
    // capacity as an extra heal; 500/1000 and 600/1200 are both half health.
    HealthLedger upgraded={0};
    assert(re4_health_step(&upgraded,4,5,9,500,1000,1,.5f,0,0,&value) && value==500);
    assert(re4_health_step(&upgraded,4,5,9,600,1200,1,.5f,0,0,&value) && value==600 && !upgraded.damage && !upgraded.healing);
    puts("Shared HP: hits/heals, acknowledgements, creative/survival, wrap, death and actor identity passed");
}
