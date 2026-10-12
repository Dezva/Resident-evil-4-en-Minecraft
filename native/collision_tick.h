#include "block_collision.h"
/* Existing pad/camera/projection callbacks; one small geometry snapshot, no atlas copies. */
static uint32_t collision_player,collision_generation,collision_stamp;
static float collision_previous[3],collision_origin[4];
static SRWLOCK collision_tick_lock=SRWLOCK_INIT;
static CubeCommand collision_command; /* protected by collision_tick_lock; no 96KB stack frame */
static void cube_collision_tick(const ControlFrame *control,int complete,uint32_t now) {
    unsigned active=0,reason=1,contact=0,correction=0;
    uint32_t player=0;
    CubeCommand *command=&collision_command;
    if(!collision_shared || !TryAcquireSRWLockExclusive(&collision_tick_lock))return;
    if(!complete || !control->armed || !state->armed || state->stop || (uint32_t)(now-control->stamp)>=500 || (GetAsyncKeyState(VK_F8)&0x8000))goto disabled;
    // Torn publication / a skipped graphics pass must never replace the last safe feet.
    if(!cube_collision_command_read(command))goto transient;
    if(!command->enabled || !command->collisions || !command->epoch || (uint32_t)(now-command->stamp)>=500 || command->arm_generation!=control->generation)goto disabled;
    if(!cube_collision_graphics_ready())goto transient;
    if(!block_collision_scene_valid(command)){reason=3;goto disabled;}
    player=current_player();
    if(!player || !writable((void*)(uintptr_t)(player+0x94),12)){reason=3;goto disabled;}
    float observed[3],resolved[3];memcpy(observed,(void*)(uintptr_t)(player+0x94),12);
    for(unsigned i=0;i<3;++i)if(!isfinite(observed[i]) || fabsf(observed[i])>20000000.f){reason=3;goto disabled;}
    active=1;reason=0;
    if(collision_player!=player || collision_generation!=control->generation || memcmp(collision_origin,command->origin,16)){
        reason=2;goto reference;
    }
    if((uint32_t)(now-collision_stamp)>=500){reason=4;InterlockedCompareExchange(&state->stop,1,0);goto disabled;}
    float distance=0;for(unsigned i=0;i<3;++i){float d=observed[i]-collision_previous[i];distance+=d*d;}
    if(distance>500*500){reason=4;InterlockedCompareExchange(&state->stop,1,0);goto disabled;} // Teleports/area transitions: do not drag the player back.
    if(!distance)goto reference; // still refresh lease/position; no sweep when the engine did not move
    int hit=block_resolve_horizontal_validated(command,collision_previous,observed,resolved);
    if(hit<0){reason=3;goto disabled;}
    contact=(unsigned)hit;
    if(hit) {
        if(!isfinite(resolved[0]+resolved[2]) || fabsf(resolved[0])>20000000.f || fabsf(resolved[2])>20000000.f){reason=3;goto disabled;}
        if(*player_slot!=player || state->stop || !state->armed || (GetAsyncKeyState(VK_F8)&0x8000)
            || (uint32_t)(GetTickCount()-control->stamp)>=500
            || memcmp(observed,(void*)(uintptr_t)(player+0x94),12)){reason=5;goto disabled;}
        /* Never replace native Y, velocity, animation, inventory or collision flags. */
        memcpy((void*)(uintptr_t)(player+0x94),resolved,4);memcpy((void*)(uintptr_t)(player+0x9C),resolved+2,4);
        observed[0]=resolved[0];observed[2]=resolved[2];correction=1;
    }
reference:
    collision_player=player;collision_generation=control->generation;collision_stamp=now;
    memcpy(collision_previous,observed,12);memcpy(collision_origin,command->origin,16);goto publish;
disabled:
    active=0;collision_player=collision_generation=0;goto publish;
transient:
    active=0; // retain safe reference and its original age, never adopt a penetrating pose
publish:
    InterlockedIncrement(&collision_shared->sequence);MemoryBarrier();
    collision_shared->ticks++;collision_shared->contacts+=contact;collision_shared->corrections+=correction;
    if(reason>=3)collision_shared->rejections++;
    collision_shared->reason=reason;collision_shared->active=active;collision_shared->generation=complete?control->generation:0;
    MemoryBarrier();InterlockedIncrement(&collision_shared->sequence);
    ReleaseSRWLockExclusive(&collision_tick_lock);
}
