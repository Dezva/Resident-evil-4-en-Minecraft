/* Separate command lease and read-only telemetry. Initialized after CreateDevice. */
#include "cube_policy.h"
#include "hud_ipc.h"
typedef struct {
    uint32_t magic,version;volatile LONG sequence;
    uint32_t build,pid,live_devices,wrap_failures,escapes,begins,frames,draws,failures,
             restore_failures,last_hresult,last_restore_hresult,active,reason,resets,
             reset_failures,present_failures,rejections,camera_rejections,pass_skips;
} CubeTelemetry;
_Static_assert(sizeof(CubeTelemetry)==92,"Cube telemetry layout");
typedef struct {uint32_t magic,version;volatile LONG sequence;uint32_t build,pid,ticks,contacts,corrections,rejections,reason,active,generation;} CollisionTelemetry;
_Static_assert(sizeof(CollisionTelemetry)==48,"Collision telemetry layout");
static CollisionTelemetry *collision_shared;static HANDLE collision_mapping;
static CubeTelemetry cube_metrics={.magic=0x52453455,.version=1,.build=434,.reason=CUBE_OFF};
static CubeTelemetry *cube_shared;
static CubeCommand *cube_command;
static HANDLE cube_telemetry_mapping,cube_command_mapping;
static SRWLOCK cube_metrics_lock=SRWLOCK_INIT;
static INIT_ONCE cube_mapping_once=INIT_ONCE_STATIC_INIT;
static void cube_publish_locked(void) {
    if (!cube_shared) return;
    InterlockedIncrement(&cube_shared->sequence);MemoryBarrier();
    cube_shared->magic=cube_metrics.magic;cube_shared->version=cube_metrics.version;
    memcpy((unsigned char *)cube_shared+12,(unsigned char *)&cube_metrics+12,sizeof(CubeTelemetry)-12);
    MemoryBarrier();InterlockedIncrement(&cube_shared->sequence);
}
static BOOL CALLBACK cube_initialize(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    (void)once;(void)parameter;(void)context;
    wchar_t name[80];DWORD pid=GetCurrentProcessId();
    swprintf(name,80,L"Local\\RE4Bridge-CubeTelemetry-%lu",(unsigned long)pid);
    HANDLE telemetry=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CubeTelemetry),name);
    if (!telemetry) return FALSE;
    if (GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(telemetry);return FALSE;}
    CubeTelemetry *view=MapViewOfFile(telemetry,FILE_MAP_ALL_ACCESS,0,0,sizeof(CubeTelemetry));
    if (!view) {CloseHandle(telemetry);return FALSE;}
    swprintf(name,80,L"Local\\RE4Bridge-CubeCommand-%lu",(unsigned long)pid);
    HANDLE command=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CubeCommand),name);
    if (!command || GetLastError()==ERROR_ALREADY_EXISTS) {if (command) CloseHandle(command);UnmapViewOfFile(view);CloseHandle(telemetry);return FALSE;}
    CubeCommand *input=MapViewOfFile(command,FILE_MAP_ALL_ACCESS,0,0,sizeof(CubeCommand));
    if (!input) {CloseHandle(command);UnmapViewOfFile(view);CloseHandle(telemetry);return FALSE;}
    memset(view,0,sizeof(*view));memset(input,0,sizeof(*input));
    input->magic=0x52453454;input->version=6;
    cube_telemetry_mapping=telemetry;cube_command_mapping=command;cube_command=input;
    cube_shared=view;cube_metrics.pid=pid;cube_publish_locked();
    swprintf(name,80,L"Local\\RE4Bridge-Collision-%lu",(unsigned long)pid);
    HANDLE barrier=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CollisionTelemetry),name);
    if(barrier && GetLastError()!=ERROR_ALREADY_EXISTS) {
        CollisionTelemetry *report=MapViewOfFile(barrier,FILE_MAP_ALL_ACCESS,0,0,sizeof(*report));
        if(report){memset(report,0,sizeof(*report));report->magic=0x52453456;report->version=1;report->build=434;report->pid=pid;report->reason=1;collision_shared=report;collision_mapping=barrier;}else CloseHandle(barrier);
    }else if(barrier)CloseHandle(barrier);
    hud_initialize(pid);
    return TRUE;
}
static int cube_ensure_mapping(void) {
    return InitOnceExecuteOnce(&cube_mapping_once,cube_initialize,NULL,NULL)!=0;
}
static int cube_command_requested(void){
    if(!cube_command)return 0;
    for(unsigned attempt=0;attempt<3;attempt++){
        uint32_t header[8],before=((volatile CubeCommand*)cube_command)->sequence;
        MemoryBarrier();memcpy(header,cube_command,sizeof(header));MemoryBarrier();
        if(!(before&1) && before==header[2] && before==((volatile CubeCommand*)cube_command)->sequence)
            return header[0]==0x52453454 && header[1]==6 && header[4]==1 && header[5]!=0 && !header[7];
    }
    return 0;
}
static int cube_command_read_prefix(CubeCommand *out,size_t bytes) {
    if (!cube_command) return 0;
    for (unsigned attempt=0;attempt<3;++attempt) {
        uint32_t before=((volatile CubeCommand *)cube_command)->sequence;
        MemoryBarrier();memcpy(out,cube_command,bytes);MemoryBarrier();
        if (!(before&1) && before==((volatile CubeCommand *)cube_command)->sequence
                && before==out->sequence && out->magic==0x52453454 && out->version==6
                && !out->reserved && !out->material_reserved && out->enabled<=1 && out->collisions<=1
                && out->tile_count<=BLOCK_MAX_TILES && (!out->collisions || (out->mode==2 && out->arm_generation>0 && out->arm_generation<=2147483647))
                && (out->style!=3 || out->tile_count>0) && out->mode<=2
                && (out->mode==2 ? out->count>0 && out->count<=CUBE_MAX_BOXES && out->style<=3 : !out->count && !out->style)) return 1;
    }
    return 0;
}
static int cube_command_read(CubeCommand *out){return cube_command_read_prefix(out,sizeof(*out));}
/* Collision needs only the lease, transform and boxes, never UVs or atlas pixels. */
static int cube_collision_command_read(CubeCommand *out){return cube_command_read_prefix(out,offsetof(CubeCommand,materials));}
static void cube_record_wrap(int delta,int failed) {
    AcquireSRWLockExclusive(&cube_metrics_lock);
    cube_metrics.live_devices=(uint32_t)((int32_t)cube_metrics.live_devices+delta);
    cube_metrics.wrap_failures+=(uint32_t)failed;cube_publish_locked();
    ReleaseSRWLockExclusive(&cube_metrics_lock);
}
static void cube_record(unsigned event,uint32_t value) {
    AcquireSRWLockExclusive(&cube_metrics_lock);
    switch (event) {
    case CUBE_BEGIN:cube_metrics.begins++;break;
    case CUBE_FRAME:cube_metrics.frames++;break;
    case CUBE_DRAW:cube_metrics.draws++;cube_metrics.last_hresult=value;cube_metrics.active=1;cube_metrics.reason=0;break;
    case CUBE_FAILURE:cube_metrics.failures++;cube_metrics.last_hresult=value;cube_metrics.active=0;cube_metrics.reason=CUBE_GRAPHICS;break;
    case CUBE_RESTORE_FAILURE:cube_metrics.restore_failures++;cube_metrics.last_restore_hresult=value;cube_metrics.active=0;cube_metrics.reason=CUBE_RESTORE;break;
    case CUBE_ESCAPE:cube_metrics.escapes++;break;
    case CUBE_RESET:cube_metrics.resets++;cube_metrics.active=0;break;
    case CUBE_RESET_FAILURE:cube_metrics.reset_failures++;cube_metrics.last_hresult=value;break;
    case CUBE_PRESENT_FAILURE:cube_metrics.present_failures++;cube_metrics.last_hresult=value;cube_metrics.active=0;cube_metrics.reason=CUBE_LOST;break;
    case CUBE_REJECT:cube_metrics.rejections++;cube_metrics.active=0;cube_metrics.reason=value;break;
    case CUBE_CAMERA_REJECT:cube_metrics.camera_rejections++;cube_metrics.active=0;cube_metrics.reason=CUBE_CAMERA;break;
    case CUBE_PASS_SKIP:cube_metrics.pass_skips++;break;
    case CUBE_ACTIVE:cube_metrics.active=value;break;
    }
    cube_publish_locked();ReleaseSRWLockExclusive(&cube_metrics_lock);
}
