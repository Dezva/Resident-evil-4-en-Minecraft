/* The Python runner inserts the ACTUAL Windows import installer below. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
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
static DWORD last_error,protection;
static struct {unsigned hook_status;} capture_metrics;
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
    wcscpy(out,foreign?L"D:\\Games\\d3d9.dll":native_system==1?L"C:\\Windows\\System32\\d3d9.dll":L"C:\\Windows\\SysWOW64\\d3d9.dll");return (DWORD)wcslen(out);
}
static UINT GetSystemWow64DirectoryW(wchar_t *out,UINT n) {
    assert(n==MAX_PATH);if (native_system) return 0;wcscpy(out,L"C:\\Windows\\SysWOW64");return (UINT)wcslen(out);
}
static UINT GetSystemDirectoryW(wchar_t *out,UINT n) {
    assert(n==MAX_PATH);wcscpy(out,L"C:\\Windows\\System32");return (UINT)wcslen(out);
}
static int lstrcmpiW(const wchar_t *a,const wchar_t *b) {return wcscmp(a,b);}
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
static LONG InterlockedCompareExchange(LONG *slot,LONG next,LONG expected) {
    if (cas_changed) {*slot=17;cas_changed=0;}
    LONG old=*slot;if (old==expected) *slot=next;return old;
}
/* ACTUAL_INSTALL */
static void word(uint32_t offset,uint16_t n) {memcpy(image+offset,&n,2);}
static void dword(uint32_t offset,uint32_t n) {memcpy(image+offset,&n,4);}
static void setup(void) {
    memset(image,0,sizeof(image));missing=foreign=protect_calls=protect_fail=restore_fail=cas_changed=0;
    native_system=query_fail=0;
    capture_original_create9=NULL;capture_metrics.hook_status=0;last_error=777;protection=2;
    word(0,0x5a4d);dword(0x3c,0x80);dword(0x80,0x4550);word(0x84,0x14c);word(0x94,224);word(0x98,0x10b);
    dword(0xd0,sizeof(image));dword(0xf4,16);dword(0x100,0x400);dword(0x104,40);
    dword(0x400,0x600);dword(0x40c,0x500);dword(0x410,0x700);strcpy((char *)image+0x500,"d3d9.dll");
    dword(0x600,0x800);dword(0x700,(uint32_t)(uintptr_t)original_create);strcpy((char *)image+0x802,"Direct3DCreate9");
}
int main(void) {
    setup();capture_install_import();
    assert(capture_metrics.hook_status==1 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)capture_create9);
    assert(capture_original_create9==original_create && protection==2 && last_error==777);
    setup();missing=1;capture_install_import();assert(capture_metrics.hook_status==2 && !protect_calls && last_error==777);
    setup();foreign=1;capture_install_import();assert(capture_metrics.hook_status==3 && !protect_calls);
    setup();dword(0x700,42);capture_install_import();assert(capture_metrics.hook_status==3 && !protect_calls);
    setup();dword(0x3c,0xfffffff0);capture_install_import();assert(capture_metrics.hook_status==3 && !protect_calls);
    setup();protect_fail=1;capture_install_import();assert(capture_metrics.hook_status==4 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)original_create);
    setup();restore_fail=1;capture_install_import();assert(capture_metrics.hook_status==4 && capture_u32(image+0x700)==(uint32_t)(uintptr_t)original_create && protection==2 && last_error==777);
    setup();cas_changed=1;capture_install_import();assert(capture_metrics.hook_status==3 && capture_u32(image+0x700)==17 && protection==2);
    setup();protection=0x20;capture_install_import();assert(capture_metrics.hook_status==1 && protection==0x20);
    setup();query_fail=1;capture_install_import();assert(capture_metrics.hook_status==4 && !protect_calls);
    setup();native_system=1;capture_install_import();assert(capture_metrics.hook_status==1 && protection==2);
    setup();native_system=2;capture_install_import();assert(capture_metrics.hook_status==3 && !protect_calls); // SysWOW64 table cannot be accepted as a System32 import.
    puts("Actual capture installer checks passed: system module and unchanged unique import, original target before patch, conflict refusal, protection rollback and preserved LastError; no COM invoked during install");
}
