/* Bounded humanoid enemy sweep; no AI/routines, Y, velocity, HP or animation writes. */
#include "runtime_discovery.h"
typedef struct {unsigned valid;uint32_t actor,vtable,guid,stamp;float position[3];} EnemyReference;
static EnemyReference enemy_references[4096];
static uint32_t enemy_array,enemy_generation;static float enemy_origin[4];
static uint32_t enemy_candidates[64];static unsigned enemy_candidate_count;
static uint32_t enemy_select_manager(void){
    if(!enemy_candidate_count)return enemy_manager;
    uint32_t selected=0;
    for(unsigned i=0;i<enemy_candidate_count;i++){
        uint32_t header[4],address=enemy_candidates[i];
        if(!readable((void*)(uintptr_t)address,16))continue;
        memcpy(header,(void*)(uintptr_t)address,16);
        if(!re4_cem_pool_header_valid(header,(uintptr_t)image_base,image_size)
            || !readable_span((void*)(uintptr_t)header[1],(size_t)header[2]*header[3]))continue;
        if(selected && selected!=address)return 0; // two cEm-compatible pools remain ambiguous
        selected=address;
    }
    return selected;
}
static int enemy_kind(unsigned id){
    return (id>=6 && id<=11) || id==13 || id==0x11 || id==0x12 || (id>=0x15 && id<=0x17)
        || id==0x19 || id==0x1a || id==0x1f || id==0x20 || id==0x42 || id==0x43 || id==0x44;
}
static void enemy_collision_tick(const ControlFrame *control,int complete,uint32_t now){
    if(!complete || !control->armed || !state->armed || state->stop || now-control->stamp>=500
        || runtime_scripted() || (GetAsyncKeyState(VK_F8)&0x8000)){
        runtime_enemy_reason=enemy_manager?1:6;enemy_array=0;memset(enemy_references,0,sizeof(enemy_references));return;
    }
    if(!TryAcquireSRWLockExclusive(&collision_tick_lock))return;
    uint32_t selected=enemy_select_manager();
    if(selected!=enemy_manager){enemy_array=0;memset(enemy_references,0,sizeof(enemy_references));}
    enemy_manager=selected;runtime_enemy_available=selected!=0;
    if(!selected){runtime_enemy_reason=6;runtime_enemy_scanned=runtime_enemy_eligible=runtime_enemy_capacity=runtime_enemy_stride=0;goto done;}
    CubeCommand *command=&collision_command;
    runtime_enemy_reason=2;runtime_enemy_scanned=runtime_enemy_eligible=0;
    if(!cube_collision_command_read(command))goto done;
    if(!command->enabled || !command->collisions || !command->epoch || now-command->stamp>=500
        || command->arm_generation!=control->generation || !block_collision_scene_valid(command)){
        enemy_array=0;memset(enemy_references,0,sizeof(enemy_references));goto done;
    }
    if(!cube_collision_graphics_ready() && !(control->flags&8u))goto done;
    uint32_t header[4];
    if(!readable((void*)(uintptr_t)enemy_manager,16)){runtime_enemy_available=0;runtime_enemy_reason=3;goto done;}
    memcpy(header,(void*)(uintptr_t)enemy_manager,16);
    uint32_t vtable=header[0],array=header[1],count=header[2],stride=header[3];
    runtime_enemy_capacity=count;runtime_enemy_stride=stride;runtime_enemy_reason=4;
    if(vtable<(uintptr_t)image_base || vtable>=(uintptr_t)image_base+image_size
        || count>4096 || stride<0x408 || stride>65536 || (stride&3u)
        || array<0x10000 || (array&3u) || array>UINT32_MAX-count*stride
        || (size_t)count*stride>64u*1024u*1024u){runtime_enemy_available=0;goto done;}
    if(count && !readable_span((void*)(uintptr_t)array,(size_t)count*stride)){runtime_enemy_available=0;runtime_enemy_reason=5;goto done;}
    runtime_enemy_available=1;runtime_enemy_reason=0;
    if(enemy_array!=array || enemy_generation!=control->generation || memcmp(enemy_origin,command->origin,16)){
        memset(enemy_references,0,sizeof(enemy_references));enemy_array=array;enemy_generation=control->generation;memcpy(enemy_origin,command->origin,16);
    }
    unsigned budget=0;
    for(uint32_t i=0;i<count;i++){
        uint32_t actor=array+i*stride;unsigned char *data=(unsigned char*)(uintptr_t)actor;runtime_enemy_scanned++;
        EnemyReference *ref=&enemy_references[i];uint32_t vt,flags,guid;int16_t hp;
        memcpy(&vt,data,4);memcpy(&flags,data+4,4);memcpy(&guid,data+0xF8,4);memcpy(&hp,data+0x324,2);
        if((flags&0x601u)!=1u || !enemy_kind(data[0x100]) || hp<=0 || (data[0xFC]!=1 && data[0xFC]!=2)
            || vt<(uintptr_t)image_base || vt>=(uintptr_t)image_base+image_size){ref->valid=0;continue;}
        runtime_enemy_eligible++;
        if(++budget>128){ref->valid=0;continue;}
        float observed[3],resolved[3];memcpy(observed,data+0x94,12);
        if(!isfinite(observed[0]+observed[1]+observed[2]) || fabsf(observed[0])>20000000 || fabsf(observed[1])>20000000 || fabsf(observed[2])>20000000){ref->valid=0;continue;}
        if(!ref->valid || ref->actor!=actor || ref->vtable!=vt || ref->guid!=guid || now-ref->stamp>=500){
            *ref=(EnemyReference){.valid=1,.actor=actor,.vtable=vt,.guid=guid,.stamp=now};memcpy(ref->position,observed,12);
        }
        float distance=0;for(unsigned a=0;a<3;a++){float d=observed[a]-ref->position[a];distance+=d*d;}
        float limit=500.f+6.f*(now-ref->stamp);
        if(distance>limit*limit)goto reference; // spawn/teleport: never sweep an unrelated pose back
        {
            int hit=block_resolve_horizontal_actor_validated(command,ref->position,observed,resolved,250,1600);
            if(hit>=0)hit+=block_depenetrate_horizontal(command,resolved,250,1600);
            if(hit<0){ref->valid=0;continue;}
            if(hit){
                if(state->stop || !state->armed || now-control->stamp>=500 || (GetAsyncKeyState(VK_F8)&0x8000)
                    || !writable(data+0x94,12) || memcmp(header,(void*)(uintptr_t)enemy_manager,16)
                    || memcmp(&vt,data,4) || memcmp(&guid,data+0xF8,4) || memcmp(&flags,data+4,4)
                    || memcmp(&hp,data+0x324,2) || (data[0xFC]!=1 && data[0xFC]!=2)
                    || memcmp(observed,data+0x94,12) || GetTickCount()-control->stamp>=500
                    || state->stop || !state->armed || (GetAsyncKeyState(VK_F8)&0x8000)){ref->valid=0;continue;}
                memcpy(data+0x94,resolved,4);memcpy(data+0x9C,resolved+2,4);observed[0]=resolved[0];observed[2]=resolved[2];
                runtime_enemy_contacts+=(uint32_t)hit;runtime_enemy_corrections++;
            }
        }
reference:
        *ref=(EnemyReference){.valid=1,.actor=actor,.vtable=vt,.guid=guid,.stamp=now};memcpy(ref->position,observed,12);
    }
done:
    ReleaseSRWLockExclusive(&collision_tick_lock);
}
