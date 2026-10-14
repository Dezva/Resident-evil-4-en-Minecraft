/* Independent RE4 UHD 1.1.0 input bridge; public pattern references in README.md. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "control_logic.h"
#include "branch_logic.h"
#include "look_logic.h"

typedef struct {
    uint32_t magic, version;
    volatile LONG sequence;
    volatile uint32_t stamp, armed, buttons;
    volatile LONG stop, status, hook_ticks;
    volatile uint32_t native_buttons;
    volatile float look_total, pitch, eye_height;
    volatile uint32_t look_flags, arm_id, capabilities;
    volatile float applied_yaw;
    volatile LONG camera_ticks, look_applied_count, camera_active;
} Shared;
_Static_assert(sizeof(Shared) == 80, "IPC v2 layout");
static Shared *state;
/* Retain the named mapping handle until process exit, independently of its view. */
static HANDLE shared_mapping;
/* Separate telemetry: it never extends or changes the 80-byte control protocol. */
typedef struct {
    uint32_t magic, version;
    volatile LONG sequence;
    uint32_t build, pid, calls, writes, rejected, stamp, pad_ticks, camera_ticks, applied;
    float yaw_before, yaw_written, yaw_after, view_yaw;
} CameraDiagnostics;
_Static_assert(sizeof(CameraDiagnostics) == 64, "Camera diagnostic layout");
static CameraDiagnostics *diagnostics;
static HANDLE diagnostic_mapping;
static HMODULE system_dinput;
static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
static void (__cdecl *original_pad)(void);
static volatile uint64_t *key_on,*key_trigger;
static volatile uint32_t *player_slot;
static unsigned char *image_base;
static DWORD image_size;
static unsigned char *camera_ctrl;
typedef struct { float position[3], target[3], roll, fovy; } CameraPoint;
static void (__attribute__((fastcall)) *original_camera)(void *, void *, void *, void *, CameraPoint *);
static uint32_t last_look_sequence, last_arm_id, last_player;
static float consumed_yaw;
static float view_yaw;
static int relative_walk;
static volatile uint32_t *global_slot;
static void (__cdecl *original_projection)(void);
static unsigned char *saved_global_camera;
static unsigned char saved_global[0xF8], saved_ctrl[0xF8];
static int render_saved;
static float render_matrix[12], render_view[12], render_right[3], render_up[3], render_back[3];
static CameraPoint render_point;
static uint32_t render_player, render_generation;
typedef struct {
    uint32_t sequence, stamp, armed, buttons, flags, generation;
    float total, pitch, eye;
} ControlFrame;
typedef struct {unsigned valid;ControlFrame control;uint32_t pad_ticks,camera_ticks,player,vtable,validated_at;float feet[3];} ProjectionCache;
static ProjectionCache projection_cache;
typedef struct { volatile LONG sequence;uint32_t valid,stamp,player,generation,epoch;float view[12],camera[12],right[3],up[3],back[3];CameraPoint point; } BlockCameraFrame;
static BlockCameraFrame block_camera_frame;
static uint32_t native_hud_system,native_hud_owner,native_hud_saved;
static unsigned block_camera_scene;
static uint32_t block_camera_epoch;
static volatile unsigned runtime_last_scene;
static void block_camera_begin(void){block_camera_epoch++;block_camera_scene=1;}
static void block_camera_end(void){block_camera_scene=0;}
static void cube_collision_tick(const ControlFrame *control,int complete,uint32_t now);
static void enemy_collision_tick(const ControlFrame *control,int complete,uint32_t now);
static ControlFrame cached_frame;
static int have_cached_frame;
static int read_control_frame(ControlFrame *out) {
    for (unsigned attempt=0;attempt<3;++attempt) {
        LONG before = state->sequence;
        MemoryBarrier();
        if (before & 1) continue;
        ControlFrame frame = {(uint32_t)before,state->stamp,state->armed,state->buttons,
            state->look_flags,state->arm_id,state->look_total,state->pitch,state->eye_height};
        MemoryBarrier();
        if (before == state->sequence) {
            cached_frame = frame; have_cached_frame = 1; *out = frame; return 1;
        }
    }
    if (re4_cached_frame_usable(have_cached_frame,GetTickCount(),cached_frame.stamp,
            cached_frame.armed,state->armed,(uint32_t)state->stop)) {
        *out = cached_frame; return 1;
    }
    return 0;
}
static void *eye_matrix;
static float saved_eye_matrix[12];
static uint32_t hidden_player, hidden_vtable, saved_draw_bits;
typedef struct { uint32_t controller, object, vtable, draw_bits, slot; } HiddenWeapon;
static HiddenWeapon hidden_weapons[2];
static unsigned char *smooth_continue __attribute__((used));
static int runtime_scripted(void);
/* Only integer reads; the mid-function hook must not disturb x87/SSE state. */
static unsigned __attribute__((cdecl,noinline,used)) camera_smoothing_flags(unsigned flags, void *self) {
    int active = !runtime_last_scene && state && state->camera_active && state->armed && !state->stop
        && self == camera_ctrl + 0x278;
    return re4_smoothing_flags(flags,active);
}
static void __attribute__((naked,used)) smoothing_hook(void) {
    __asm__ volatile (
        "cmpb $0, 0x1b6(%ebx)\n\t" /* Reproduce the original CMP and its flags. */
        "pushfl\n\tpushal\n\tmovl 32(%esp), %eax\n\t"
        "pushl %ebx\n\tpushl %eax\n\tcall _camera_smoothing_flags\n\taddl $8, %esp\n\t"
        "movl %eax, 32(%esp)\n\tpopal\n\tpopfl\n\tjmp *_smooth_continue\n\t");
}

static int writable(const void *pointer, SIZE_T length) {
    MEMORY_BASIC_INFORMATION m;
    return VirtualQuery(pointer, &m, sizeof(m)) && m.State == MEM_COMMIT
        && !(m.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        && (m.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))
        && (uintptr_t)pointer >= (uintptr_t)m.BaseAddress
        && length <= m.RegionSize - ((uintptr_t)pointer - (uintptr_t)m.BaseAddress);
}

