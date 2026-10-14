#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "runtime_policy.h"
typedef int32_t LONG;
typedef unsigned SRWLOCK;
#define SRWLOCK_INIT 0
static int TryAcquireSRWLockExclusive(SRWLOCK *lock){if(*lock)return 0;*lock=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){*lock=0;}
typedef struct {uint32_t armed,stop,arm_id;} Shared;
typedef struct {uint32_t armed,generation,stamp,flags;} ControlFrame;
static Shared storage={1,0,7},*state=&storage;
static unsigned char globals[0x6000];static uint32_t global,*global_slot=&global,now=1000,block_camera_epoch=1;
static uint64_t on=0x80,trigger=0x20;static volatile uint64_t *key_on=&on,*key_trigger=&trigger;
static volatile unsigned runtime_last_scene;
static unsigned queries,f8;
static void MemoryBarrier(void){}
static void InterlockedIncrement(volatile LONG *v){++*v;}
static int readable(const void *p,size_t n){queries++;return p && n;}
static uint32_t GetTickCount(void){return now;}
static int GetAsyncKeyState(int key){(void)key;return f8?0x8000:0;}
#define VK_F8 119
static int runtime_scripted(void);
/* ACTUAL_RUNTIME */
int main(void){
    RuntimeShared report={0};runtime_shared=&report;global=(uint32_t)(uintptr_t)globals;assert((uintptr_t)globals<=UINT32_MAX);
    globals[0x20]=3;uint32_t flags=50331920;memcpy(globals+0x501C,&flags,4); // user's normal widescreen gameplay: 0x03000110
    assert(!runtime_scripted() && !report.scene);unsigned prior=queries;
    LONG report_sequence=report.sequence;runtime_report_lock=1;runtime_publish(1,0);
    assert(report.sequence==report_sequence && report.scene==0);runtime_report_lock=0;
    for(int i=0;i<10000;i++){assert(!runtime_scripted());}assert(queries==prior); // no per-draw VirtualQuery scans
    flags=0x10000000|50331920;memcpy(globals+0x501C,&flags,4);block_camera_epoch++;
    assert(runtime_scripted() && report.scene==1 && report.flags==flags);
    assert(re4_scene_code(3,0x4000)==0); // action prompt alone is not a cinematic
    assert(re4_scene_code(4,0)==1); // native door/loading routine
    for(unsigned bit=0;bit<32;bit++)if(RE4_SCRIPTED_MASK&(1u<<bit))assert(re4_scene_code(3,50331920|(1u<<bit))==1);
    flags=0x100000;memcpy(globals+0x501C,&flags,4);block_camera_epoch++;
    assert(runtime_death());
    ControlFrame control={.armed=1,.generation=7,.stamp=1000};
    report.request_stamp=1000;report.action_id=1;report.generation=7;
    runtime_action_apply(&control,1,now);assert(runtime_actions==1 && (on&RE4_ACTION_BITS)==RE4_ACTION_BITS && (trigger&RE4_ACTION_BITS)==RE4_ACTION_BITS);
    runtime_action_restore();assert(on==0x80 && trigger==0x20); // only borrowed action bits restored before original input
    runtime_action_apply(&control,1,now);assert(runtime_actions==1); // same token is one pulse
    for(unsigned fail=0;fail<7;fail++){
        report.action_id=2;report.request_stamp=control.stamp=now=1000;report.generation=control.generation=7;control.flags=0;state->armed=1;state->stop=0;state->arm_id=7;report.request_sequence=0;f8=0;
        if(fail==0)state->stop=1;
        if(fail==1)f8=1;
        if(fail==2)control.flags=8;
        if(fail==3)report.generation=8;
        if(fail==4)now=1500;
        if(fail==5)report.request_sequence=1;
        if(fail==6)state->arm_id=8;
        runtime_action_apply(&control,1,now);assert(runtime_actions==1 && on==0x80 && trigger==0x20);
    }
    state->arm_id=7;state->stop=0;f8=0;control.flags=0;now=1000;report.request_sequence=2;report.generation=7;
    runtime_action_apply(&control,1,now);assert(runtime_actions==2);runtime_action_restore();
    flags=50331920;memcpy(globals+0x501C,&flags,4);block_camera_epoch++;
    control.flags=3;report.lift=1200;report.scale=1000;report.vertical_enabled=1;report.lift_token=1;report.request_stamp=control.stamp=now=1000;
    runtime_vertical_tick(&control,1,now);assert(runtime_lift==0);now=1025;runtime_vertical_tick(&control,1,now);assert(runtime_lift==600 && runtime_body_radius==300 && runtime_body_height==1800);
    now=1050;runtime_vertical_tick(&control,1,now);assert(runtime_lift==1200);
    control.flags=11;now=1060;runtime_vertical_tick(&control,1,now);assert(runtime_lift==1200);
    control.flags=3;f8=1;runtime_vertical_tick(&control,1,now);assert(runtime_lift==0);f8=0;
    for(unsigned failure=0;failure<8;failure++){
        report.lift=1200;report.scale=1000;report.vertical_enabled=1;report.lift_token=2;report.generation=7;report.request_stamp=control.stamp=now=1000;control.flags=3;control.generation=7;
        if(failure==0)report.scale=NAN;
        if(failure==1)report.lift=-1;
        if(failure==2)report.lift=128001;
        if(failure==3)report.vertical_enabled=2;
        if(failure==4)report.lift_token=0xFFFFFFFFu;
        if(failure==5)report.generation=8;
        if(failure==6)control.flags=0;
        if(failure==7)now=1500;
        runtime_vertical_tick(&control,1,now);assert(runtime_lift==0 && runtime_body_radius==250 && runtime_body_height==1600);
    }
    state->armed=0;queries=0;assert(!runtime_scripted() && !queries);
    flags=50331920;memcpy(globals+0x501C,&flags,4);now+=16;
    assert(runtime_refresh()==0 && report.scene==0 && report.flags==flags); // no stale cinematic report after F8
    state->stop=1;globals[0x20]=4;now+=16;assert(runtime_refresh()==1 && report.scene==1);
    globals[0x20]=3;now+=16;assert(runtime_refresh()==0 && report.scene==0); // fresh preparation sees the real scene
    puts("Runtime: native movie/event/death/door gates, bounded cache, one-shot actions and F8/TTL/menu/generation/torn guards passed");
}
