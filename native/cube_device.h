/* Typed external facade. Native device and native vtable are never modified.
 * Owned native references mirror facade references, including its initial reference. */
#include "cube_policy.h"
#include "block_geometry.h"
#include "block_texture.h"
typedef struct {
    IDirect3DDevice9 iface;IDirect3DDevice9 *inner;
    volatile LONG references;CRITICAL_SECTION lock;
    unsigned method_depth,deferred_releases;
    CubeGate gate;unsigned scene,escaped,resetting;uint32_t player,hud_generation,hud_blocked_generation;
    IDirect3DTexture9 *hud_texture;unsigned hud_pixels_valid;uint32_t hud_pixels[256*32];
    BlockTextureCache block_texture;
} CubeDevice;
static const GUID cube_iunknown={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID cube_iid_device={0xd0223b96,0xbf7a,0x43fd,{0x92,0xbd,0xa4,0x3b,0x0d,0x82,0xb9,0xeb}};
#ifndef CUBE_TEST_DRAW_STUB
#include "cube_draw.h"
#include "hud_draw.h"
#else
static void cube_render_device(CubeDevice *f);
static void hud_render_device(CubeDevice *f);
static void hud_reset_device(CubeDevice *f);
static void hud_release_texture(CubeDevice *f);
#endif
/* Keep the facade alive across native callbacks. Client native releases during
   a call are deferred until the outer call returns; no COM AddRef/Release pair
   is needed on each of the game's ordinary drawing/state calls. */
static void cube_method_enter(CubeDevice *f) {
    EnterCriticalSection(&f->lock);
    InterlockedIncrement(&f->references);f->method_depth++;
}
static void cube_method_leave(CubeDevice *f,DWORD error) {
    LONG refs=InterlockedDecrement(&f->references);f->method_depth--;
    if(!refs){hud_release_texture(f);block_texture_cache_release(&f->block_texture);}
    if(!f->method_depth){
        unsigned pending=f->deferred_releases;f->deferred_releases=0;
        while(pending--)f->inner->lpVtbl->Release(f->inner);
    }
    LeaveCriticalSection(&f->lock);
    if (!refs) {DeleteCriticalSection(&f->lock);cube_record_wrap(-1,0);HeapFree(GetProcessHeap(),0,f);}
    SetLastError(error);
}
static ULONG WINAPI cube_AddRef(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;EnterCriticalSection(&f->lock);
    f->inner->lpVtbl->AddRef(f->inner);LONG refs=InterlockedIncrement(&f->references);
    DWORD error=GetLastError();LeaveCriticalSection(&f->lock);SetLastError(error);return (ULONG)refs;
}
static ULONG WINAPI cube_Release(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;EnterCriticalSection(&f->lock);
    LONG refs=InterlockedDecrement(&f->references);
    if(f->method_depth)f->deferred_releases++;
    else {if(!refs){hud_release_texture(f);block_texture_cache_release(&f->block_texture);}f->inner->lpVtbl->Release(f->inner);}
    DWORD error=GetLastError();LeaveCriticalSection(&f->lock);
    if (!refs) {DeleteCriticalSection(&f->lock);cube_record_wrap(-1,0);HeapFree(GetProcessHeap(),0,f);}
    SetLastError(error);return (ULONG)refs;
}
static HRESULT WINAPI cube_QueryInterface(IDirect3DDevice9 *iface,REFIID iid,void **out) {
    CubeDevice *f=(CubeDevice *)iface;if (!out) return E_POINTER;
    if (iid && (!memcmp(iid,&cube_iunknown,sizeof(GUID)) || !memcmp(iid,&cube_iid_device,sizeof(GUID)))) {
        cube_AddRef(iface);*out=iface;return S_OK;
    }
    cube_method_enter(f);
    HRESULT hr=f->inner->lpVtbl->QueryInterface(f->inner,iid,out);DWORD error=GetLastError();
    if (SUCCEEDED(hr) && *out) {f->escaped=1;hud_release_texture(f);block_texture_cache_release(&f->block_texture);cube_record(CUBE_ESCAPE,0);}
    cube_method_leave(f,error);return hr;
}
static HRESULT WINAPI cube_BeginScene(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT hr=f->inner->lpVtbl->BeginScene(f->inner);DWORD error=GetLastError();
    if (SUCCEEDED(hr)) f->scene=1;
    cube_record(CUBE_BEGIN,0);cube_method_leave(f,error);return hr;
}
static HRESULT WINAPI cube_EndScene(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    cube_record(CUBE_FRAME,0);cube_render_device(f);hud_render_device(f);f->scene=0;
    HRESULT hr=f->inner->lpVtbl->EndScene(f->inner);DWORD error=GetLastError();
    if (FAILED(hr)) {hud_reset_device(f);block_texture_cache_release(&f->block_texture);f->gate.lost=1;f->gate.blocked=f->gate.epoch;f->gate.blocked_reason=CUBE_LOST;cube_record(CUBE_ACTIVE,0);}
    cube_method_leave(f,error);return hr;
}
static HRESULT WINAPI cube_Reset(IDirect3DDevice9 *iface,D3DPRESENT_PARAMETERS *parameters) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    hud_reset_device(f);block_texture_cache_release(&f->block_texture);f->scene=0;f->resetting=1;f->gate.anchored=0;f->gate.blocked=f->gate.epoch;f->gate.blocked_reason=CUBE_LOST;cube_record(CUBE_RESET,0);
    HRESULT hr=f->inner->lpVtbl->Reset(f->inner,parameters);DWORD error=GetLastError();
    f->resetting=0;f->gate.lost=FAILED(hr);if (FAILED(hr)) cube_record(CUBE_RESET_FAILURE,(uint32_t)hr);
    cube_method_leave(f,error);return hr;
}
static HRESULT WINAPI cube_Present(IDirect3DDevice9 *iface,const RECT *src,const RECT *dst,HWND window,const RGNDATA *dirty) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT hr=f->inner->lpVtbl->Present(f->inner,src,dst,window,dirty);DWORD error=GetLastError();
    if (FAILED(hr)) {hud_reset_device(f);block_texture_cache_release(&f->block_texture);f->gate.lost=1;f->gate.blocked=f->gate.epoch;f->gate.blocked_reason=CUBE_LOST;cube_record(CUBE_PRESENT_FAILURE,(uint32_t)hr);}
    cube_method_leave(f,error);return hr;
}
#include "device_forwarders.h"
static IDirect3DDevice9 *cube_wrap_device(IDirect3DDevice9 *inner) {
    DWORD error=GetLastError();
    if (!cube_ensure_mapping()) {SetLastError(error);return inner;}
    CubeDevice *f=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*f));
    if (!f) {cube_record_wrap(0,1);SetLastError(error);return inner;}
    f->iface.lpVtbl=&cube_device_vtable;f->inner=inner;f->references=1;
    if (!InitializeCriticalSectionAndSpinCount(&f->lock,0)) {HeapFree(GetProcessHeap(),0,f);cube_record_wrap(0,1);SetLastError(error);return inner;}
    cube_record_wrap(1,0);SetLastError(error);return &f->iface;
}