/* A cManager pool can span several committed VirtualQuery regions. */
static int readable_span(const void *pointer,SIZE_T length){
    uintptr_t cursor=(uintptr_t)pointer;
    if(!cursor || !length || cursor>UINTPTR_MAX-length)return 0;
    uintptr_t end=cursor+length;
    for(unsigned i=0;i<64 && cursor<end;i++){
        MEMORY_BASIC_INFORMATION m;
        if(!VirtualQuery((void*)cursor,&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))
            || !(m.Protect&(PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return 0;
        uintptr_t base=(uintptr_t)m.BaseAddress;
        if(cursor<base || m.RegionSize>UINTPTR_MAX-base || base+m.RegionSize<=cursor)return 0;
        cursor=base+m.RegionSize;
    }
    return cursor>=end;
}
/* Only readable committed memory; never infer fallback addresses. */
static int readable(const void *pointer, SIZE_T length) {
    MEMORY_BASIC_INFORMATION m;
    if (!pointer || !VirtualQuery(pointer, &m, sizeof(m)) || m.State != MEM_COMMIT
            || (m.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    DWORD p = m.Protect & 0xff;
    if (p != PAGE_READONLY && p != PAGE_READWRITE && p != PAGE_WRITECOPY
            && p != PAGE_EXECUTE_READ && p != PAGE_EXECUTE_READWRITE && p != PAGE_EXECUTE_WRITECOPY) return 0;
    return (uintptr_t)pointer >= (uintptr_t)m.BaseAddress
        && length <= m.RegionSize - ((uintptr_t)pointer - (uintptr_t)m.BaseAddress);
}

static uint32_t current_player(void) {
    uint32_t player = player_slot ? *player_slot : 0;
    if (player < 0x10000 || player > 0xFFFFFF00 || (player & 3u)
            || !readable((void *)(uintptr_t)player, 0xAC)) return 0;
    uint32_t vtable; memcpy(&vtable, (void *)(uintptr_t)player, 4);
    if (vtable < (uintptr_t)image_base || vtable >= (uintptr_t)image_base + image_size) return 0;
    return player;
}

#include "runtime_ipc.h"

static uint32_t weapon_controller(uint32_t player) {
    if (!player || !readable((void *)(uintptr_t)player,0x7DC)) return 0;
    uint32_t controller; memcpy(&controller,(void *)(uintptr_t)(player+0x7D8),4);
    return controller >= 0x10000 && !(controller & 3u)
        && readable((void *)(uintptr_t)controller,0x50) ? controller : 0;
}
static void hide_weapons(uint32_t player) {
    uint32_t controller = weapon_controller(player);
    if (!controller) return;
    for (unsigned i=0;i<2;++i) {
        uint32_t object; memcpy(&object,(void *)(uintptr_t)(controller+0x34+4*i),4);
        if (object < 0x10000 || (object & 3u) || !re4_distinct_model(object,i ? hidden_weapons[0].object : 0)
                || !readable((void *)(uintptr_t)object,8) || !writable((void *)(uintptr_t)(object+4),4)) continue;
        uint32_t vtable = *(uint32_t *)(uintptr_t)object;
        if (vtable < (uintptr_t)image_base || vtable >= (uintptr_t)image_base+image_size) continue;
        uint32_t *draw = (uint32_t *)(uintptr_t)(object+4);
        if(hidden_weapons[i].controller!=controller || hidden_weapons[i].object!=object || hidden_weapons[i].vtable!=vtable)
            hidden_weapons[i] = (HiddenWeapon){controller,object,vtable,*draw & RE4_MODEL_DRAW_BITS,0x34+4*i};
        *draw = re4_hide_model(*draw);
    }
}

/* Restore before native logic. Only restore a model while it is still the same player. */
/* IDSystem display-off classes, pinned UHD SDK ID.h: +0x2c[8].
   Cockpit updates and already-created ID primitives have separate visibility:
   the old DPF_COCKPIT flag did not suppress those primitives. Hide only the
   life/ammo cockpit classes, preserving messages, prompts and inventory. */
#define RE4_COCKPIT_CLASSES (0x40000000u | 0x2000u | 0x800u) /* 0x21,0x32,0x34 */
typedef struct {uint32_t address;uint8_t identity[7],visible;} HudUnitLease;
static HudUnitLease native_hud_units[256];static unsigned native_hud_unit_count,native_hud_candidates;
static uint32_t native_hud_array,native_hud_capacity;
static int cockpit_class(uint8_t id){return id==0x21 || id==0x32 || id==0x34;}
static void native_hud_restore(void){
    if(native_hud_owner && native_hud_owner==native_hud_system
        && writable((void*)(uintptr_t)(native_hud_owner+0x30),4)){
        uint32_t *off=(uint32_t*)(uintptr_t)(native_hud_owner+0x30);
        *off=(*off&~RE4_COCKPIT_CLASSES)|native_hud_saved;
        uint32_t array=0,max=0;
        if(readable((void*)(uintptr_t)native_hud_owner,0x58)){
            memcpy(&array,(void*)(uintptr_t)(native_hud_owner+0x50),4);memcpy(&max,(void*)(uintptr_t)native_hud_owner,4);
        }
        if(array==native_hud_array && max==native_hud_capacity)for(unsigned i=0;i<native_hud_unit_count;i++){
            HudUnitLease *lease=&native_hud_units[i];uint8_t *unit=(uint8_t*)(uintptr_t)lease->address;
            if(writable(unit,8) && !memcmp(unit+1,lease->identity,7))unit[0]=(unit[0]&~8u)|lease->visible;
        }
    }
    native_hud_owner=0;native_hud_unit_count=0;native_hud_candidates=0;native_hud_array=0;native_hud_capacity=0;
}
static void native_hud_reapply(void){
    if(!native_hud_owner || native_hud_owner!=native_hud_system)return;
    *(uint32_t*)(uintptr_t)(native_hud_owner+0x30)|=RE4_COCKPIT_CLASSES;
    if(*(uint32_t*)(uintptr_t)(native_hud_owner+0x50)!=native_hud_array || *(uint32_t*)(uintptr_t)native_hud_owner!=native_hud_capacity){native_hud_unit_count=0;return;}
    for(unsigned i=0;i<native_hud_unit_count;i++){
        HudUnitLease *lease=&native_hud_units[i];uint8_t *unit=(uint8_t*)(uintptr_t)lease->address;
        if(!memcmp(unit+1,lease->identity,7))unit[0]&=~8u;
    }
}
static void native_hud_hide(void){
    uint32_t system=native_hud_system;
    if(!system || !readable((void*)(uintptr_t)system,0x58)
        || !writable((void*)(uintptr_t)(system+0x30),4))return;
    int32_t max,count;uint32_t array;memcpy(&max,(void*)(uintptr_t)system,4);memcpy(&count,(void*)(uintptr_t)(system+4),4);memcpy(&array,(void*)(uintptr_t)(system+0x50),4);
    if(max<=0 || max>8192 || count<0 || count>max)return;
    if(native_hud_owner!=system){native_hud_restore();native_hud_owner=system;native_hud_saved=*(uint32_t*)(uintptr_t)(system+0x30)&RE4_COCKPIT_CLASSES;}
    *(uint32_t*)(uintptr_t)(system+0x30)|=RE4_COCKPIT_CLASSES;
    if(native_hud_unit_count){native_hud_reapply();return;}
    if(array<0x10000 || (array&3u) || array>0xFFFFFFFFu-(uint32_t)max*0x144u
        || !writable((void*)(uintptr_t)array,(size_t)max*0x144u))return;
    // Existing ID primitives may ignore the class suppression in this UHD build.
    // Keep their actual visibility bit leased, never delete them or alter alpha/HP.
    native_hud_array=array;native_hud_capacity=(uint32_t)max;
    for(int32_t i=0;i<max;i++){
        uint8_t *unit=(uint8_t*)(uintptr_t)(array+(uint32_t)i*0x144u);
        if(!cockpit_class(unit[2]))continue;
        native_hud_candidates++;
        if(native_hud_unit_count==256)continue;
        HudUnitLease *lease=&native_hud_units[native_hud_unit_count++];lease->address=(uint32_t)(uintptr_t)unit;memcpy(lease->identity,unit+1,7);lease->visible=unit[0]&8u;
        unit[0]&=~8u;
    }
}
static void restore_rendered_camera(void) {
    /* This callback is on a shared projection thunk, including ordinary RE4 draws. */
    if(!render_saved && !eye_matrix && !hidden_player && !hidden_weapons[0].object && !hidden_weapons[1].object)return;
    projection_cache.valid=0;
    if (render_saved) {
        if (writable(camera_ctrl+0x60,sizeof(saved_ctrl))) memcpy(camera_ctrl+0x60,saved_ctrl,sizeof(saved_ctrl));
        uint32_t global = global_slot ? *global_slot : 0;
        if (global && (uintptr_t)saved_global_camera == (uintptr_t)global+0x74
                && writable(saved_global_camera,sizeof(saved_global)))
            memcpy(saved_global_camera,saved_global,sizeof(saved_global));
        render_saved = 0; saved_global_camera = NULL;
    }
    if (eye_matrix && writable(eye_matrix, sizeof(saved_eye_matrix)))
        memcpy(eye_matrix, saved_eye_matrix, sizeof(saved_eye_matrix));
    eye_matrix = NULL;
    uint32_t player = current_player();
    if (hidden_player && player == hidden_player && *(uint32_t *)(uintptr_t)player == hidden_vtable
            && writable((void *)(uintptr_t)(player + 4), 4)) {
        uint32_t *flags = (uint32_t *)(uintptr_t)(player + 4);
        *flags = re4_restore_model(*flags, saved_draw_bits);
    }
    uint32_t controller = player == hidden_player ? weapon_controller(player) : 0;
    for (unsigned i=0;i<2;++i) {
        HiddenWeapon *w = &hidden_weapons[i];
        if (w->object && controller
                && re4_model_slot_matches(w->controller,controller,w->object,*(uint32_t *)(uintptr_t)(controller+w->slot))
                && readable((void *)(uintptr_t)w->object,8)
                && *(uint32_t *)(uintptr_t)w->object == w->vtable
                && writable((void *)(uintptr_t)(w->object+4),4)) {
            uint32_t *draw = (uint32_t *)(uintptr_t)(w->object+4);
            *draw = re4_restore_model(*draw,w->draw_bits);
        }
        memset(w,0,sizeof(*w));
    }
    hidden_player = 0;
}

/* Camera/model leases are per native update. HUD visibility is per FP session:
   restoring it before every native callback allowed cockpit drawing between callbacks. */
static void restore_first_person(void){native_hud_restore();restore_rendered_camera();}
static void __attribute__((fastcall)) camera_hook(void *self, void *unused,
        void *matrix, void *offset, CameraPoint *point) {
    runtime_cache_valid=0;
    if(runtime_scripted()){restore_first_person();state->camera_active=0;original_camera(self,unused,matrix,offset,point);return;}
    // Auxiliary cameras must not clear the primary eye/render lease.
    if(self!=camera_ctrl+0x278){original_camera(self,unused,matrix,offset,point);return;}
    restore_rendered_camera();
    InterlockedIncrement(&state->camera_ticks);
    state->camera_active = 0;
    ControlFrame control = {0};
    int complete = read_control_frame(&control);
    uint32_t stamp = control.stamp, armed = control.armed, flags = control.flags, generation = control.generation;
    float total = control.total, pitch = control.pitch, eye = control.eye;
    if(complete && armed && state->armed && !state->stop)cube_collision_tick(&control,complete,GetTickCount());
    uint32_t player = complete && armed && state->armed && !state->stop && (flags&3u)==3u ? current_player() : 0;
    if (!complete || !armed || !state->armed || state->stop
            || (uint32_t)(GetTickCount()-stamp) >= RE4_TTL_MS || (flags & 3u) != 3u
            || !(state->capabilities & 2u) || !re4_valid_look(total,pitch,eye,flags) || !player
            || self != camera_ctrl + 0x278
            || !writable(matrix,48) || !writable(point,sizeof(*point))
            || !writable((void *)(uintptr_t)(player + 4),4)
            || !readable(offset,0x2C)) {
        native_hud_restore();
        original_camera(self, unused, matrix, offset, point);
        return;
    }
    float frame[12], position[3], target[3];
    float feet[3], yaw;
    runtime_vertical_interpolate(GetTickCount());
    memcpy(feet, (void *)(uintptr_t)(player + 0x94), sizeof(feet));if(runtime_actor_active)feet[1]=runtime_view_y;
    yaw = view_yaw;
    float fovy; memcpy(&fovy,(unsigned char *)offset+0x28,4);
    if (!re4_eye_frame(feet,yaw,pitch,eye,frame,position,target) || *player_slot != player
            || !isfinite(fovy) || fovy > 179.f || last_player != player || last_arm_id != generation) {
        native_hud_restore();
        original_camera(self, unused, matrix, offset, point);
        return;
    }
    /* Active FP replaces hitCheck, as in the documented freecam reference.
       Calling it first leaves native orbit/collision side effects in QuasiFPS. */
    /* Preserve the game's matrix for the next update; no accumulation across frames. */
    if (matrix == camera_ctrl + 0x278 + 0x178) {
        memcpy(saved_eye_matrix,matrix,sizeof(saved_eye_matrix)); eye_matrix = matrix;
    } /* Stack-local argument matrices must never be retained beyond this call. */
    memcpy(matrix,frame,sizeof(frame));
    memcpy(point->position,position,sizeof(position)); memcpy(point->target,target,sizeof(target));
    point->roll = 0.f;
    point->fovy = fovy > 0.f ? fovy : 50.f;
    render_point = *point;
    for (unsigned i=0;i<3;++i) {
        render_point.position[i] = frame[4*i+3];
        render_point.target[i] += render_point.position[i];
    }
    re4_view_frame(render_point.position,yaw,pitch,render_matrix,render_view,render_right,render_up,render_back);
    render_player = player; render_generation = generation;
    uint32_t *draw = (uint32_t *)(uintptr_t)(player + 4);
    hidden_player = player; hidden_vtable = *(uint32_t *)(uintptr_t)player;
    saved_draw_bits = *draw & RE4_MODEL_DRAW_BITS;
    *draw = re4_hide_model(*draw);
    hide_weapons(player);native_hud_hide();
    state->camera_active = 1;
}

static void write_render_camera(unsigned char *camera) {
    memcpy(camera,render_matrix,48); memcpy(camera+0x30,render_view,48);
    memcpy(camera+0xA4,&render_point,sizeof(render_point));
    memcpy(camera+0xC4,render_up,12); memcpy(camera+0xD0,render_back,12);
    memcpy(camera+0xDC,render_right,12);
    float distance = 1000.f; memcpy(camera+0xF4,&distance,4);
}
static float observed_camera_yaw(unsigned char *camera) {
    if (!readable(camera+0xA4,sizeof(CameraPoint))) return 0.f;
    CameraPoint point; memcpy(&point,camera+0xA4,sizeof(point));
    float x=point.target[0]-point.position[0], z=point.target[2]-point.position[2];
    return isfinite(x) && isfinite(z) ? atan2f(x,z) : 0.f;
}
/* Build the final view from the current feet and independent gaze, even when
   an idle/auxiliary camera or a later pad update ran after the primary callback. */
static int refresh_render_frame(const ControlFrame *control,uint32_t player,float fovy) {
    if(!player || last_player!=player || last_arm_id!=control->generation
            || (control->flags&3u)!=3u || !(state->capabilities&2u)
            || !re4_valid_look(control->total,control->pitch,control->eye,control->flags))return 0;
    float feet[3],frame[12],position[3],target[3];
    runtime_vertical_interpolate(GetTickCount());
    memcpy(feet,(void *)(uintptr_t)(player+0x94),sizeof(feet));if(runtime_actor_active)feet[1]=runtime_view_y;
    if(!re4_eye_frame(feet,view_yaw,control->pitch,control->eye,frame,position,target) || *player_slot!=player)return 0;
    for(unsigned i=0;i<3;++i){render_point.position[i]=frame[4*i+3];render_point.target[i]=render_point.position[i]+target[i];}
    render_point.roll=0;render_point.fovy=isfinite(fovy) && fovy>0 && fovy<179?fovy:50.f;
    re4_view_frame(render_point.position,view_yaw,control->pitch,render_matrix,render_view,render_right,render_up,render_back);
    render_player=player;render_generation=control->generation;return 1;
}
/* Reuse the validated camera/model pages only within the same native update,
   for at most 16ms. Input/pose/pointer/owner/FOV changes take the full checks. */
static int projection_reuse(uint32_t now){
    ProjectionCache *cache=&projection_cache;
    if((block_camera_scene && block_camera_frame.epoch!=block_camera_epoch) || !cache->valid || !render_saved || !state->camera_active || !global_slot || !player_slot
        || (uint32_t)state->sequence!=cache->control.sequence || state->hook_ticks!=(LONG)cache->pad_ticks
        || state->camera_ticks!=(LONG)cache->camera_ticks || (uint32_t)(now-cache->validated_at)>=16
        || (uint32_t)(now-cache->control.stamp)>=RE4_TTL_MS
        || *player_slot!=cache->player || (uintptr_t)saved_global_camera!=(uintptr_t)*global_slot+0x74)return 0;
    uint32_t player=cache->player;
    if(*(uint32_t*)(uintptr_t)player!=cache->vtable
        || memcmp(cache->feet,(void*)(uintptr_t)(player+0x94),12))return 0;
    float fovy;memcpy(&fovy,saved_global_camera+0xC0,4);
    if(fovy!=render_point.fovy)return 0;
    for(unsigned i=0;i<2;i++){
        HiddenWeapon *w=&hidden_weapons[i];
        if(!w->object)continue;
        if(*(uint32_t*)(uintptr_t)(player+0x7D8)!=w->controller
            || *(uint32_t*)(uintptr_t)(w->controller+w->slot)!=w->object
            || *(uint32_t*)(uintptr_t)w->object!=w->vtable)return 0;
    }
    if(GetAsyncKeyState(VK_F8)&0x8000){InterlockedExchange(&state->stop,1);return 0;}
    if(!state->armed || state->stop || (uint32_t)state->sequence!=cache->control.sequence
        || *player_slot!=player || (uintptr_t)saved_global_camera!=(uintptr_t)*global_slot+0x74
        || memcmp(cache->feet,(void*)(uintptr_t)(player+0x94),12))return 0;
    /* The native projection can overwrite matrices/flags; always reapply them. */
    *(uint32_t*)(uintptr_t)(player+4)=re4_hide_model(*(uint32_t*)(uintptr_t)(player+4));
    for(unsigned i=0;i<2;i++)if(hidden_weapons[i].object){uint32_t *draw=(uint32_t*)(uintptr_t)(hidden_weapons[i].object+4);*draw=re4_hide_model(*draw);}
    write_render_camera(camera_ctrl+0x60);write_render_camera(saved_global_camera);
    native_hud_reapply();
    return 1;
}
static void block_camera_publish(uint32_t now){
    InterlockedIncrement(&block_camera_frame.sequence);MemoryBarrier();
    block_camera_frame.valid=1;block_camera_frame.stamp=now;block_camera_frame.player=render_player;
    block_camera_frame.generation=render_generation;block_camera_frame.point=render_point;block_camera_frame.epoch=block_camera_epoch;
    memcpy(block_camera_frame.camera,render_matrix,48);memcpy(block_camera_frame.right,render_right,12);memcpy(block_camera_frame.up,render_up,12);memcpy(block_camera_frame.back,render_back,12);
    memcpy(block_camera_frame.view,render_view,48);
    MemoryBarrier();InterlockedIncrement(&block_camera_frame.sequence);
}
/* One scene uses one camera, even if a new mouse/input update arrives halfway
   through world rendering. Collisions continue using the current native pose. */
static int block_camera_reuse_scene(const ControlFrame *control,uint32_t player,uint32_t now){
    BlockCameraFrame *f=&block_camera_frame;
    if(!block_camera_scene || !f->valid || f->epoch!=block_camera_epoch || f->player!=player
        || f->generation!=control->generation || (uint32_t)(now-f->stamp)>=100)return 0;
    render_point=f->point;memcpy(render_matrix,f->camera,48);memcpy(render_view,f->view,48);
    memcpy(render_right,f->right,12);memcpy(render_up,f->up,12);memcpy(render_back,f->back,12);
    return 1;
}
/* Projection's validated cdecl entry sees the final CAMERA after interpolation. */
static void __cdecl projection_hook(void) {
    /* Never query actor/camera memory or poll keys on dormant projection calls.
       The observed E9 entry serves callers beyond the six recognized call sites. */
    if(!state || !state->armed || state->stop){
        runtime_action_restore();
        projection_cache.valid=0;
        restore_first_person();
        if(state)state->camera_active=0;
        if(diagnostics && diagnostics->applied){
            InterlockedIncrement(&diagnostics->sequence);MemoryBarrier();
            diagnostics->applied=0;diagnostics->rejected++;diagnostics->stamp=GetTickCount();
            MemoryBarrier();InterlockedIncrement(&diagnostics->sequence);
        }
        original_projection();return;
    }
    if(runtime_scripted()){projection_cache.valid=0;restore_first_person();state->camera_active=0;runtime_action_projection();original_projection();return;}
    if(projection_reuse(GetTickCount())){original_projection();return;}
    projection_cache.valid=0;
    ControlFrame control = {0};
    int complete = read_control_frame(&control);
    uint32_t armed = control.armed, stamp = control.stamp, generation = control.generation;
    uint32_t global = global_slot ? *global_slot : 0;
    unsigned char *camera = (unsigned char *)(uintptr_t)((uintptr_t)global+0x74);
    int applied = 0;
    if(GetAsyncKeyState(VK_F8)&0x8000)InterlockedExchange(&state->stop,1);
    runtime_action_projection(); // contextual requests also need the bounded fallback when PadRead is idle
    cube_collision_tick(&control,complete,GetTickCount());
    if(!block_camera_scene || !block_camera_frame.valid || block_camera_frame.epoch!=block_camera_epoch)enemy_collision_tick(&control,complete,GetTickCount());
    uint32_t player=current_player();
    float fovy=50.f;
    if(global>=0x10000 && global<=0xFFFFFE00 && readable(camera+0xC0,4))memcpy(&fovy,camera+0xC0,4);
    if (diagnostics) {
        InterlockedIncrement(&diagnostics->sequence);
        MemoryBarrier();
        diagnostics->calls++;
        diagnostics->stamp=GetTickCount();
        diagnostics->pad_ticks=(uint32_t)state->hook_ticks;
        diagnostics->camera_ticks=(uint32_t)state->camera_ticks;
        diagnostics->view_yaw=isfinite(view_yaw) ? view_yaw : 0.f;
        diagnostics->yaw_before=observed_camera_yaw(camera);
    }
    if (complete && armed && state->armed && !state->stop
            && (uint32_t)(GetTickCount()-stamp) < RE4_TTL_MS
            && refresh_render_frame(&control,player,fovy) && generation == render_generation
            && global >= 0x10000 && !(global & 3u) && global <= 0xFFFFFE00
            && writable(camera,0xF8) && writable(camera_ctrl+0x60,0xF8)) {
        if(render_saved && camera!=saved_global_camera)restore_first_person();
        if (!render_saved) {
            memcpy(saved_global,camera,sizeof(saved_global)); saved_global_camera = camera;
            memcpy(saved_ctrl,camera_ctrl+0x60,sizeof(saved_ctrl)); render_saved = 1;
        }
        if (camera == saved_global_camera) {
            if(hidden_player!=player || hidden_vtable!=*(uint32_t *)(uintptr_t)player){hidden_player=player;hidden_vtable=*(uint32_t *)(uintptr_t)player;saved_draw_bits=*(uint32_t *)(uintptr_t)(player+4)&RE4_MODEL_DRAW_BITS;}
            if(writable((void *)(uintptr_t)(player+4),4))*(uint32_t *)(uintptr_t)(player+4)=re4_hide_model(*(uint32_t *)(uintptr_t)(player+4));
            hide_weapons(player);native_hud_hide();
            int reused_scene=block_camera_reuse_scene(&control,player,GetTickCount());
            write_render_camera(camera_ctrl+0x60); write_render_camera(camera);
            state->camera_active=1;applied = 1;
            if(!reused_scene)block_camera_publish(GetTickCount());
            projection_cache=(ProjectionCache){.valid=1,.control=control,.pad_ticks=(uint32_t)state->hook_ticks,
                .camera_ticks=(uint32_t)state->camera_ticks,.player=player,.vtable=hidden_vtable,.validated_at=GetTickCount()};
            memcpy(projection_cache.feet,(void*)(uintptr_t)(player+0x94),12);
        }
    } else {restore_first_person();state->camera_active=0;}
    if (diagnostics) {
        diagnostics->applied=(uint32_t)applied;
        if (applied) diagnostics->writes++; else diagnostics->rejected++;
        diagnostics->yaw_written=observed_camera_yaw(camera);
    }
    original_projection();
    if (diagnostics) {
        diagnostics->yaw_after=observed_camera_yaw(camera);
        MemoryBarrier();
        InterlockedIncrement(&diagnostics->sequence);
    }
}

#include "health_ipc.h"

static void __cdecl pad_hook(void) {
    runtime_pad_stamp=GetTickCount();
    runtime_action_restore();
    restore_rendered_camera();
    state->camera_active = 0;
    original_pad();
    runtime_refresh(); // report the current scene even while waiting for a new arm
    int scripted=runtime_scripted();
    int fp_live=0;
    InterlockedIncrement(&state->hook_ticks);
    if (GetAsyncKeyState(VK_F8) & 0x8000) InterlockedExchange(&state->stop, 1);
    ControlFrame control = {0};
    int complete = read_control_frame(&control);
    uint32_t before = control.sequence, stamp = control.stamp, armed = control.armed, buttons = control.buttons;
    uint32_t flags = control.flags, generation = control.generation;
    float total = control.total, pitch = control.pitch, eye = control.eye;
    uint32_t now = GetTickCount();
    if ((complete && armed && (uint32_t)(now-stamp)>=RE4_TTL_MS)
            || (!complete && have_cached_frame && cached_frame.armed && state->armed
                && (uint32_t)(now-cached_frame.stamp)>=RE4_TTL_MS))
        InterlockedCompareExchange(&state->stop, 2, 0);
    if (complete && armed && state->armed && !state->stop && (uint32_t)(now - stamp) < RE4_TTL_MS) {
        uint32_t input = re4_buttons(before,before,now,stamp,armed,(uint32_t)state->stop,buttons);
        if(scripted)input=0;
        if (!scripted && (state->capabilities & 1u) && (flags & 1u) && re4_valid_look(total,pitch,eye,flags)) {
            uint32_t player = current_player();
            if (player && writable((void *)(uintptr_t)(player + 0xA4),4)) {
                if (last_arm_id != generation || last_player != player) {
                    consumed_yaw = (last_arm_id == generation && last_player != player) ? total : 0;
                    last_arm_id = generation; last_player = player;
                    last_look_sequence = (uint32_t)before - 2u;
                    memcpy(&view_yaw,(void *)(uintptr_t)(player+0xA4),4);
                }
                if (last_look_sequence != (uint32_t)before) {
                    float *yaw = (float *)(uintptr_t)(player + 0xA4);
                    if (*player_slot == player && isfinite(*yaw) && fabsf(*yaw) <= 100000000.f) {
                        float delta = (flags&2u)?re4_take_view_yaw(total,&consumed_yaw):re4_take_yaw(total,&consumed_yaw);
                        view_yaw = (flags&4u)?re4_wrap_yaw(-total*(3.14159265358979323846f/180.f)):re4_wrap_yaw(((flags & 2u) ? view_yaw : *yaw) + delta);
                        *yaw = view_yaw;
                        state->applied_yaw = view_yaw;
                        InterlockedIncrement(&state->look_applied_count);
                    }
                    last_look_sequence = (uint32_t)before;
                }
                if ((state->capabilities & 2u) && (flags & 3u) == 3u && isfinite(view_yaw) && *player_slot == player) {
                    uint32_t movement = input & RE4_MOVEMENT_MASK;
                    *(float *)(uintptr_t)(player+0xA4) = movement ? re4_walk_heading(view_yaw,movement) : view_yaw;
                    input = re4_relative_input(input); relative_walk = 1;native_hud_hide();fp_live=1;
                }
            }
        }
        *key_on = (*key_on & ~(uint64_t)RE4_CONTROL_MASK) | input;
        state->native_buttons = input;
    } else {
        uint32_t player = relative_walk ? current_player() : 0;
        if (relative_walk && player == last_player && isfinite(view_yaw)
                && writable((void *)(uintptr_t)(player+0xA4),4))
            *(float *)(uintptr_t)(player+0xA4) = view_yaw;
        relative_walk = 0; state->native_buttons = 0; last_arm_id = 0; consumed_yaw = 0;
    }
    if(!fp_live)native_hud_restore();
    runtime_vertical_tick(&control,complete,now);
    cube_collision_tick(&control,complete,now);
    health_tick(&control,complete,now);
    enemy_collision_tick(&control,complete,now);
    runtime_action_apply(&control,complete,now);
    runtime_publish(runtime_last_scene,runtime_cache_flags);
}

/* Literal bytes with -1 wildcards; these are interoperable signatures, not absolute addresses. */
static const int version_pattern[] = {0x68,-1,-1,-1,-1,0x8B,0xC8,0xE8,-1,-1,-1,-1,0xDC,0x2D,-1,-1,-1,-1,0xA1,-1,-1,-1,-1,0xDC,0x25};
static const int key_pattern[] = {0xA1,-1,-1,-1,-1,0x33,0xC9,0x83,0xE0,-1,0x83,0x3D};
static const int pad_pattern[] = {0xE8,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0x68,-1,-1,-1,-1,0x6A,0x04,0xE8,-1,-1,-1,-1,0x83,0xC4,0x18,0xE8,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0xA1};
static const int trigger_pattern[] = {0xA1,-1,-1,-1,-1,0x25,-1,-1,-1,-1,0x0B,0xC1,0x74,-1,0x88,0x0D};
static const int enemy_pattern[] = {0x81,0xE1,0x01,0x02,0x00,0x00,0x83,0xF9,0x01,0x75,-1,0x50,0xB9,-1,-1,-1,-1,0xE8};
static const int player_pattern[] = {0xA1,-1,-1,-1,-1,0xD8,0xCC,0xD8,0xC9,0xD8,0xCA,0xD9,0x5D,-1,0xD9,0x45,-1};
static const int camera_pattern[] = {0x52,0x8D,0x85,-1,-1,-1,-1,0x50,0x8D,0x4D,-1,0x51,0x8B,0xCB,0xE8};
static const int smooth_pattern[] = {0xE8,-1,-1,-1,-1,0x80,0xBB,0xB6,0x01,0x00,0x00,0x00};
static const int ctrl_pattern[] = {0xD9,0xEE,0xB9,-1,-1,-1,-1,0xD9,0x9E,0xA4,0x00,0x00,0x00,0xE8,-1,-1,-1,-1,0xD9,0xEE};
static const int global_pattern[] = {0xA1,-1,-1,-1,-1,0xB9,0xFF,0xFF,0xFF,0x7F,0x21,0x48,-1,0xA1};
static const int id_system_pattern[] = {0xB9,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0x8B,-1,-1,-1,-1,-1,0x8B,0xC8,0xD9};
static const int projection_pattern[] = {0xE8,-1,-1,-1,-1,0x8A,0x46,0x30,0x3C,0xFD};

static unsigned find_patterns(unsigned char *base, DWORD image_size,
        IMAGE_NT_HEADERS32 *nt, const int *pattern, SIZE_T length,
        unsigned char **found, unsigned capacity) {
    unsigned count = 0;
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    if (!readable(sections, nt->FileHeader.NumberOfSections * sizeof(*sections))) return 0;
    for (unsigned s = 0; s < nt->FileHeader.NumberOfSections; ++s) {
        DWORD rva = sections[s].VirtualAddress, size = sections[s].Misc.VirtualSize;
        if (!(sections[s].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        if (rva >= image_size || size > image_size - rva || size > 32u*1024u*1024u || size < length
                || !readable(base + rva, size)) return 0;
        for (SIZE_T i = 0; i <= size - length; ++i) {
            if (base[rva + i] != pattern[0]) continue;
            SIZE_T j;
            for (j = 1; j < length; ++j) if (pattern[j] >= 0 && base[rva+i+j] != pattern[j]) break;
            if (j == length) {
                if (count == capacity) return 0;
                found[count++] = base+rva+i;
            }
        }
    }
    return count;
}
static unsigned char *unique_pattern(unsigned char *base, DWORD image_size,
        IMAGE_NT_HEADERS32 *nt, const int *pattern, SIZE_T length) {
    unsigned char *found;
    return find_patterns(base,image_size,nt,pattern,length,&found,1) == 1 ? found : NULL;
}

#include "runtime_discovery.h"
static unsigned collect_pattern_operands(unsigned char *base,DWORD size,IMAGE_NT_HEADERS32 *nt,
        const int *pattern,SIZE_T length,SIZE_T offset,SIZE_T bytes,uint32_t *targets,unsigned capacity){
    unsigned char *sites[64];
    unsigned count=find_patterns(base,size,nt,pattern,length,sites,64);
    unsigned used=re4_distinct_operands(sites,count,offset,(uintptr_t)base,size,bytes,targets,capacity);
    for(unsigned i=0;i<used;i++)if(!readable((void*)(uintptr_t)targets[i],bytes))return 0;
    return used;
}

static int cube_camera(float *view,float *fov,uint32_t *player) {
    if(runtime_scripted())return 0;
    *player=current_player();if (!*player || !global_slot || !readable((void *)global_slot,4)) return 0;
    ControlFrame control={0};
    if(!read_control_frame(&control))return 0;
    if(state->armed && !state->stop && control.armed && (control.flags&3u)==3u){
        uint32_t now=GetTickCount();
        if((uint32_t)(now-control.stamp)>=RE4_TTL_MS)return 0;
        for(unsigned attempt=0;attempt<3;++attempt){
            LONG before=block_camera_frame.sequence;if(before&1)continue;
            MemoryBarrier();BlockCameraFrame frame=block_camera_frame;MemoryBarrier();
            if(before!=block_camera_frame.sequence)continue;
            if(!frame.valid || frame.player!=*player || frame.generation!=control.generation
                || (uint32_t)(now-frame.stamp)>=100 || !state->armed || state->stop)return 0;
            memcpy(view,frame.view,48);*fov=frame.point.fovy;return 1;
        }
        return 0; // never draw FP blocks from an auxiliary/native orbit camera
    }
    uint32_t global=*global_slot;
    if (global<0x10000 || global>0xFFFFFE00 || (global&3u)
            || !readable((void *)(uintptr_t)(global+0x74),0xF8)) return 0;
    const unsigned char *camera=(const unsigned char *)(uintptr_t)(global+0x74);
    memcpy(view,camera+0x30,48);memcpy(fov,camera+0xC0,4);return 1;
}
/* Creation-owned facades; withdrawn block_render.h remains excluded. */
static int cube_projection(float *projection){
    if(!global_slot || !readable((void*)global_slot,4))return 0;
    uint32_t global=*global_slot;if(global<0x10000 || global>0xFFFFFE00 || (global&3u) || !readable((void*)(uintptr_t)(global+0x74+0x64),64))return 0;
    memcpy(projection,(void*)(uintptr_t)(global+0x74+0x64),64);
    for(unsigned i=0;i<16;i++)if(!isfinite(projection[i]) || fabsf(projection[i])>100000000.f)return 0;
    return 1;
}
#include "d3d9_capture.h"
static int cube_collision_graphics_ready(void) {
    AcquireSRWLockShared(&cube_metrics_lock);
    int ready=cube_metrics.active && cube_metrics.live_devices && !cube_metrics.restore_failures && !cube_metrics.escapes;
    ReleaseSRWLockShared(&cube_metrics_lock);return ready;
}
#include "collision_tick.h"
#include "enemy_collision.h"

static void install_hook(void) {
    wchar_t name[80];
    swprintf(name, 80, L"Local\\RE4Bridge-Control-%lu", (unsigned long)GetCurrentProcessId());
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Shared), name);
    if (!mapping) return;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mapping); return; }
    state = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!state) { CloseHandle(mapping); return; }
    shared_mapping = mapping;
    memset(state, 0, sizeof(*state)); state->magic = RE4_MAGIC; state->version = 2; state->status = 2;
    capture_ensure_mapping();
    health_ipc_open();
    runtime_ipc_open();
    swprintf(name,80,L"Local\\RE4Bridge-CameraDiagnostics-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE telemetry = CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CameraDiagnostics),name);
    if (telemetry) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) CloseHandle(telemetry);
        else {
            diagnostics=MapViewOfFile(telemetry,FILE_MAP_ALL_ACCESS,0,0,sizeof(CameraDiagnostics));
            if (!diagnostics) CloseHandle(telemetry);
            else {
                diagnostic_mapping=telemetry;
                memset(diagnostics,0,sizeof(*diagnostics));
                diagnostics->magic=0x52453444; diagnostics->version=1;
                diagnostics->build=450; diagnostics->pid=GetCurrentProcessId();
            }
        }
    }
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!readable(base, sizeof(IMAGE_DOS_HEADER))) return;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 64 || dos->e_lfanew > 1024*1024) return;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
    if (!readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE
            || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
            || nt->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER32)
            || nt->FileHeader.NumberOfSections > 96) return;
    DWORD size = nt->OptionalHeader.SizeOfImage;
    if (size < 4096 || size > 128u*1024u*1024u) return;
    unsigned char *v = unique_pattern(base,size,nt,version_pattern,sizeof(version_pattern)/sizeof(int));
    unsigned char *k = unique_pattern(base,size,nt,key_pattern,sizeof(key_pattern)/sizeof(int));
    unsigned char *call = unique_pattern(base,size,nt,pad_pattern,sizeof(pad_pattern)/sizeof(int));
    if (!v || !k || !call) return;
    uint32_t version_address, keys_address;
    memcpy(&version_address,v+1,4); memcpy(&keys_address,k+1,4);
    if (version_address < (uintptr_t)base || version_address > (uintptr_t)base+size-6
            || !readable((void *)(uintptr_t)version_address,6) || memcmp((void *)(uintptr_t)version_address,"1.1.0",6)) return;
    if (keys_address < (uintptr_t)base || keys_address > (uintptr_t)base+size-8
            || !readable((void *)(uintptr_t)keys_address,8)) return;
    MEMORY_BASIC_INFORMATION key_memory;
    VirtualQuery((void *)(uintptr_t)keys_address,&key_memory,sizeof(key_memory));
    if (!(key_memory.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) return;
    int32_t displacement;
    memcpy(&displacement,call+1,4);
    uintptr_t wrapper = (uintptr_t)(call+5) + displacement;
    if (wrapper < (uintptr_t)base || wrapper > (uintptr_t)base+size-5 || !readable((void *)wrapper,5)) return;
    /* The observed UHD entry is an E9 tail thunk. Preserve CALL/JMP semantics. */
    call = (unsigned char *)wrapper;
    uint32_t target_rva;
    if (!re4_relative_target(call, (uint32_t)(wrapper - (uintptr_t)base), size, &target_rva)) return;
    uintptr_t target = (uintptr_t)base + target_rva;
    if (!readable((void *)target,1)) return;
    MEMORY_BASIC_INFORMATION code_memory;
    VirtualQuery((void *)target,&code_memory,sizeof(code_memory));
    if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
    original_pad = (void (__cdecl *)(void))target;
    key_on = (volatile uint64_t *)(uintptr_t)keys_address;
    unsigned char *trigger=unique_pattern(base,size,nt,trigger_pattern,sizeof(trigger_pattern)/sizeof(int));
    if(trigger){uint32_t address;memcpy(&address,trigger+1,4);if(!(address&3u) && address>=(uintptr_t)base && address<=(uintptr_t)base+size-8 && writable((void*)(uintptr_t)address,8))key_trigger=(volatile uint64_t*)(uintptr_t)address;}
    enemy_candidate_count=collect_pattern_operands(base,size,nt,enemy_pattern,sizeof(enemy_pattern)/sizeof(int),13,32,enemy_candidates,64);
    enemy_manager=0;runtime_enemy_available=0; // choose the typed pool after game initialization

    DWORD old_protect;
    state->status = 3;
    if (!VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&old_protect)) return;
    int32_t replacement = (int32_t)((uintptr_t)pad_hook - (uintptr_t)(call+5));
    memcpy(call+1,&replacement,4);
    FlushInstructionCache(GetCurrentProcess(),call,5);
    DWORD ignored;
    if (!VirtualProtect(call,5,old_protect,&ignored)) return;
    state->status = 1;
    image_base = base; image_size = size;
    unsigned char *p = unique_pattern(base,size,nt,player_pattern,sizeof(player_pattern)/sizeof(int));
    uint32_t slot;
    if (!p) return;
    memcpy(&slot,p+1,4);
    if (slot < (uintptr_t)base || slot > (uintptr_t)base+size-4 || !readable((void *)(uintptr_t)slot,4)) return;
    player_slot = (volatile uint32_t *)(uintptr_t)slot;
    state->capabilities = 5; /* Mouse + KEY_RUN; FP is added after camera validation. */
    unsigned char *c = unique_pattern(base,size,nt,camera_pattern,sizeof(camera_pattern)/sizeof(int));
    unsigned char *ctrl = unique_pattern(base,size,nt,ctrl_pattern,sizeof(ctrl_pattern)/sizeof(int));
    unsigned char *smooth = unique_pattern(base,size,nt,smooth_pattern,sizeof(smooth_pattern)/sizeof(int));
    unsigned char *g = unique_pattern(base,size,nt,global_pattern,sizeof(global_pattern)/sizeof(int));
    unsigned char *projections[32];
    uint32_t projection_rvas[32];
    unsigned projection_count = find_patterns(base,size,nt,projection_pattern,
            sizeof(projection_pattern)/sizeof(int),projections,32);
    if (!c || !ctrl || !smooth || !g || !projection_count) return;
    uint32_t global_address; memcpy(&global_address,g+1,4);
    if ((global_address & 3u) || global_address < (uintptr_t)base
            || global_address > (uintptr_t)base+size-4
            || !readable((void *)(uintptr_t)global_address,4)) return;
    for (unsigned i=0;i<projection_count;++i) projection_rvas[i] = (uint32_t)(projections[i]-base);
    uint32_t projection_target_rva;
    int observed_profile = 0;
    if (!re4_shared_call_target((const unsigned char *const *)projections,projection_rvas,
            projection_count,size,&projection_target_rva)) {
        uint32_t thunks[32], destinations[32]; unsigned selected[32], selected_count;
        for (unsigned i=0;i<projection_count;++i) {
            if (projections[i][0]!=0xE8 || !re4_relative_target(projections[i],projection_rvas[i],size,&thunks[i])
                    || thunks[i]>size-5 || !readable(base+thunks[i],5) || base[thunks[i]]!=0xE9
                    || !re4_relative_target(base+thunks[i],thunks[i],size,&destinations[i])
                    || !readable(base+destinations[i],1)) return;
            VirtualQuery(base+thunks[i],&code_memory,sizeof(code_memory));
            if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
            VirtualQuery(base+destinations[i],&code_memory,sizeof(code_memory));
            if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
        }
        if (!re4_observed_projection_profile(projection_rvas,thunks,destinations,projection_count,
                selected,&selected_count,&projection_target_rva)) return;
        /* Ascending indices permit compaction without overwriting unread sites. */
        for (unsigned i=0;i<selected_count;++i) projections[i] = projections[selected[i]];
        projection_count = selected_count;
        observed_profile = 1;
    }
    uintptr_t projection_target = (uintptr_t)base+projection_target_rva;
    if (projection_target < (uintptr_t)base || projection_target >= (uintptr_t)base+size
            || !readable((void *)projection_target,1)) return;
    VirtualQuery((void *)projection_target,&code_memory,sizeof(code_memory));
    if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
    uint32_t ctrl_address; memcpy(&ctrl_address,ctrl+3,4);
    if (ctrl_address < (uintptr_t)base || ctrl_address > (uintptr_t)base+size-0x788
            || !readable((void *)(uintptr_t)ctrl_address,0x788)) return;
    memcpy(&displacement,c+15,4);
    uintptr_t camera_wrapper = (uintptr_t)(c+19) + displacement;
    if (camera_wrapper < (uintptr_t)base || camera_wrapper > (uintptr_t)base+size-5
            || !readable((void *)camera_wrapper,5)) return;
    uint32_t camera_target_rva;
    if (!re4_relative_target((unsigned char *)camera_wrapper, (uint32_t)(camera_wrapper-(uintptr_t)base),size,&camera_target_rva)) return;
    uintptr_t camera_target = (uintptr_t)base+camera_target_rva;
    if (!readable((void *)camera_target,1)) return;
    VirtualQuery((void *)camera_target,&code_memory,sizeof(code_memory));
    if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
    camera_ctrl = (unsigned char *)(uintptr_t)ctrl_address;
    original_camera = (void (__attribute__((fastcall)) *)(void *,void *,void *,void *,CameraPoint *))camera_target;
    if (!VirtualProtect((void *)camera_wrapper,5,PAGE_EXECUTE_READWRITE,&old_protect)) return;
    replacement = (int32_t)((uintptr_t)camera_hook - (camera_wrapper+5));
    memcpy((void *)(camera_wrapper+1),&replacement,4);
    FlushInstructionCache(GetCurrentProcess(),(void *)camera_wrapper,5);
    if (!VirtualProtect((void *)camera_wrapper,5,old_protect,&ignored)) return;
    unsigned char *cmp = smooth+5;
    smooth_continue = cmp+7;
    if (!VirtualProtect(cmp,7,PAGE_EXECUTE_READWRITE,&old_protect)) return;
    unsigned char patch[7] = {0xE9,0,0,0,0,0x90,0x90};
    replacement = (int32_t)((uintptr_t)smoothing_hook - (uintptr_t)(cmp+5));
    memcpy(patch+1,&replacement,4); memcpy(cmp,patch,7);
    FlushInstructionCache(GetCurrentProcess(),cmp,7);
    if (!VirtualProtect(cmp,7,old_protect,&ignored)) return;
    global_slot = (volatile uint32_t *)(uintptr_t)global_address;
    original_projection = (void (__cdecl *)(void))projection_target;
    if (observed_profile) {
        /* Hook the proven E9 entry, covering callers outside the short pattern.
           original_projection is the canonical body, never this patched thunk. */
        unsigned char *entry = base+0xe395;
        if (!VirtualProtect(entry,5,PAGE_EXECUTE_READWRITE,&old_protect)) return;
        replacement = (int32_t)((uintptr_t)projection_hook-(uintptr_t)(entry+5));
        memcpy(entry+1,&replacement,4);
        FlushInstructionCache(GetCurrentProcess(),entry,5);
        if (!VirtualProtect(entry,5,old_protect,&ignored)) return;
    } else for (unsigned i=0;i<projection_count;++i) {
        unsigned char *projection = projections[i];
        if (!VirtualProtect(projection,5,PAGE_EXECUTE_READWRITE,&old_protect)) return;
        replacement = (int32_t)((uintptr_t)projection_hook-(uintptr_t)(projection+5));
        memcpy(projection+1,&replacement,4);
        FlushInstructionCache(GetCurrentProcess(),projection,5);
        if (!VirtualProtect(projection,5,old_protect,&ignored)) return;
    }
    state->capabilities |= 10u; /* FP + mapped absolute gaze. */
    unsigned char *ids=unique_pattern(base,size,nt,id_system_pattern,sizeof(id_system_pattern)/sizeof(int));
    if(ids){
        uint32_t system;memcpy(&system,ids+1,4);
        if(!(system&3u) && system>=(uintptr_t)base && system<=(uintptr_t)base+size-0x58
            && readable((void*)(uintptr_t)system,0x58) && writable((void*)(uintptr_t)(system+0x30),4)){
            native_hud_system=system;state->capabilities|=16u;
        }
    }
}

