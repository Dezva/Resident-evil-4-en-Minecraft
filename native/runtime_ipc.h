#include "runtime_policy.h"
typedef struct {
    uint32_t magic,version,build,pid;volatile LONG sequence;
    uint32_t stamp,scene,flags,capabilities,enemy_contacts,enemy_corrections,actions;
    volatile LONG request_sequence;uint32_t request_stamp,action_id,generation;
    float lift;uint32_t lift_token;float scale;uint32_t vertical_enabled;
} RuntimeShared;
_Static_assert(sizeof(RuntimeShared)==80,"Runtime IPC v2");
static RuntimeShared *runtime_shared;static HANDLE runtime_mapping;
static SRWLOCK runtime_report_lock=SRWLOCK_INIT;
static uint32_t enemy_manager,runtime_enemy_available,runtime_enemy_contacts,runtime_enemy_corrections;
static uint32_t runtime_actions,action_generation,action_consumed;
static uint64_t action_saved_on,action_saved_trigger;static unsigned action_injected;
static unsigned runtime_cache_valid;static uint32_t runtime_cache_stamp,runtime_cache_epoch,runtime_cache_generation,runtime_cache_flags;
static float runtime_body_radius=250,runtime_body_height=1600;
static float runtime_lift,runtime_lift_from,runtime_lift_target;static uint32_t runtime_lift_generation,runtime_lift_token,runtime_lift_stamp;
static void runtime_vertical_tick(const ControlFrame *control,int complete,uint32_t now){
    RuntimeShared request={0};int stable=0;
    if(runtime_shared)for(unsigned i=0;i<3;i++){LONG before=runtime_shared->request_sequence;if(before&1)continue;MemoryBarrier();memcpy((char*)&request+48,(const char*)runtime_shared+48,32);MemoryBarrier();if(before==runtime_shared->request_sequence){stable=1;break;}}
    if(!stable || !complete || !state->armed || !control->armed || state->stop || runtime_scripted()
        || (control->flags&3u)!=3u || control->generation!=state->arm_id || request.generation!=control->generation || request.vertical_enabled!=1u
        || now-control->stamp>=500 || now-request.request_stamp>=500 || (GetAsyncKeyState(VK_F8)&0x8000)
        || !request.lift_token || request.lift_token>2147483647u || !isfinite(request.lift+request.scale) || request.scale<40 || request.scale>2500 || request.lift<0 || request.lift>128*request.scale){
        runtime_lift=runtime_lift_from=runtime_lift_target=0;runtime_lift_generation=runtime_lift_token=0;runtime_body_radius=250;runtime_body_height=1600;return;
    }
    runtime_body_radius=fmaxf(100,fminf(1000,.3f*request.scale));runtime_body_height=fmaxf(300,fminf(3000,1.8f*request.scale));
    if(runtime_lift_generation!=control->generation){runtime_lift_generation=control->generation;runtime_lift_token=0;runtime_lift=0;}
    if(control->flags&8u){runtime_lift_from=runtime_lift_target=runtime_lift;runtime_lift_stamp=now;return;}
    if(runtime_lift_token!=request.lift_token){runtime_lift_from=runtime_lift;runtime_lift_target=request.lift;runtime_lift_token=request.lift_token;runtime_lift_stamp=now;}
    float t=fminf(1.f,(now-runtime_lift_stamp)/50.f);runtime_lift=runtime_lift_from+(runtime_lift_target-runtime_lift_from)*t;
}
static unsigned runtime_scene_code(uint32_t *flags){
    *flags=0;
    if(!global_slot || !readable((void*)global_slot,4))return 2;
    uint32_t global=*global_slot;
    if(global<0x10000 || global>0xFFFF7000 || (global&3u)
        || !readable((void*)(uintptr_t)(global+0x20),1)
        || !readable((void*)(uintptr_t)(global+0x501C),4))return 2;
    memcpy(flags,(void*)(uintptr_t)(global+0x501C),4);
    return re4_scene_code(*(uint8_t*)(uintptr_t)(global+0x20),*flags);
}
static void runtime_publish(unsigned scene,uint32_t flags){
    if(!runtime_shared || !TryAcquireSRWLockExclusive(&runtime_report_lock))return;
    InterlockedIncrement(&runtime_shared->sequence);MemoryBarrier();
    runtime_shared->stamp=GetTickCount();runtime_shared->scene=scene;runtime_shared->flags=flags;
    runtime_shared->capabilities=runtime_enemy_available|(key_trigger?2u:0u);runtime_shared->enemy_contacts=runtime_enemy_contacts;
    runtime_shared->enemy_corrections=runtime_enemy_corrections;runtime_shared->actions=runtime_actions;
    MemoryBarrier();InterlockedIncrement(&runtime_shared->sequence);
    ReleaseSRWLockExclusive(&runtime_report_lock);
}
static unsigned runtime_refresh(void){
    if(!state)return 2;
    uint32_t now=GetTickCount();
    if(runtime_cache_valid && now-runtime_cache_stamp<8 && runtime_cache_epoch==block_camera_epoch && runtime_cache_generation==state->arm_id)return runtime_last_scene;
    uint32_t flags;unsigned scene=runtime_scene_code(&flags);runtime_last_scene=scene;
    runtime_cache_valid=1;runtime_cache_stamp=now;runtime_cache_epoch=block_camera_epoch;runtime_cache_generation=state->arm_id;runtime_cache_flags=flags;
    runtime_publish(scene,flags);return scene;
}
static int runtime_scripted(void){return state && state->armed && !state->stop && runtime_refresh()!=0;}
static int runtime_death(void){return runtime_scripted() && (runtime_cache_flags&0x00100000u);}
static void runtime_ipc_open(void){
    wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-Runtime-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(RuntimeShared),name);
    if(!mapping)return;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mapping);return;}
    RuntimeShared *p=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));
    if(!p){CloseHandle(mapping);return;}
    memset(p,0,sizeof(*p));p->magic=0x52453452;p->version=2;p->build=444;p->pid=GetCurrentProcessId();p->scene=2;
    runtime_shared=p;runtime_mapping=mapping;
}
static void runtime_action_restore(void){
    if(!action_injected)return;
    if(key_on)*key_on=(*key_on&~RE4_ACTION_BITS)|action_saved_on;
    if(key_trigger)*key_trigger=(*key_trigger&~RE4_ACTION_BITS)|action_saved_trigger;
    action_injected=0;
}
static void runtime_action_apply(const ControlFrame *control,int complete,uint32_t now){
    if(!runtime_shared || !key_on || !key_trigger)return;
    RuntimeShared request={0};int stable=0;
    for(unsigned i=0;i<3;i++){
        LONG before=runtime_shared->request_sequence;if(before&1)continue;
        MemoryBarrier();memcpy((char*)&request+48,(const char*)runtime_shared+48,16);MemoryBarrier();
        if(before==runtime_shared->request_sequence){stable=1;break;}
    }
    unsigned live=stable && complete && state->armed && control->armed && !state->stop && control->generation==state->arm_id && now-control->stamp<500
        && !(GetAsyncKeyState(VK_F8)&0x8000);
    if(!re4_action_pulse(now,request.request_stamp,control->generation,request.generation,request.action_id,
        &action_generation,&action_consumed,live,(control->flags&8u)!=0))return;
    action_saved_on=*key_on&RE4_ACTION_BITS;action_saved_trigger=*key_trigger&RE4_ACTION_BITS;action_injected=1;
    *key_on|=RE4_ACTION_BITS;*key_trigger|=RE4_ACTION_BITS;runtime_actions++;
}
