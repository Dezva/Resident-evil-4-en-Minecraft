#include "health_logic.h"
typedef struct {
    uint32_t magic,version,build,pid;
    volatile LONG request_sequence;
    uint32_t stamp,enabled,generation,mode;float ratio;uint32_t ack_damage,ack_healing;
    volatile LONG sequence;
    uint32_t observed_stamp,reason,active,observed_generation,player,hp,max,damage,healing,dead,writes,observed_mode;
} HealthShared;
_Static_assert(sizeof(HealthShared)==100,"Health IPC v1");
static HealthShared *health_shared;static HANDLE health_mapping;static HealthLedger health_ledger;
static void health_ipc_open(void){
    wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-Health-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(HealthShared),name);
    if(!mapping)return;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mapping);return;}
    HealthShared *p=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));
    if(!p){CloseHandle(mapping);return;}
    memset(p,0,sizeof(*p));p->magic=0x52453448;p->version=1;p->build=457;p->pid=GetCurrentProcessId();p->reason=1;
    health_mapping=mapping;health_shared=p;
}
/* Existing game-thread pad callback only. No external process memory writes,
   no HP-max changes, damage hooks, debug flags or resurrection. */
static void health_tick(const ControlFrame *control,int complete,uint32_t now){
    if(!health_shared)return;
    HealthShared request={0};int stable=0;
    for(unsigned i=0;i<3;i++){
        LONG before=health_shared->request_sequence;if(before&1)continue;
        MemoryBarrier();memcpy(&request,(const void*)health_shared,48);MemoryBarrier();
        if(before==health_shared->request_sequence){stable=1;break;}
    }
    unsigned reason=1,active=0,wrote=0;
    // Preserve a lethal native event even when its animation removes the player
    // before the external position reader can sample it. This is read-only and
    // restricted to the exact global/arm that was live on the previous callback.
    if(health_shared->active && health_ledger.initialized && complete && control->generation==health_ledger.generation
        && !state->stop && global_slot && *global_slot==health_ledger.global
        && readable((void*)(uintptr_t)(health_ledger.global+0x4FB4),2)){
        int16_t observed;memcpy(&observed,(void*)(uintptr_t)(health_ledger.global+0x4FB4),2);
        if(observed<=0 && !health_ledger.dead){health_ledger.damage+=(uint32_t)lroundf((float)health_ledger.hp/health_ledger.max*RE4_HEALTH_UNIT);health_ledger.hp=0;health_ledger.dead=1;}
    }
    if(!stable){reason=2;goto publish;}
    if(runtime_scripted() || !complete || !control->armed || !state->armed || state->stop || !request.enabled
        || request.generation!=control->generation || (uint32_t)(now-request.stamp)>=500
        || (uint32_t)(now-control->stamp)>=500 || (GetAsyncKeyState(VK_F8)&0x8000))goto publish;
    uint32_t player=current_player(),global=global_slot?*global_slot:0;
    if(!player || global<0x10000 || global>0xFFFF7000 || (global&3u)
        || !readable((void*)(uintptr_t)(global+0x20),1)
        || *(uint8_t*)(uintptr_t)(global+0x20)!=3
        || !readable((void*)(uintptr_t)(global+0x4FC8),1)
        || (*(uint8_t*)(uintptr_t)(global+0x4FC8)!=0 && *(uint8_t*)(uintptr_t)(global+0x4FC8)!=6)
        || !writable((void*)(uintptr_t)(global+0x4FB4),4)
        || !writable((void*)(uintptr_t)(player+0x324),2)
        || !writable((void*)(uintptr_t)(player+0xFC),4)){reason=3;goto publish;}
    int16_t hp,max;memcpy(&hp,(void*)(uintptr_t)(global+0x4FB4),2);memcpy(&max,(void*)(uintptr_t)(global+0x4FB6),2);
    HealthLedger next=health_ledger;int desired;
    if(!re4_health_step(&next,control->generation,player,global,hp<0?0:hp,max,request.mode,request.ratio,request.ack_damage,request.ack_healing,&desired)){reason=4;goto publish;}
    if((uint32_t)(GetTickCount()-control->stamp)>=500 || *player_slot!=player || *global_slot!=global || state->stop || !state->armed
        || (GetAsyncKeyState(VK_F8)&0x8000)
        || memcmp(&hp,(void*)(uintptr_t)(global+0x4FB4),2) || memcmp(&max,(void*)(uintptr_t)(global+0x4FB6),2)){reason=5;goto publish;}
    if(hp!=desired){int16_t value=(int16_t)desired;memcpy((void*)(uintptr_t)(global+0x4FB4),&value,2);wrote=1;}
    // SDK cPlayer inherits cEm::hp_324. Keep its current HP mirror consistent;
    // max HP remains owned by RE4. cModel routine bytes are validated separately.
    {int16_t value=(int16_t)desired;memcpy((void*)(uintptr_t)(player+0x324),&value,2);}
    if(request.mode==1 && request.ratio==0 && desired==0){
        uint8_t routine=*(uint8_t*)(uintptr_t)(player+0xFC);
        // Only a live Move/Damage routine may enter cEm::Routine0::Die(3).
        // Native lethal hits retain their own death animation/subroutine.
        if(routine==1 || routine==2){const uint8_t die[4]={3,0,0,0};memcpy((void*)(uintptr_t)(player+0xFC),die,4);}
    }
    health_ledger=next;reason=0;active=1;
publish:
    InterlockedIncrement(&health_shared->sequence);MemoryBarrier();
    health_shared->observed_stamp=now;health_shared->reason=reason;health_shared->active=active;
    health_shared->observed_generation=health_ledger.generation;health_shared->player=health_ledger.player;
    health_shared->hp=(uint32_t)health_ledger.hp;health_shared->max=(uint32_t)health_ledger.max;
    health_shared->damage=health_ledger.damage;health_shared->healing=health_ledger.healing;
    health_shared->observed_mode=health_ledger.mode;
    health_shared->dead=(uint32_t)health_ledger.dead;health_shared->writes+=wrote;
    MemoryBarrier();InterlockedIncrement(&health_shared->sequence);
}