static BOOL CALLBACK initialize(PINIT_ONCE unused, PVOID parameter, PVOID *context) {
    (void)unused; (void)parameter; (void)context;
    wchar_t path[MAX_PATH];
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (!length || length > MAX_PATH-14) return FALSE;
    memcpy(path+length,L"\\dinput8.dll",sizeof(L"\\dinput8.dll"));
    system_dinput = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return system_dinput != NULL;
}
static FARPROC system_export(const char *name) {
    if (!InitOnceExecuteOnce(&once,initialize,NULL,NULL)) return NULL;
    return GetProcAddress(system_dinput,name);
}

__declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version,
        const IID *iid, LPVOID *output, LPVOID outer) {
    typedef HRESULT (WINAPI *Create)(HINSTANCE,DWORD,const IID *,LPVOID *,LPVOID);
    Create original = (Create)system_export("DirectInput8Create");
    if (!original) return E_FAIL;
    capture_retry_import();
    HRESULT result = original(instance,version,iid,output,outer);
    static LONG installed;
    if (SUCCEEDED(result) && InterlockedCompareExchange(&installed,1,0) == 0) install_hook();
    return result;
}
__declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void) { return S_FALSE; }
__declspec(dllexport) HRESULT WINAPI DllGetClassObject(const IID *clsid,const IID *iid,LPVOID *output) {
    typedef HRESULT (WINAPI *GetClass)(const IID *,const IID *,LPVOID *);
    GetClass original = (GetClass)system_export("DllGetClassObject");
    return original ? original(clsid,iid,output) : E_FAIL;
}
__declspec(dllexport) HRESULT WINAPI DllRegisterServer(void) { return E_NOTIMPL; }
__declspec(dllexport) HRESULT WINAPI DllUnregisterServer(void) { return E_NOTIMPL; }
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        /* Import redirection only. Actual COM wrapping/IPC starts after entry. */
        capture_install_import(1);
    }
    return TRUE;
}
