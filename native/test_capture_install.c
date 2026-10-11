/* The Python runner inserts the ACTUAL Windows import installer below. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "capture_import.h"
#define WINAPI
#define MAX_PATH 260
#define PAGE_READWRITE 4
#define PAGE_EXECUTE_READWRITE 0x40
typedef int32_t LONG;typedef uint32_t DWORD,UINT;
typedef void *HMODULE;typedef void (*FARPROC)(void);
typedef struct {DWORD Protect;} MEMORY_BASIC_INFORMATION;
typedef struct IDirect3D9 {unsigned unused;} IDirect3D9;
static unsigned char image[65536];
static unsigned missing,foreign,protect_calls,protect_fail,restore_fail,cas_changed;
static unsigned native_system,query_fail;
static unsigned loaded_system,case_variant,truncated,system_fail,wow64_too_long;
static DWORD last_error,protection;
typedef struct {uint32_t matched;wchar_t loaded[MAX_PATH],wow64[MAX_PATH],system[MAX_PATH];} CapturePaths;
static struct {unsigned hook_status,phase,attempts;CaptureAttempt initial,latest;CapturePaths initial_paths,latest_paths;} capture_metrics;
static unsigned capture_lock,lock_depth;
static void AcquireSRWLockExclusive(unsigned *lock) {assert(lock==&capture_lock && !lock_depth);lock_depth=1;}
static void ReleaseSRWLockExclusive(unsigned *lock) {assert(lock==&capture_lock && lock_depth==1);lock_depth=0;}
static void capture_publish_locked(void) {assert(lock_depth==1);}
static IDirect3D9 *(WINAPI *capture_original_create9)(UINT);
static IDirect3D9 *WINAPI original_create(UINT sdk) {(void)sdk;assert(0);return NULL;}
static IDirect3D9 *WINAPI capture_create9(UINT sdk) {(void)sdk;assert(0);return NULL;}
static DWORD GetLastError(void) {return last_error;}
static void SetLastError(DWORD value) {last_error=value;}
static int readable(const void *p,size_t n) {
    uintptr_t offset=(uintptr_t)p-(uintptr_t)image;
    return offset<=sizeof(image) && n<=sizeof(image)-offset;
}
static HMODULE GetModuleHandleW(const wchar_t *name) {
    if (!name) return image;
    assert(!wcscmp(name,L"d3d9.dll"));return missing?NULL:(void *)42;
}
static DWORD GetModuleFileNameW(HMODULE module,wchar_t *out,DWORD n) {
    assert(module==(void *)42 && n==MAX_PATH);
    wcscpy(out,foreign==1?L"D:\\Games\\d3d9.dll":foreign==2?L"C:\\Windows\\System32\\other\\d3d9.dll":(native_system==1 || loaded_system)?L"C:\\Windows\\System32\\d3d9.dll":L"C:\\Windows\\SysWOW64\\d3d9.dll");
    if (case_variant) for (unsigned i=0;out[i];++i) out[i]=(wchar_t)towupper(out[i]);
    return truncated?MAX_PATH:(DWORD)wcslen(out);
}
static UINT GetSystemWow64DirectoryW(wchar_t *out,UINT n) {
    assert(n==MAX_PATH);if (native_system) return 0;wcscpy(out,L"C:\\Windows\\SysWOW64");return wow64_too_long?MAX_PATH:(UINT)wcslen(out);
}
static UINT GetSystemDirectoryW(wchar_t *out,UINT n) {
    assert(n==MAX_PATH);if (system_fail) return 0;wcscpy(out,L"C:\\Windows\\System32");return (UINT)wcslen(out);
}
static int lstrcmpiW(const wchar_t *a,const wchar_t *b) {
    for (;*a && *b;++a,++b) if (towlower(*a)!=towlower(*b)) return 1;
    return *a!=*b;
}
static FARPROC GetProcAddress(HMODULE module,const char *name) {
    assert(module==(void *)42 && !strcmp(name,"Direct3DCreate9"));return (FARPROC)original_create;
}
static int VirtualQuery(void *slot,MEMORY_BASIC_INFORMATION *region,size_t size) {
    assert(slot==image+0x700 && size==sizeof(*region));region->Protect=protection;return !query_fail;
}
static int VirtualProtect(void *p,size_t n,DWORD next,DWORD *old) {
    assert(p==image+0x700 && n==4);protect_calls++;last_error=123;
    if (protect_calls==1 && (protection&0xf0)) assert(next==PAGE_EXECUTE_READWRITE);
    if (protect_calls==1 && protect_fail) return 0;
    if (protect_calls==2 && restore_fail) return 0;
    *old=protection;protection=next;return 1;
}
static LONG InterlockedCompareExchange(volatile LONG *slot,LONG next,LONG expected) {
    if (cas_changed && slot==(LONG *)(image+0x700)) {*slot=17;cas_changed=0;}
    LONG old=*slot;if (old==expected) *slot=next;return old;
}
/* ACTUAL_INSTALL */
static void word(uint32_t offset,uint16_t n) {memcpy(image+offset,&n,2);}
static void dword(uint32_t offset,uint32_t n) {memcpy(image+offset,&n,4);}
static void setup(void) {
    memset(image,0,sizeof(image));missing=foreign=protect_calls=protect_fail=restore_fail=cas_changed=0;
    native_system=query_fail=0;
    loaded_system=case_variant=truncated=system_fail=wow64_too_long=0;
    capture_original_create9=NULL;memset(&capture_metrics,0,sizeof(capture_metrics));capture_retry_called=0;lock_depth=0;last_error=777;protection=2;
    word(0,0x5a4d);dword(0x3c,0x80);dword(0x80,0x4550);word(0x84,0x14c);word(0x94,224);word(0x98,0x10b);
    dword(0xd0,sizeof(image));dword(0xf4,16);dword(0x100,0x400);dword(0x104,40);
    dword(0x400,0x600);dword(0x40c,0x500);dword(0x410,0x700);strcpy((char *)image+0x500,"d3d9.dll");
    dword(0x600,0x800);dword(0x700,(uint32_t)(uintptr_t)original_create);strcpy((char *)image+0x802,"Direct3DCreate9");
}
int main(void) {
    setup();capture_install_import(1);
    assert(capture_metrics.hook_status==1 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)capture_create9);
    assert(capture_original_create9==original_create && protection==2 && last_error==777);
    assert(capture_metrics.initial_paths.matched==1 && !wcscmp(capture_metrics.initial_paths.loaded,L"C:\\Windows\\SysWOW64\\d3d9.dll"));
    setup();loaded_system=1;capture_install_import(1);assert(capture_metrics.hook_status==1 && capture_metrics.initial_paths.matched==2);
    setup();loaded_system=case_variant=1;capture_install_import(1);assert(capture_metrics.hook_status==1 && capture_metrics.latest_paths.matched==2);
    setup();truncated=1;capture_install_import(1);assert(capture_metrics.latest.reason==6 && !protect_calls);
    setup();wow64_too_long=loaded_system=1;capture_install_import(1);assert(capture_metrics.hook_status==1 && capture_metrics.latest_paths.wow64[0]==0);
    setup();native_system=system_fail=1;capture_install_import(1);assert(capture_metrics.latest.reason==6 && !protect_calls);
    setup();missing=1;capture_install_import(1);assert(capture_metrics.hook_status==2 && !protect_calls && last_error==777);
    setup();foreign=1;capture_install_import(1);assert(capture_metrics.hook_status==3 && !protect_calls && capture_metrics.initial_paths.matched==0);
    assert(!wcscmp(capture_metrics.initial_paths.loaded,L"D:\\Games\\d3d9.dll"));
    setup();foreign=2;capture_install_import(1);assert(capture_metrics.latest.reason==7 && !protect_calls);
    setup();dword(0x700,42);capture_install_import(1);assert(capture_metrics.hook_status==3 && !protect_calls);
    setup();dword(0x3c,0xfffffff0);capture_install_import(1);assert(capture_metrics.hook_status==3 && !protect_calls);
    setup();protect_fail=1;capture_install_import(1);assert(capture_metrics.hook_status==4 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)original_create);
    setup();restore_fail=1;capture_install_import(1);assert(capture_metrics.hook_status==4 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)original_create && protection==2 && last_error==777);
    setup();cas_changed=1;capture_install_import(1);assert(capture_metrics.hook_status==3 && capture_u32(image+0x700)==17 && protection==2);
    setup();protection=0x20;capture_install_import(1);assert(capture_metrics.hook_status==1 && protection==0x20);
    setup();query_fail=1;capture_install_import(1);assert(capture_metrics.hook_status==4 && !protect_calls);
    setup();native_system=1;capture_install_import(1);assert(capture_metrics.hook_status==1 && protection==2);
    setup();native_system=2;capture_install_import(1);assert(capture_metrics.hook_status==3 && !protect_calls); // SysWOW64 table cannot be accepted as a System32 import.
    setup();dword(0x700,0x800);capture_install_import(1);
    assert(capture_metrics.initial.reason==15 && capture_metrics.initial.observed==0x800 && capture_metrics.attempts==1);
    dword(0x700,(uint32_t)(uintptr_t)original_create);capture_retry_import();
    assert(capture_metrics.hook_status==1 && capture_metrics.phase==2 && capture_metrics.attempts==2 && capture_metrics.latest.reason==1);
    assert(capture_metrics.initial.reason==15 && capture_metrics.initial.observed==0x800 && last_error==777);
    unsigned protections=protect_calls;capture_retry_import();assert(protect_calls==protections && capture_metrics.attempts==2);
    setup();capture_install_import(1);capture_retry_import();assert(capture_metrics.attempts==1 && capture_metrics.phase==1);
    setup();protect_fail=1;capture_install_import(1);protect_fail=0;capture_retry_import();assert(capture_metrics.hook_status==4 && capture_metrics.attempts==1);
    setup();missing=1;capture_install_import(1);missing=0;capture_retry_import();assert(capture_metrics.initial.reason==5 && capture_metrics.latest.status==1);
    setup();dword(0x700,42);capture_install_import(1);capture_retry_import();assert(capture_metrics.hook_status==3 && capture_metrics.attempts==2 && capture_metrics.latest.reason==15);
    puts("Actual capture installer checks passed: system module and unchanged unique import, original target before patch, conflict refusal, protection rollback and preserved LastError; no COM invoked during install");
}
