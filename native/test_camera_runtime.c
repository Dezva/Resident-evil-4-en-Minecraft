#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "look_logic.h"
#include "control_logic.h"
#define __cdecl
#define __attribute__(x)
typedef int32_t LONG;
typedef struct {uint32_t magic,version;LONG sequence;uint32_t stamp,armed,buttons;LONG stop,status,hook_ticks;uint32_t native_buttons;float look_total,pitch,eye_height;uint32_t look_flags,arm_id,capabilities;float applied_yaw;LONG camera_ticks,look_applied_count,camera_active;} Shared;
typedef struct {uint32_t magic,version;LONG sequence;uint32_t build,pid,calls,writes,rejected,stamp,pad_ticks,camera_ticks,applied;float yaw_before,yaw_written,yaw_after,view_yaw;} CameraDiagnostics;
typedef struct {uint32_t sequence,stamp,armed,buttons,flags,generation;float total,pitch,eye;} ControlFrame;
typedef struct {float position[3],target[3],roll,fovy;} CameraPoint;
typedef struct {uint32_t controller,object,vtable,draw_bits,slot;} HiddenWeapon;
static Shared storage,*state=&storage;static CameraDiagnostics diag,*diagnostics=&diag;
static uint32_t id,slot,global,last_player,last_arm_id,last_look_sequence,render_player,render_generation;
static volatile uint32_t *player_slot=&slot,*global_slot=&global;static volatile uint64_t keys,*key_on=&keys;
static unsigned char *camera_ctrl,*saved_global_camera,saved_global[0xF8],saved_ctrl[0xF8];
static int render_saved,relative_walk,have_cached_frame;static ControlFrame cached_frame,packet;
static float consumed_yaw,view_yaw,render_matrix[12],render_view[12],render_right[3],render_up[3],render_back[3];
static CameraPoint render_point;static void *eye_matrix;static float saved_eye_matrix[12];
static uint32_t hidden_player,hidden_vtable,saved_draw_bits;static HiddenWeapon hidden_weapons[2];
static uint32_t now=1000;static int f8;static unsigned native_camera_calls,projection_calls;
#define VK_F8 119
static uint32_t GetTickCount(void){return now;}
static int GetAsyncKeyState(int key){assert(key==VK_F8);return f8?0x8000:0;}
static LONG InterlockedIncrement(LONG *v){return ++*v;}
static LONG InterlockedExchange(LONG *v,LONG n){LONG old=*v;*v=n;return old;}
static LONG InterlockedCompareExchange(LONG *v,LONG n,LONG old){LONG value=*v;if(value==old)*v=n;return value;}
static void MemoryBarrier(void){}
static int writable(const void *p,size_t size){return p && size;}
static int readable(const void *p,size_t size){return p && size;}
static uint32_t current_player(void){return slot;}
static uint32_t weapon_controller(uint32_t player){(void)player;return 0;}
static void hide_weapons(uint32_t player){(void)player;}
static int read_control_frame(ControlFrame *out){*out=packet;return 1;}
static void cube_collision_tick(const ControlFrame *c,int complete,uint32_t stamp){(void)c;(void)complete;(void)stamp;}
static void original_camera(void *a,void *b,void *c,void *d,CameraPoint *e){(void)a;(void)b;(void)c;(void)d;(void)e;native_camera_calls++;}
static void original_projection(void){projection_calls++;}
static void original_pad(void){}
/* ACTUAL_CAMERA_RUNTIME */
int main(void){
    unsigned char *memory=mmap(NULL,65536,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(memory!=MAP_FAILED && (uintptr_t)memory<UINT32_MAX);
    global=(uint32_t)(uintptr_t)(memory+4096);camera_ctrl=memory+8192;slot=id=(uint32_t)(uintptr_t)(memory+24576);
    *(uint32_t *)(uintptr_t)id=0x1234;*(uint32_t *)(uintptr_t)(id+4)=0x12;
    float feet[3]={-103000,-400,1700};memcpy((void *)(uintptr_t)(id+0x94),feet,12);
    float yaw=.7f;memcpy((void *)(uintptr_t)(id+0xA4),&yaw,4);
    storage.armed=1;storage.capabilities=7;packet=(ControlFrame){.sequence=2,.stamp=1000,.armed=1,.flags=3,.generation=1,.eye=1600};
    pad_hook();assert(last_player==id && fabsf(view_yaw-.7f)<1e-5f);
    float offset[11]={0};offset[10]=60;CameraPoint point={0};float matrix[12]={0};
    camera_hook(camera_ctrl+0x278,NULL,matrix,offset,&point);assert(state->camera_active);
    camera_hook(camera_ctrl+0x500,NULL,matrix,offset,&point);assert(state->camera_active && native_camera_calls==1);
    projection_hook();assert(diagnostics->applied && fabsf(re4_wrap_yaw(diagnostics->yaw_after-view_yaw))<2e-5f);
    assert((*(uint32_t *)(uintptr_t)(id+4)&0x12)==0);
    // A pad callback after the camera update used to invalidate projection.
    const unsigned moves[]={4,8,1,2,5,9,15,0,0};
    for(unsigned repeat=0;repeat<30;repeat++)for(unsigned i=0;i<sizeof(moves)/sizeof(moves[0]);i++){
        packet.sequence+=2;packet.buttons=moves[i];pad_hook();assert(!state->camera_active);
        projection_hook();assert(state->camera_active && diagnostics->applied);
        assert(fabsf(re4_wrap_yaw(diagnostics->yaw_after-.7f))<2e-5f);
        assert((*(uint32_t *)(uintptr_t)(id+4)&0x12)==0);
    }
    // Last-moment position changes must be visible in the final render view.
    feet[0]+=300;memcpy((void *)(uintptr_t)(id+0x94),feet,12);projection_hook();assert(render_point.position[0]==feet[0]);
    packet.pitch=40;projection_hook();assert(render_point.target[1]<render_point.position[1]);
    packet.sequence+=2;packet.total=120;pad_hook();projection_hook();
    assert(fabsf(re4_wrap_yaw(diagnostics->yaw_after-(.7f-2.0943951f)))<2e-5f);
    // A new global camera and a reused player address must keep valid ownership.
    global+=4096;*(uint32_t *)(uintptr_t)id=0x5678;*(uint32_t *)(uintptr_t)(id+4)=0x12;
    projection_hook();assert(diagnostics->applied && saved_global_camera==(unsigned char *)(uintptr_t)(global+0x74));
    assert(hidden_vtable==0x5678);
    // F8/expiry/new generation are authoritative, with model flags restored.
    f8=1;projection_hook();assert(!diagnostics->applied && !state->camera_active && (*(uint32_t *)(uintptr_t)(id+4)&0x12)==0x12);
    f8=0;state->stop=0;now=1500;projection_hook();assert(!diagnostics->applied);
    now=1000;packet.generation=2;projection_hook();assert(!diagnostics->applied);
    assert(projection_calls>270);munmap(memory,65536);puts("Camera callbacks: auxiliary/idle/alternating directions preserve gaze; stop restores");
}
