/* Creation-only diagnostic. No DLL loads, COM calls or heap allocation during
 * import installation; initial DllMain creates no IPC. Addresses are historical. */
#define COBJMACROS
#include <d3d9.h>
#include "capture_import.h"
typedef struct {
    uint32_t matched;
    wchar_t loaded[MAX_PATH],wow64[MAX_PATH],system[MAX_PATH];
} CapturePaths;
_Static_assert(sizeof(CapturePaths)==1564,"Capture paths use Windows UTF-16");
typedef struct {
    uint32_t magic,version;
    volatile LONG sequence;
    uint32_t build,pid,hook_status,factory_calls,factory_success,wrappers_live,wrap_failures;
    uint32_t device_calls,device_success,device_failures,last_hresult,factory_address,device_address;
    uint32_t adapter,device_type,behavior_flags,windowed,width,height,focus_window,escaped_interfaces;
    uint32_t phase,attempts;
    CaptureAttempt initial,latest;
    CapturePaths initial_paths,latest_paths;
} CaptureShared;
_Static_assert(sizeof(CaptureShared)==3296,"Capture diagnostic IPC v3");
static CaptureShared capture_metrics={.magic=0x52453447,.version=3,.build=448};
static CaptureShared *capture_shared;
static HANDLE capture_mapping;
static SRWLOCK capture_lock=SRWLOCK_INIT;
static INIT_ONCE capture_mapping_once=INIT_ONCE_STATIC_INIT;
static IDirect3D9 *(WINAPI *capture_original_create9)(UINT);

static BOOL CALLBACK capture_init_mapping(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    (void)once;(void)parameter;(void)context;
    wchar_t name[80];
    swprintf(name,80,L"Local\\RE4Bridge-GraphicsCapture-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(CaptureShared),name);
    if (!mapping) return FALSE;
    if (GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(mapping);return FALSE;}
    CaptureShared *view=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(CaptureShared));
    if (!view) {CloseHandle(mapping);return FALSE;}
    memset(view,0,sizeof(*view));
    capture_mapping=mapping;capture_shared=view;
    return TRUE;
}
/* Writers serialize; readers only consume a complete seqlock snapshot. */
static void capture_publish_locked(void) {
    if (!capture_shared) return;
    InterlockedIncrement(&capture_shared->sequence);
    MemoryBarrier();
    capture_shared->magic=capture_metrics.magic;capture_shared->version=capture_metrics.version;
    memcpy((unsigned char *)capture_shared+12,(unsigned char *)&capture_metrics+12,sizeof(CaptureShared)-12);
    MemoryBarrier();InterlockedIncrement(&capture_shared->sequence);
}
static void capture_ensure_mapping(void) {
    InitOnceExecuteOnce(&capture_mapping_once,capture_init_mapping,NULL,NULL);
    AcquireSRWLockExclusive(&capture_lock);
    capture_metrics.pid=GetCurrentProcessId();capture_publish_locked();
    ReleaseSRWLockExclusive(&capture_lock);
}
static void capture_record_factory(IDirect3D9 *factory) {
    capture_ensure_mapping();
    AcquireSRWLockExclusive(&capture_lock);
    capture_metrics.factory_calls++;
    if (factory) capture_metrics.factory_success++;
    capture_metrics.factory_address=(uint32_t)(uintptr_t)factory;
    capture_publish_locked();ReleaseSRWLockExclusive(&capture_lock);
}
static void capture_record_wrap(int delta,int failed) {
    AcquireSRWLockExclusive(&capture_lock);
    capture_metrics.wrappers_live=(uint32_t)((int32_t)capture_metrics.wrappers_live+delta);
    capture_metrics.wrap_failures+=(uint32_t)failed;
    capture_publish_locked();ReleaseSRWLockExclusive(&capture_lock);
}
static void capture_record_escape(void) {
    AcquireSRWLockExclusive(&capture_lock);capture_metrics.escaped_interfaces++;
    capture_publish_locked();ReleaseSRWLockExclusive(&capture_lock);
}
static void capture_record_device(HRESULT hr,IDirect3DDevice9 *device,UINT adapter,D3DDEVTYPE type,HWND focus,DWORD flags,D3DPRESENT_PARAMETERS *parameters) {
    AcquireSRWLockExclusive(&capture_lock);
    capture_metrics.device_calls++;
    if (SUCCEEDED(hr) && device) capture_metrics.device_success++;
    else capture_metrics.device_failures++;
    capture_metrics.last_hresult=(uint32_t)hr;
    capture_metrics.device_address=(uint32_t)(uintptr_t)device;
    capture_metrics.adapter=adapter;capture_metrics.device_type=(uint32_t)type;
    capture_metrics.behavior_flags=flags;capture_metrics.focus_window=(uint32_t)(uintptr_t)focus;
    capture_metrics.windowed=parameters?(uint32_t)parameters->Windowed:0;
    capture_metrics.width=parameters?parameters->BackBufferWidth:0;
    capture_metrics.height=parameters?parameters->BackBufferHeight:0;
    capture_publish_locked();ReleaseSRWLockExclusive(&capture_lock);
}
#include "cube_ipc.h"
#include "cube_device.h"
#include "capture_factory.h"

