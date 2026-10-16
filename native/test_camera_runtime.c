#define _GNU_SOURCE
#ifdef _WIN32
#include <stdlib.h>
#else
#include <sys/mman.h>
#endif
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "look_logic.h"
#include "control_logic.h"
#ifndef _WIN32
#define __cdecl
#endif
#define __attribute__(x)
typedef int32_t LONG;
typedef struct {uint32_t magic,version;LONG sequence;uint32_t stamp,armed,buttons;LONG stop,status,hook_ticks;uint32_t native_buttons;float look_total,pitch,eye_height;uint32_t look_flags,arm_id,capabilities;float applied_yaw;LONG camera_ticks,look_applied_count,camera_active;} Shared;
typedef struct {uint32_t magic,version;LONG sequence;uint32_t build,pid,calls,writes,rejected,stamp,pad_ticks,camera_ticks,applied;float yaw_before,yaw_written,yaw_after,view_yaw;} CameraDiagnostics;
typedef struct {uint32_t sequence,stamp,armed,buttons,flags,generation;float total,pitch,eye;} ControlFrame;
typedef struct {float position[3],target[3],roll,fovy;} CameraPoint;
typedef struct {unsigned valid;ControlFrame control;uint32_t pad_ticks,camera_ticks,player,vtable,validated_at;float feet[3];} ProjectionCache;
static ProjectionCache projection_cache;
typedef struct {volatile LONG sequence;uint32_t valid,stamp,player,generation,epoch;float view[12],camera[12],right[3],up[3],back[3];CameraPoint point;} BlockCameraFrame;
static BlockCameraFrame block_camera_frame;
static uint32_t native_hud_system,native_hud_owner,native_hud_saved;
static unsigned block_camera_scene;static uint32_t block_camera_epoch;
static void block_camera_begin(void){block_camera_epoch++;block_camera_scene=1;}
static void block_camera_end(void){block_camera_scene=0;}

