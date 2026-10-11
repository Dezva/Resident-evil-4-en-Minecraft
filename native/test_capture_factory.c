/* Typed fake COM API. Declarations follow the public IDirect3D9 contract.
 * Tests include capture_factory.h itself, not a replacement implementation. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define WINAPI
#define HEAP_ZERO_MEMORY 8
#define S_OK 0
#define E_POINTER ((int32_t)0x80004003)
#define E_NOINTERFACE ((int32_t)0x80004002)
#define SUCCEEDED(x) ((x)>=0)
typedef int32_t HRESULT,LONG;typedef uint32_t ULONG,UINT,DWORD;typedef int BOOL,WINBOOL;
typedef int D3DFORMAT,D3DDEVTYPE,D3DRESOURCETYPE,D3DMULTISAMPLE_TYPE;
typedef void *HWND;typedef void *HMONITOR;
typedef struct {uint32_t Data1;uint16_t Data2,Data3;unsigned char Data4[8];} GUID;
typedef const GUID *REFIID;
typedef struct {int value;} D3DADAPTER_IDENTIFIER9,D3DDISPLAYMODE,D3DCAPS9;
typedef struct {UINT BackBufferWidth,BackBufferHeight;BOOL Windowed;} D3DPRESENT_PARAMETERS;
typedef struct IDirect3D9 IDirect3D9;
typedef struct IDirect3DDevice9 {const void *lpVtbl;unsigned original_data;} IDirect3DDevice9;
typedef struct IDirect3D9Vtbl {
    HRESULT (*QueryInterface)(IDirect3D9 *self, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IDirect3D9 *self);
    ULONG (*Release)(IDirect3D9 *self);
    HRESULT (*RegisterSoftwareDevice)(IDirect3D9 *self, void* pInitializeFunction);
    UINT (*GetAdapterCount)(IDirect3D9 *self);
    HRESULT (*GetAdapterIdentifier)(IDirect3D9 *self, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
    UINT (*GetAdapterModeCount)(IDirect3D9 *self, UINT Adapter, D3DFORMAT Format);
    HRESULT (*EnumAdapterModes)(IDirect3D9 *self, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode);
    HRESULT (*GetAdapterDisplayMode)(IDirect3D9 *self, UINT Adapter, D3DDISPLAYMODE* pMode);
    HRESULT (*CheckDeviceType)(IDirect3D9 *self, UINT iAdapter, D3DDEVTYPE DevType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, WINBOOL bWindowed);
    HRESULT (*CheckDeviceFormat)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    HRESULT (*CheckDeviceMultiSampleType)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, WINBOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels);
    HRESULT (*CheckDepthStencilMatch)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    HRESULT (*CheckDeviceFormatConversion)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);
    HRESULT (*GetDeviceCaps)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps);
    HMONITOR (*GetAdapterMonitor)(IDirect3D9 *self, UINT Adapter);
    HRESULT (*CreateDevice)(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, struct IDirect3DDevice9** ppReturnedDeviceInterface);
} IDirect3D9Vtbl;
struct IDirect3D9 {IDirect3D9Vtbl *lpVtbl;};
static IDirect3D9 inner;
static IDirect3DDevice9 device={(void *)0xabc,123};
static unsigned inner_refs,adds,releases,live,allocation_fail,allocated,freed,wrapping_fail,escapes;
static unsigned factory_calls,factory_null,device_calls,device_fail,recorded_success,forwarded;
static DWORD last_error;
static void *GetProcessHeap(void) {return (void *)1;}
static void *HeapAlloc(void *heap,DWORD flags,size_t bytes) {assert(heap==(void *)1 && flags==8);if (allocation_fail)return NULL;allocated++;return calloc(1,bytes);}
static int HeapFree(void *heap,DWORD flags,void *p) {assert(heap==(void *)1 && !flags);freed++;free(p);last_error=777;return 1;}
static LONG InterlockedIncrement(volatile LONG *v) {return ++*v;}
static LONG InterlockedDecrement(volatile LONG *v) {return --*v;}
static DWORD GetLastError(void) {return last_error;}
static void SetLastError(DWORD v) {last_error=v;}
static void capture_record_wrap(int delta,int failed) {live=(unsigned)((int)live+delta);wrapping_fail+=(unsigned)failed;}
static void capture_record_factory(IDirect3D9 *f) {assert(f==NULL || f==&inner);factory_calls++;last_error=888;}
static void capture_record_escape(void) {escapes++;}
static void capture_record_device(HRESULT hr,IDirect3DDevice9 *d,UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS *pp) {
 assert(adapter==2 && type==3 && window==(void *)4 && flags==5);
 assert(SUCCEEDED(hr)?d==&device && pp && pp->Windowed:d==NULL && pp==NULL);
 if (d) recorded_success++;
 last_error=999;
}
static HRESULT fake_query(IDirect3D9 *self,REFIID iid,void **out) {
 assert(self==&inner);assert(iid);if (iid->Data1==42) {*out=(void *)0x1234;return 37;}*out=NULL;return E_NOINTERFACE;
}
static ULONG fake_addref(IDirect3D9 *self) {assert(self==&inner);adds++;return ++inner_refs;}
static ULONG fake_release(IDirect3D9 *self) {assert(self==&inner && inner_refs);releases++;return --inner_refs;}
static HRESULT fake_device(IDirect3D9 *self,UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS *pp,IDirect3DDevice9 **out) {
 assert(self==&inner && adapter==2 && type==3 && window==(void *)4 && flags==5);
 device_calls++;last_error=444;
 if (device_fail) return E_POINTER; // Do not read or write invalid failed output.
 assert(pp && out);pp->BackBufferWidth=640;pp->BackBufferHeight=480;*out=&device;
 inner_refs++;return 29; // Device retains the native factory, not our facade.
}
static IDirect3D9 *fake_create(UINT sdk) {assert(sdk==32);last_error=321;return factory_null?NULL:&inner;}
static IDirect3D9 *(*capture_original_create9)(UINT)=fake_create;
static HRESULT fake_RegisterSoftwareDevice(IDirect3D9 *self, void* pInitializeFunction) {assert(self==&inner);assert(pInitializeFunction==(void*)(uintptr_t)65920);forwarded++;return (HRESULT)(uintptr_t)43779;}
static UINT fake_GetAdapterCount(IDirect3D9 *self) {assert(self==&inner);forwarded++;return (UINT)(uintptr_t)43780;}
static HRESULT fake_GetAdapterIdentifier(IDirect3D9 *self, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66176);assert(Flags==(DWORD)(uintptr_t)66184);assert(pIdentifier==(D3DADAPTER_IDENTIFIER9*)(uintptr_t)66192);forwarded++;return (HRESULT)(uintptr_t)43781;}
static UINT fake_GetAdapterModeCount(IDirect3D9 *self, UINT Adapter, D3DFORMAT Format) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66304);assert(Format==(D3DFORMAT)(uintptr_t)66312);forwarded++;return (UINT)(uintptr_t)43782;}
static HRESULT fake_EnumAdapterModes(IDirect3D9 *self, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66432);assert(Format==(D3DFORMAT)(uintptr_t)66440);assert(Mode==(UINT)(uintptr_t)66448);assert(pMode==(D3DDISPLAYMODE*)(uintptr_t)66456);forwarded++;return (HRESULT)(uintptr_t)43783;}
static HRESULT fake_GetAdapterDisplayMode(IDirect3D9 *self, UINT Adapter, D3DDISPLAYMODE* pMode) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66560);assert(pMode==(D3DDISPLAYMODE*)(uintptr_t)66568);forwarded++;return (HRESULT)(uintptr_t)43784;}
static HRESULT fake_CheckDeviceType(IDirect3D9 *self, UINT iAdapter, D3DDEVTYPE DevType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, WINBOOL bWindowed) {assert(self==&inner);assert(iAdapter==(UINT)(uintptr_t)66688);assert(DevType==(D3DDEVTYPE)(uintptr_t)66696);assert(DisplayFormat==(D3DFORMAT)(uintptr_t)66704);assert(BackBufferFormat==(D3DFORMAT)(uintptr_t)66712);assert(bWindowed==(WINBOOL)(uintptr_t)66720);forwarded++;return (HRESULT)(uintptr_t)43785;}
static HRESULT fake_CheckDeviceFormat(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66816);assert(DeviceType==(D3DDEVTYPE)(uintptr_t)66824);assert(AdapterFormat==(D3DFORMAT)(uintptr_t)66832);assert(Usage==(DWORD)(uintptr_t)66840);assert(RType==(D3DRESOURCETYPE)(uintptr_t)66848);assert(CheckFormat==(D3DFORMAT)(uintptr_t)66856);forwarded++;return (HRESULT)(uintptr_t)43786;}
static HRESULT fake_CheckDeviceMultiSampleType(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, WINBOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)66944);assert(DeviceType==(D3DDEVTYPE)(uintptr_t)66952);assert(SurfaceFormat==(D3DFORMAT)(uintptr_t)66960);assert(Windowed==(WINBOOL)(uintptr_t)66968);assert(MultiSampleType==(D3DMULTISAMPLE_TYPE)(uintptr_t)66976);assert(pQualityLevels==(DWORD*)(uintptr_t)66984);forwarded++;return (HRESULT)(uintptr_t)43787;}
static HRESULT fake_CheckDepthStencilMatch(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)67072);assert(DeviceType==(D3DDEVTYPE)(uintptr_t)67080);assert(AdapterFormat==(D3DFORMAT)(uintptr_t)67088);assert(RenderTargetFormat==(D3DFORMAT)(uintptr_t)67096);assert(DepthStencilFormat==(D3DFORMAT)(uintptr_t)67104);forwarded++;return (HRESULT)(uintptr_t)43788;}
static HRESULT fake_CheckDeviceFormatConversion(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)67200);assert(DeviceType==(D3DDEVTYPE)(uintptr_t)67208);assert(SourceFormat==(D3DFORMAT)(uintptr_t)67216);assert(TargetFormat==(D3DFORMAT)(uintptr_t)67224);forwarded++;return (HRESULT)(uintptr_t)43789;}
static HRESULT fake_GetDeviceCaps(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)67328);assert(DeviceType==(D3DDEVTYPE)(uintptr_t)67336);assert(pCaps==(D3DCAPS9*)(uintptr_t)67344);forwarded++;return (HRESULT)(uintptr_t)43790;}
static HMONITOR fake_GetAdapterMonitor(IDirect3D9 *self, UINT Adapter) {assert(self==&inner);assert(Adapter==(UINT)(uintptr_t)67456);forwarded++;return (HMONITOR)(uintptr_t)43791;}
static IDirect3D9Vtbl table={fake_query,fake_addref,fake_release,fake_RegisterSoftwareDevice,fake_GetAdapterCount,fake_GetAdapterIdentifier,fake_GetAdapterModeCount,fake_EnumAdapterModes,fake_GetAdapterDisplayMode,fake_CheckDeviceType,fake_CheckDeviceFormat,fake_CheckDeviceMultiSampleType,fake_CheckDepthStencilMatch,fake_CheckDeviceFormatConversion,fake_GetDeviceCaps,fake_GetAdapterMonitor,fake_device};

static unsigned cube_wrap_calls,cube_wrap_output;
static IDirect3DDevice9 cube_output;
static IDirect3DDevice9 *cube_wrap_device(IDirect3DDevice9 *d) {assert(d==&device);cube_wrap_calls++;last_error=555;return cube_wrap_output?&cube_output:d;}
#include "capture_factory.h"
static void reset(void) {
 inner.lpVtbl=&table;inner_refs=1;adds=releases=live=allocation_fail=allocated=freed=wrapping_fail=escapes=0;
 factory_calls=factory_null=device_calls=device_fail=recorded_success=forwarded=0;last_error=0;
 cube_wrap_calls=cube_wrap_output=0;
}
int main(void) {
 reset();IDirect3D9 *w=capture_create9(32);assert(w && w!=&inner && allocated==1 && live==1 && inner_refs==1 && last_error==321);
 void *identity=NULL;
 assert(w->lpVtbl->QueryInterface(w,&capture_iunknown,&identity)==S_OK && identity==w && inner_refs==2);
 assert(w->lpVtbl->Release(w)==1 && inner_refs==1);
 assert(w->lpVtbl->QueryInterface(w,&capture_iid9,&identity)==S_OK && identity==w);
 assert(w->lpVtbl->Release(w)==1);
 assert(w->lpVtbl->QueryInterface(w,&capture_iid9,NULL)==E_POINTER);
 GUID unknown={42,0,0,{0}};assert(w->lpVtbl->QueryInterface(w,&unknown,&identity)==37 && identity==(void *)0x1234 && escapes==1);
 unknown.Data1=43;assert(w->lpVtbl->QueryInterface(w,&unknown,&identity)==E_NOINTERFACE && !identity && escapes==1);
 assert(w->lpVtbl->RegisterSoftwareDevice(w, (void*)(uintptr_t)65920)==(HRESULT)(uintptr_t)43779);
 assert(w->lpVtbl->GetAdapterCount(w)==(UINT)(uintptr_t)43780);
 assert(w->lpVtbl->GetAdapterIdentifier(w, (UINT)(uintptr_t)66176, (DWORD)(uintptr_t)66184, (D3DADAPTER_IDENTIFIER9*)(uintptr_t)66192)==(HRESULT)(uintptr_t)43781);
 assert(w->lpVtbl->GetAdapterModeCount(w, (UINT)(uintptr_t)66304, (D3DFORMAT)(uintptr_t)66312)==(UINT)(uintptr_t)43782);
 assert(w->lpVtbl->EnumAdapterModes(w, (UINT)(uintptr_t)66432, (D3DFORMAT)(uintptr_t)66440, (UINT)(uintptr_t)66448, (D3DDISPLAYMODE*)(uintptr_t)66456)==(HRESULT)(uintptr_t)43783);
 assert(w->lpVtbl->GetAdapterDisplayMode(w, (UINT)(uintptr_t)66560, (D3DDISPLAYMODE*)(uintptr_t)66568)==(HRESULT)(uintptr_t)43784);
 assert(w->lpVtbl->CheckDeviceType(w, (UINT)(uintptr_t)66688, (D3DDEVTYPE)(uintptr_t)66696, (D3DFORMAT)(uintptr_t)66704, (D3DFORMAT)(uintptr_t)66712, (WINBOOL)(uintptr_t)66720)==(HRESULT)(uintptr_t)43785);
 assert(w->lpVtbl->CheckDeviceFormat(w, (UINT)(uintptr_t)66816, (D3DDEVTYPE)(uintptr_t)66824, (D3DFORMAT)(uintptr_t)66832, (DWORD)(uintptr_t)66840, (D3DRESOURCETYPE)(uintptr_t)66848, (D3DFORMAT)(uintptr_t)66856)==(HRESULT)(uintptr_t)43786);
 assert(w->lpVtbl->CheckDeviceMultiSampleType(w, (UINT)(uintptr_t)66944, (D3DDEVTYPE)(uintptr_t)66952, (D3DFORMAT)(uintptr_t)66960, (WINBOOL)(uintptr_t)66968, (D3DMULTISAMPLE_TYPE)(uintptr_t)66976, (DWORD*)(uintptr_t)66984)==(HRESULT)(uintptr_t)43787);
 assert(w->lpVtbl->CheckDepthStencilMatch(w, (UINT)(uintptr_t)67072, (D3DDEVTYPE)(uintptr_t)67080, (D3DFORMAT)(uintptr_t)67088, (D3DFORMAT)(uintptr_t)67096, (D3DFORMAT)(uintptr_t)67104)==(HRESULT)(uintptr_t)43788);
 assert(w->lpVtbl->CheckDeviceFormatConversion(w, (UINT)(uintptr_t)67200, (D3DDEVTYPE)(uintptr_t)67208, (D3DFORMAT)(uintptr_t)67216, (D3DFORMAT)(uintptr_t)67224)==(HRESULT)(uintptr_t)43789);
 assert(w->lpVtbl->GetDeviceCaps(w, (UINT)(uintptr_t)67328, (D3DDEVTYPE)(uintptr_t)67336, (D3DCAPS9*)(uintptr_t)67344)==(HRESULT)(uintptr_t)43790);
 assert(w->lpVtbl->GetAdapterMonitor(w, (UINT)(uintptr_t)67456)==(HMONITOR)(uintptr_t)43791);
 assert(forwarded==13);
 D3DPRESENT_PARAMETERS pp={100,200,1};IDirect3DDevice9 *out=NULL;
 IDirect3DDevice9 before=device;
 assert(w->lpVtbl->CreateDevice(w,2,3,(void *)4,5,&pp,&out)==29);
 assert(out==&device && recorded_success==1 && device_calls==1 && last_error==444);
 assert(!memcmp(&before,&device,sizeof(device)) && pp.BackBufferWidth==640);
 assert(w->lpVtbl->Release(w)==0 && !live && freed==1 && inner_refs==1 && last_error==444);
 // Native device still owns its native factory after our facade dies.
 assert(inner.lpVtbl->Release(&inner)==0);
 reset();allocation_fail=1;w=capture_create9(32);assert(w==&inner && wrapping_fail==1 && !live && !freed && inner_refs==1 && last_error==321);
 reset();factory_null=1;assert(!capture_create9(32) && factory_calls==1 && !allocated && !freed && last_error==321);
 reset();w=capture_create9(32);device_fail=1;
 // Addresses intentionally invalid: no success means we must not touch them.
 assert(w->lpVtbl->CreateDevice(w,2,3,(void *)4,5,(void *)1,(void *)1)==E_POINTER);
 assert(!recorded_success && device_calls==1 && last_error==444);
 assert(!cube_wrap_calls);
 assert(w->lpVtbl->Release(w)==0 && inner_refs==0 && allocated==freed);
 reset();cube_wrap_output=1;w=capture_create9(32);out=NULL;
 assert(w->lpVtbl->CreateDevice(w,2,3,(void *)4,5,&pp,&out)==29 && out==&cube_output && cube_wrap_calls==1 && last_error==444);
 assert(!memcmp(&before,&device,sizeof(device)));
 assert(w->lpVtbl->Release(w)==0 && inner_refs==1);assert(inner.lpVtbl->Release(&inner)==0);
 puts("Factory capture checks passed: all 17 methods, identity/refcounts, failed allocation/factory/device calls, unchanged device and native factory lifetime");
}