static const unsigned char *capture_image_read(void *base,uint32_t offset,size_t length) {
    if (offset>UINTPTR_MAX-(uintptr_t)base) return NULL;
    unsigned char *p=(unsigned char *)base+offset;
    return readable(p,length)?p:NULL;
}
static int capture_system_path(wchar_t *path,UINT length) {
    if (!length || length>MAX_PATH-12) {path[0]=0;return 0;}
    memcpy(path+length,L"\\d3d9.dll",sizeof(L"\\d3d9.dll"));
    return 1;
}
static void capture_install_import(unsigned phase) {
    DWORD saved_error=GetLastError();
    CaptureAttempt attempt={.status=3};
    CapturePaths paths={0};
    unsigned char *base=(unsigned char *)GetModuleHandleW(NULL);
    if (!readable(base,64) || capture_u16(base)!=0x5a4d) {attempt.reason=2;goto done;}
    uint32_t pe=capture_u32(base+0x3c);
    attempt.pe_rva=pe;
    if (pe<64 || pe>1024*1024 || !readable(base+pe,248)) {attempt.reason=3;goto done;}
    uint32_t size=capture_u32(base+pe+80);
    attempt.image_size=size;
    if (size<pe+248 || size>512*1024*1024 || size>UINTPTR_MAX-(uintptr_t)base) {attempt.reason=4;goto done;}
    HMODULE d3d9=GetModuleHandleW(L"d3d9.dll");
    if (!d3d9) {attempt.status=2;attempt.reason=5;goto done;}
    /* Refuse local proxies/wrappers. Do not load or initialize any library here. */
    DWORD length=GetModuleFileNameW(d3d9,paths.loaded,MAX_PATH);
    if (!length || length>=MAX_PATH) {paths.loaded[MAX_PATH-1]=0;attempt.reason=6;goto done;}
    int wow64=capture_system_path(paths.wow64,GetSystemWow64DirectoryW(paths.wow64,MAX_PATH));
    int system=capture_system_path(paths.system,GetSystemDirectoryW(paths.system,MAX_PATH));
    if (!wow64 && !system) {attempt.reason=6;goto done;}
    /* GetModuleFileName preserves the loader's spelling. On WOW64, an import
       loaded via System32 can refer to the redirected SysWOW64 DLL. Both exact
       OS-provided paths are valid; local/proxy/arbitrary paths remain refused. */
    if (wow64 && !lstrcmpiW(paths.loaded,paths.wow64)) paths.matched=1;
    else if (system && !lstrcmpiW(paths.loaded,paths.system)) paths.matched=2;
    else {attempt.reason=7;goto done;}
    FARPROC original=GetProcAddress(d3d9,"Direct3DCreate9");
    if (!original) {attempt.reason=8;goto done;}
    uint32_t slot=0;
    CaptureImportDetails details;
    int found=capture_find_import(capture_image_read,base,size,(uint32_t)(uintptr_t)original,&slot,&details);
    attempt.slot_rva=details.slot;attempt.observed=details.observed;attempt.expected=details.expected;
    if (found!=1) {attempt.status=(uint32_t)found;attempt.reason=details.reason;goto done;}
    LONG *iat=(LONG *)(base+slot);DWORD protection,ignored;
    MEMORY_BASIC_INFORMATION region;
    if (!VirtualQuery(iat,&region,sizeof(region))) {attempt.status=4;attempt.reason=20;attempt.win_error=GetLastError();goto done;}
    DWORD temporary=(region.Protect&0xf0u)?PAGE_EXECUTE_READWRITE:PAGE_READWRITE;
    if (!VirtualProtect(iat,4,temporary,&protection)) {attempt.status=4;attempt.reason=21;attempt.win_error=GetLastError();goto done;}
    capture_original_create9=(IDirect3D9 *(WINAPI *)(UINT))original;
    LONG replaced=InterlockedCompareExchange(iat,(LONG)(uintptr_t)capture_create9,(LONG)(uintptr_t)original);
    if (replaced!=(LONG)(uintptr_t)original) {
        attempt.reason=22;attempt.observed=(uint32_t)replaced;
        if (!VirtualProtect(iat,4,protection,&ignored)) {attempt.status=4;attempt.reason=23;attempt.win_error=GetLastError();}
        goto done;
    }
    if (!VirtualProtect(iat,4,protection,&ignored)) {
        attempt.win_error=GetLastError();
        InterlockedCompareExchange(iat,(LONG)(uintptr_t)original,(LONG)(uintptr_t)capture_create9);
        VirtualProtect(iat,4,protection,&ignored);
        attempt.status=4;attempt.reason=23;goto done;
    }
    attempt.status=1;attempt.reason=1;
done:
    AcquireSRWLockExclusive(&capture_lock);
    capture_metrics.hook_status=attempt.status;capture_metrics.phase=phase;capture_metrics.attempts++;
    if (phase==1) {capture_metrics.initial=attempt;capture_metrics.initial_paths=paths;}
    capture_metrics.latest=attempt;capture_metrics.latest_paths=paths;capture_publish_locked();
    ReleaseSRWLockExclusive(&capture_lock);
    SetLastError(saved_error);
}
static volatile LONG capture_retry_called;
static void capture_retry_import(void) {
    DWORD error=GetLastError();
    if (InterlockedCompareExchange(&capture_retry_called,1,0)!=0) {SetLastError(error);return;}
    AcquireSRWLockExclusive(&capture_lock);
    uint32_t status=capture_metrics.hook_status;
    ReleaseSRWLockExclusive(&capture_lock);
    /* One API-boundary retry only. Never retry a ready or protection-failed
       hook, force device recreation, or search for an existing device. */
    if (status==2 || status==3) capture_install_import(2);
    SetLastError(error);
}