typedef struct {uint32_t controller,object,vtable,draw_bits,slot;} HiddenWeapon;
static Shared storage,*state=&storage;static CameraDiagnostics diag,*diagnostics=&diag;
static uint32_t id,slot,global,last_player,last_arm_id,last_look_sequence,render_player,render_generation;
static unsigned char *image_base=(unsigned char*)(uintptr_t)0x1000;static uint32_t image_size=0x10000;
static volatile uint32_t *player_slot=&slot,*global_slot=&global;static volatile uint64_t keys,*key_on=&keys;
static unsigned char *camera_ctrl,*saved_global_camera,saved_global[0xF8],saved_ctrl[0xF8];
static int render_saved,relative_walk,have_cached_frame;static ControlFrame cached_frame,packet;
static float consumed_yaw,view_yaw,render_matrix[12],render_view[12],render_right[3],render_up[3],render_back[3];
static CameraPoint render_point;static void *eye_matrix;static float saved_eye_matrix[12];
static uint32_t hidden_player,hidden_vtable,saved_draw_bits;static HiddenWeapon hidden_weapons[2];
static uint32_t now=1000;static int f8;static unsigned native_camera_calls,projection_calls;
static volatile unsigned runtime_last_scene;static unsigned runtime_cache_valid;static uint32_t runtime_cache_flags,runtime_native_flags2;
#include "runtime_policy.h"
static float runtime_view_y,runtime_physics_y,runtime_support_skin=.1f,runtime_body_radius=250,runtime_body_height=1600;
static uint32_t runtime_vertical_valid,runtime_lift_generation=1,runtime_request_stamp=1000,runtime_actor_active,runtime_vertical_writes,runtime_vertical_rebase;static float runtime_actor_y,runtime_vertical_scale=1000;
static void runtime_vertical_interpolate(uint32_t stamp){(void)stamp;}
static void runtime_vertical_tick(const ControlFrame *c,int complete,uint32_t stamp){(void)c;(void)complete;(void)stamp;}
static unsigned runtime_refresh(void){runtime_last_scene=re4_scene_action_code(3,runtime_cache_flags,runtime_native_flags2);return runtime_last_scene;}
static int runtime_scripted(void){return state->armed && !state->stop && runtime_refresh()!=0;}
static uint32_t runtime_pad_stamp;
static void runtime_action_restore(void){}
static void runtime_action_projection(void){}
static void enemy_collision_tick(const ControlFrame *c,int complete,uint32_t now){(void)c;(void)complete;(void)now;}
static void runtime_action_apply(const ControlFrame *c,int complete,uint32_t now){(void)c;(void)complete;(void)now;}
static void runtime_publish(unsigned scene,uint32_t flags){(void)scene;(void)flags;}
static unsigned memory_queries,actor_queries,control_reads,key_polls,collision_calls;
static unsigned late_projection_move;
#define VK_F8 119
static uint32_t GetTickCount(void){return now;}
static int GetAsyncKeyState(int key){assert(key==VK_F8);key_polls++;if(late_projection_move){*(float*)(uintptr_t)(id+0x94)+=60;late_projection_move=0;}return f8?0x8000:0;}
static LONG InterlockedIncrement(volatile LONG *v){return ++*v;}
static LONG InterlockedExchange(LONG *v,LONG n){LONG old=*v;*v=n;return old;}
static LONG InterlockedCompareExchange(LONG *v,LONG n,LONG old){LONG value=*v;if(value==old)*v=n;return value;}
static void MemoryBarrier(void){}
static int writable(const void *p,size_t size){memory_queries++;return p && size;}
static int readable(const void *p,size_t size){memory_queries++;return p && size;}
static uint32_t current_player(void){actor_queries++;return slot;}
static int read_control_frame(ControlFrame *out){control_reads++;*out=packet;return 1;}
typedef struct {unsigned held;} SRWLOCK;
#define SRWLOCK_INIT {0}
static int TryAcquireSRWLockExclusive(SRWLOCK *lock){if(lock->held)return 0;lock->held=1;return 1;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){assert(lock->held);lock->held=0;}
typedef struct {LONG sequence;uint32_t ticks,contacts,corrections,rejections,reason,active,generation;} CollisionTelemetry;
static CollisionTelemetry barrier_report,*collision_shared;
#include "cube_policy.h"
static CubeCommand barrier_command;
static int cube_collision_command_read(CubeCommand *out){memcpy(out,&barrier_command,3168);return 1;}
static int cube_collision_graphics_ready(void){return 1;}
#define cube_collision_tick cube_actual_collision_tick
#include "collision_tick.h"
#undef cube_collision_tick
static void combat_tick(const ControlFrame *c,int complete,uint32_t stamp){(void)c;(void)complete;(void)stamp;}
static void health_tick(const ControlFrame *c,int complete,uint32_t stamp){(void)c;(void)complete;(void)stamp;}
static void cube_collision_tick(const ControlFrame *c,int complete,uint32_t stamp){collision_calls++;cube_actual_collision_tick(c,complete,stamp);}
static void original_camera(void *a,void *b,void *c,void *d,CameraPoint *e){(void)a;(void)b;(void)c;(void)d;(void)e;native_camera_calls++;}
static unsigned clobber_projection;
static void original_projection(void){
    projection_calls++;
    if(clobber_projection){
        assert(state->camera_active && !memcmp((void*)(uintptr_t)(global+0x74),render_matrix,48));
        assert((*(uint32_t*)(uintptr_t)(id+4)&RE4_MODEL_DRAW_BITS)==0);
        for(unsigned i=0;i<2;i++)assert((*(uint32_t*)(uintptr_t)(hidden_weapons[i].object+4)&RE4_MODEL_DRAW_BITS)==0);
        *(float*)(uintptr_t)(global+0x74)=123;
        *(uint32_t*)(uintptr_t)(id+4)|=RE4_MODEL_DRAW_BITS;
        for(unsigned i=0;i<2;i++)*(uint32_t*)(uintptr_t)(hidden_weapons[i].object+4)|=RE4_MODEL_DRAW_BITS;
    }
}
static void original_pad(void){}
/* ACTUAL_CAMERA_RUNTIME */
static void project(void){storage.sequence=(LONG)packet.sequence;projection_hook();}
int main(void){
#ifdef _WIN32
    unsigned char *memory=calloc(1,65536); // fixture uses the project's 32-bit compiler
    assert(memory && (uintptr_t)memory<UINT32_MAX);
#else
    unsigned char *memory=mmap(NULL,65536,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(memory!=MAP_FAILED && (uintptr_t)memory<UINT32_MAX);
#endif
    global=(uint32_t)(uintptr_t)(memory+4096);camera_ctrl=memory+8192;slot=id=(uint32_t)(uintptr_t)(memory+24576);
    *(uint32_t *)(uintptr_t)id=0x1234;*(uint32_t *)(uintptr_t)(id+4)=0x12;
    float feet[3]={-103000,-400,1700};memcpy((void *)(uintptr_t)(id+0x94),feet,12);
    float yaw=.7f;memcpy((void *)(uintptr_t)(id+0xA4),&yaw,4);
    storage.arm_id=1;storage.armed=1;storage.capabilities=15;packet=(ControlFrame){.sequence=2,.stamp=1000,.armed=1,.flags=3,.generation=1,.eye=1600};
    runtime_cache_flags=50331920; // same flags as the failed preparation log; normal gameplay must take control
    pad_hook();assert(last_player==id && fabsf(view_yaw-.7f)<1e-5f);
    float offset[11]={0};offset[10]=60;CameraPoint point={0};float matrix[12]={0};
    camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);assert(state->camera_active);
    camera_hook(camera_ctrl+0x500,NULL,matrix,offset,&point);assert(state->camera_active && native_camera_calls==1);
    project();assert(diagnostics->applied && fabsf(re4_wrap_yaw(diagnostics->yaw_after-view_yaw))<2e-5f);
    assert((*(uint32_t *)(uintptr_t)(id+4)&0x12)==0);
    // Drawing must use the latched final view, never the global overwritten for auxiliary passes.
    float block_view[12],block_fov;uint32_t block_player;
    float expected[12];memcpy(expected,render_view,48);
    memset((void*)(uintptr_t)(global+0x74+0x30),0,48);
    assert(cube_camera(block_view,&block_fov,&block_player) && !memcmp(block_view,expected,48) && block_player==id);
    pad_hook(); // transient restore between the final view and overlay must not select the orbit camera
    assert(!state->camera_active && cube_camera(block_view,&block_fov,&block_player) && !memcmp(block_view,expected,48));
    packet.generation++;assert(!cube_camera(block_view,&block_fov,&block_player));packet.generation--;
    block_camera_frame.sequence++;assert(!cube_camera(block_view,&block_fov,&block_player));block_camera_frame.sequence++;
    now+=100;assert(!cube_camera(block_view,&block_fov,&block_player));now-=100;
    runtime_vertical_valid=1;runtime_lift_generation=1;runtime_request_stamp=now;runtime_physics_y=runtime_view_y=feet[1]+1200;pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();
    assert(fabsf(render_point.position[1]-(feet[1]+packet.eye+1200))<.01f);
    assert(*(float*)(uintptr_t)(id+0x98)==feet[1]+1200);
    *(float*)(uintptr_t)(id+0x98)=feet[1]-60;packet.sequence+=2;project();assert(render_point.position[1]==feet[1]+2800 && *(float*)(uintptr_t)(id+0x98)==feet[1]+1200);
    runtime_vertical_valid=0;*(float*)(uintptr_t)(id+0x98)=feet[1]; // real actor and camera share absolute feet despite native floor movement
    // Native movie owns camera/model/HUD while the logical bridge stays armed.
    packet.buttons=65;pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();
    assert(state->camera_active && state->native_buttons==65 && !state->stop); // preparation ACK and W/sprint remain possible
    runtime_cache_flags=50331920|0x10000000;float native_yaw=*(float*)(uintptr_t)(id+0xA4);
    pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();
    assert(!state->camera_active && state->armed && !state->stop && (*(uint32_t*)(uintptr_t)(id+4)&0x12)==0x12);
    assert(*(float*)(uintptr_t)(id+0xA4)==native_yaw && !state->native_buttons);
    runtime_cache_flags=50331920;packet.buttons=0;pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();assert(state->camera_active);
    runtime_native_flags2=0x40000000; // native player/enemy action restores Leon's body and camera
    pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();
    assert(!state->camera_active && state->armed && !state->stop && (*(uint32_t*)(uintptr_t)(id+4)&0x12)==0x12);
    runtime_native_flags2=0;pad_hook();camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);project();
    assert(state->camera_active && state->armed && !state->stop && (*(uint32_t*)(uintptr_t)(id+4)&0x12)==0);
    // Absolute mapped gaze cannot inherit an idle/tank body yaw or lose the first mouse delta.
    packet.flags=7;packet.total=135;packet.sequence+=2;packet.generation++;
    *(float*)(uintptr_t)(id+0xA4)=1.2f;pad_hook();assert(fabsf(view_yaw+2.35619449f)<1e-5f);
    packet.total=-172;packet.sequence+=2;packet.buttons=4;pad_hook();assert(fabsf(view_yaw-3.0019663f)<1e-5f);
    packet.buttons=0;packet.sequence+=2;pad_hook();assert(fabsf(view_yaw-3.0019663f)<1e-5f);
    packet.flags=3;packet.total=0;packet.generation++;packet.sequence+=2;*(float*)(uintptr_t)(id+0xA4)=.7f;pad_hook();
    // New input midway through one D3D scene must not move the blocks relative to already drawn geometry.
    block_camera_begin();project();memcpy(expected,render_view,48);
    packet.total=40;packet.sequence+=2;pad_hook();project();
    assert(!memcmp(expected,render_view,48));assert(cube_camera(block_view,&block_fov,&block_player) && !memcmp(expected,block_view,48));
    block_camera_end();block_camera_begin();project();assert(memcmp(expected,render_view,48));block_camera_end();
    packet.total=0;packet.generation++;packet.sequence+=2;*(float*)(uintptr_t)(id+0xA4)=.7f;pad_hook();
    // Existing cockpit ID primitives are hidden without changing gameplay/global flags.
    restore_first_person();native_hud_system=(uint32_t)(uintptr_t)(memory+45056);
    *(int32_t*)(uintptr_t)native_hud_system=4;
    uint8_t *units=memory+50000;*(uint32_t*)(uintptr_t)(native_hud_system+0x50)=(uint32_t)(uintptr_t)units;
    units[0]=0x19;units[1]=7;units[2]=0x21;
    units[0x144]=0x11;units[0x144+2]=0x32; // already hidden before FP
    units[0x288]=0x19;units[0x288+2]=0x20; // interaction prompt is untouched
    uint32_t *off=(uint32_t*)(uintptr_t)(native_hud_system+0x30);
    *off=0x40000000u|0x80u;native_hud_hide();
    assert((*off&RE4_COCKPIT_CLASSES)==RE4_COCKPIT_CLASSES);
    assert(units[0]==0x11 && units[0x144]==0x11 && units[0x288]==0x19);
    *off|=0x400u;native_hud_restore();assert(*off==(0x40000000u|0x480u));assert(units[0]==0x19 && units[0x144]==0x11);
    assert(*(uint32_t*)(uintptr_t)(global+0x58)==0);
    *off=0;pad_hook();project();assert((*off&RE4_COCKPIT_CLASSES)==RE4_COCKPIT_CLASSES);
    *off=0;project();assert((*off&RE4_COCKPIT_CLASSES)==RE4_COCKPIT_CLASSES); // engine overwrites during cached scene
    restore_first_person();assert(*off==0);
    *(int32_t*)(uintptr_t)native_hud_system=8193;native_hud_hide();assert(*off==0);
    *(int32_t*)(uintptr_t)native_hud_system=4;
    // Repeated projections in one update reuse view, while restoring game-overwritten matrices/models.
    uint32_t controller=(uint32_t)(uintptr_t)(memory+32768);
    *(uint32_t*)(uintptr_t)(id+0x7D8)=controller;
    for(unsigned i=0;i<2;i++){
        uint32_t model=(uint32_t)(uintptr_t)(memory+36864+i*4096);
        *(uint32_t*)(uintptr_t)(controller+0x34+i*4)=model;
        *(uint32_t*)(uintptr_t)model=0x2345;*(uint32_t*)(uintptr_t)(model+4)=0x12;
    }
    pad_hook();project();assert(projection_cache.valid && hidden_weapons[0].object && hidden_weapons[1].object);
    unsigned cached_controls=control_reads,cached_actors=actor_queries,cached_collision=collision_calls,cached_diag=diag.calls,cached_queries=memory_queries;
    clobber_projection=1;for(unsigned i=0;i<10000;i++)project();clobber_projection=0;
    assert(control_reads==cached_controls && actor_queries==cached_actors && collision_calls==cached_collision && diag.calls==cached_diag);
    assert(memory_queries==cached_queries);
    assert((*(uint32_t*)(uintptr_t)(hidden_weapons[0].object+4)&0x12)==0x12); // native function clobbered after the last reapplication
    project();assert((*(uint32_t*)(uintptr_t)(hidden_weapons[0].object+4)&0x12)==0);
    // FOV and new owner/input/native frame must invalidate cache immediately.
    float next_fov=65;memcpy((void*)(uintptr_t)(global+0x74+0xC0),&next_fov,4);
    project();assert(render_point.fovy==65 && diag.calls==cached_diag+1);
    late_projection_move=1;float before_late=render_point.position[0];project();
    assert(render_point.position[0]==before_late+60 && diag.calls==cached_diag+2); // never reuse after late movement
    unsigned before_age=diag.calls;now+=16;project();assert(diag.calls==before_age+1);now=1000;projection_cache.valid=0;
    // Actual collider + actual projection: repeated late engine steps must never reuse a penetrating view.
    float barrier_start=*(float*)(uintptr_t)(id+0x94);
    barrier_command=(CubeCommand){.enabled=1,.epoch=7,.stamp=now,.mode=2,.count=1,.collisions=1,.arm_generation=packet.generation};
    barrier_command.origin[0]=barrier_start+300;barrier_command.origin[1]=*(float*)(uintptr_t)(id+0x98);barrier_command.origin[2]=*(float*)(uintptr_t)(id+0x9C);
    float wall[6]={0,0,-500,1000,1000,500};memcpy(barrier_command.boxes[0],wall,sizeof(wall));collision_shared=&barrier_report;project();
    for(unsigned i=0;i<2000;i++){
        *(float*)(uintptr_t)(id+0x94)+=60;project();
        assert(*(float*)(uintptr_t)(id+0x94)<barrier_start+50 && render_point.position[0]==*(float*)(uintptr_t)(id+0x94));
    }
    assert(barrier_report.corrections>=2000 && !state->stop);collision_shared=NULL;
    // A pad callback after the camera update used to invalidate projection.
    const unsigned moves[]={4,8,1,2,5,9,15,0,0};
    for(unsigned repeat=0;repeat<30;repeat++)for(unsigned i=0;i<sizeof(moves)/sizeof(moves[0]);i++){
        packet.sequence+=2;packet.buttons=moves[i];pad_hook();assert(!state->camera_active);
        project();assert(state->camera_active && diagnostics->applied);
        assert(fabsf(re4_wrap_yaw(diagnostics->yaw_after-.7f))<2e-5f);
        assert((*(uint32_t *)(uintptr_t)(id+4)&0x12)==0);
    }
    // Last-moment position changes must be visible in the final render view.
    feet[0]+=300;memcpy((void *)(uintptr_t)(id+0x94),feet,12);project();assert(render_point.position[0]==feet[0]);
    packet.sequence+=2;packet.pitch=40;project();assert(render_point.target[1]<render_point.position[1]);
    packet.sequence+=2;packet.total=120;pad_hook();project();
    assert(fabsf(re4_wrap_yaw(diagnostics->yaw_after-(.7f-2.0943951f)))<2e-5f);
    // A new global camera and a reused player address must keep valid ownership.
    global+=4096;*(uint32_t *)(uintptr_t)id=0x5678;*(uint32_t *)(uintptr_t)(id+4)=0x12;
    project();assert(diagnostics->applied && saved_global_camera==(unsigned char *)(uintptr_t)(global+0x74));
    assert(hidden_vtable==0x5678);
    // F8/expiry/new generation are authoritative, with model flags restored.
    f8=1;project();assert(!diagnostics->applied && !state->camera_active && (*(uint32_t *)(uintptr_t)(id+4)&0x12)==0x12);
    f8=0;state->stop=0;now=1500;project();assert(!diagnostics->applied);
    now=1000;packet.sequence+=2;packet.generation=2;project();assert(!diagnostics->applied);
    // No Minecraft/host: high-frequency projection calls must do only native forwarding.
    storage.armed=0;packet.armed=0;project(); // allow one final restore/invalidation
    unsigned old_queries=memory_queries,old_actor=actor_queries,old_control=control_reads,old_keys=key_polls,old_collision=collision_calls,old_calls=diag.calls;
    for(unsigned i=0;i<10000;i++)project();
    assert(memory_queries==old_queries && actor_queries==old_actor && control_reads==old_control && key_polls==old_keys && collision_calls==old_collision && diag.calls==old_calls);
    pad_hook(); // release the earlier relative-walk heading once
    old_queries=memory_queries;old_actor=actor_queries;
    for(unsigned i=0;i<1000;i++){camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);pad_hook();}
    assert(memory_queries==old_queries && actor_queries==old_actor);
    // Stop while an eye/model is borrowed restores ownership once, then becomes dormant.
    storage.stop=0;storage.armed=packet.armed=1;packet.generation=1;pad_hook();project();assert(state->camera_active);
    storage.stop=1;project();assert(!diag.applied && !render_saved && !hidden_player && !state->camera_active);
    old_queries=memory_queries;for(unsigned i=0;i<1000;i++)project();assert(memory_queries==old_queries);
    assert(projection_calls>11000);
#ifdef _WIN32
    free(memory);
#else
    munmap(memory,65536);
#endif
    puts("Camera callbacks: active gaze/restoration; 10000 dormant projections do zero memory/key/control/collision work");
}
