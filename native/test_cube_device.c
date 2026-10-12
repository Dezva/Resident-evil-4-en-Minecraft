#include "test_cube_platform.h"
#include "cube_policy.h"
static IDirect3DDevice9 inner;
static unsigned native_refs,forwarded,allocations,frees,allocation_fail,mapping_fail,lock_fail,live,wrap_failures;
static unsigned events[32],renders,begins,ends,begin_fail,end_fail,reset_fail,present_fail;
static DWORD last_error;
static IDirect3DDevice9 *reset_reentry;
static unsigned reset_drop_reference;
static unsigned resource_releases;
static unsigned native_adds,native_drops;
static ULONG resource_release(IDirect3DTexture9 *texture){(void)texture;assert(native_refs>1);native_refs--;resource_releases++;return 0;}
static TestTextureVtbl resource_table={.Release=resource_release};
static IDirect3DTexture9 resource={&resource_table};
static void EnterCriticalSection(CRITICAL_SECTION *lock) {lock->held++;}
static void LeaveCriticalSection(CRITICAL_SECTION *lock) {assert(lock->held);lock->held--;last_error=999;}
static int InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *lock,DWORD spin) {assert(!spin);lock->held=0;return !lock_fail;}
static void DeleteCriticalSection(CRITICAL_SECTION *lock) {assert(!lock->held);}
static LONG InterlockedIncrement(volatile LONG *value) {return ++*value;}
static LONG InterlockedDecrement(volatile LONG *value) {return --*value;}
static DWORD GetLastError(void) {return last_error;}
static void SetLastError(DWORD value) {last_error=value;}
static void *GetProcessHeap(void) {return (void *)1;}
static void *HeapAlloc(void *heap,DWORD flags,size_t size) {assert(heap==(void *)1 && flags==8);if(allocation_fail)return NULL;allocations++;return calloc(1,size);}
static int HeapFree(void *heap,DWORD flags,void *pointer) {assert(heap==(void *)1 && !flags);frees++;free(pointer);last_error=888;return 1;}
static int cube_ensure_mapping(void) {last_error=123;return !mapping_fail;}
static void cube_record_wrap(int delta,int failed) {live=(unsigned)((int)live+delta);wrap_failures+=(unsigned)failed;last_error=123;}
static void cube_record(unsigned event,uint32_t value) {(void)value;events[event]++;last_error=123;}
static HRESULT fake_QueryInterface(IDirect3DDevice9 *self,REFIID iid,void **out) {assert(self==&inner);last_error=444;*out=NULL;if(iid && iid->Data1==42){*out=self;native_refs++;return 29;}return E_NOINTERFACE;}
static ULONG fake_AddRef(IDirect3DDevice9 *self) {assert(self==&inner);native_adds++;last_error=444;return ++native_refs;}
static ULONG fake_Release(IDirect3DDevice9 *self) {assert(self==&inner && native_refs);native_drops++;last_error=444;return --native_refs;}
static HRESULT fake_BeginScene(IDirect3DDevice9 *self) {assert(self==&inner);begins++;last_error=444;return begin_fail?E_FAIL:S_OK;}
static HRESULT fake_EndScene(IDirect3DDevice9 *self) {assert(self==&inner);ends++;last_error=444;return end_fail?E_FAIL:27;}
static HRESULT fake_Reset(IDirect3DDevice9 *self,D3DPRESENT_PARAMETERS *p) {
    assert(self==&inner && p->marker==123);
    assert(resource_releases); // default-pool resources must already be gone before native Reset
    if (reset_reentry) assert(reset_reentry->lpVtbl->GetAvailableTextureMem(reset_reentry)==(UINT)0x7004);
    if (reset_drop_reference) {
        for(unsigned i=0;i<reset_drop_reference;i++)assert(reset_reentry->lpVtbl->Release(reset_reentry)==reset_drop_reference-i);
        assert(native_refs==reset_drop_reference); // retain backing references until all nested methods return
        assert(reset_reentry->lpVtbl->GetAvailableTextureMem(reset_reentry)==(UINT)0x7004 && native_refs==reset_drop_reference);
        reset_reentry=NULL;reset_drop_reference=0;
    }
    last_error=444;return reset_fail?E_FAIL:28;
}
static HRESULT fake_Present(IDirect3DDevice9 *self,const RECT *a,const RECT *b,HWND c,const RGNDATA *d) {assert(self==&inner && a==(void *)1 && b==(void *)2 && c==(void *)3 && d==(void *)4);last_error=444;return present_fail?E_FAIL:30;}
#include "test_device_passthrough.h"
static unsigned scene_open,scene_starts;
static void block_camera_begin(void){scene_open=1;scene_starts++;}
static void block_camera_end(void){scene_open=0;}
#define CUBE_TEST_DRAW_STUB
#include "cube_device.h"
static void hud_render_device(CubeDevice *f){(void)f;}
static void hud_reset_device(CubeDevice *f){(void)f;}
static void hud_release_texture(CubeDevice *f){if(f->hud_texture)f->hud_texture->lpVtbl->Release(f->hud_texture);f->hud_texture=NULL;f->hud_pixels_valid=0;}
static void cube_render_device(CubeDevice *f) {if (f->scene && !f->escaped && !f->resetting) renders++;}
int main(void) {
    inner.lpVtbl=&native_table;native_refs=1;last_error=777;
    IDirect3DDevice9 before=inner,*w=cube_wrap_device(&inner);assert(w!=&inner && live==1 && allocations==1 && native_refs==1 && last_error==777);
    assert(!memcmp(&before,&inner,sizeof(inner)));
    void *out=NULL;assert(w->lpVtbl->QueryInterface(w,&cube_iunknown,&out)==S_OK && out==w && native_refs==2);
    assert(w->lpVtbl->Release(w)==1 && native_refs==1 && last_error==444);
    assert(w->lpVtbl->QueryInterface(w,&cube_iid_device,&out)==S_OK && out==w);
    assert(w->lpVtbl->Release(w)==1);
    assert(w->lpVtbl->QueryInterface(w,&cube_iunknown,NULL)==E_POINTER);
    unsigned before_adds=native_adds,before_drops=native_drops;
    #include "test_device_calls.h"
    assert(native_adds==before_adds && native_drops==before_drops && native_refs==1); // 112 forwards, zero native ref churn
    assert(w->lpVtbl->EndScene(w)==27 && !renders && ends==1 && last_error==444);
    begin_fail=1;assert(w->lpVtbl->BeginScene(w)==E_FAIL);assert(w->lpVtbl->EndScene(w)==27 && !renders);
    begin_fail=0;assert(w->lpVtbl->BeginScene(w)==S_OK);assert(w->lpVtbl->EndScene(w)==27 && renders==1 && !scene_open && scene_starts==1);
    assert(last_error==444 && events[CUBE_FRAME]==3 && events[CUBE_BEGIN]==2);
    D3DPRESENT_PARAMETERS parameters={123};CubeDevice *f=(CubeDevice *)w;f->gate.epoch=9;f->gate.anchored=1;f->scene=1;
    f->block_texture.texture=&resource;native_refs++; // texture owns a native device reference
    reset_reentry=w;reset_fail=1;assert(w->lpVtbl->Reset(w,&parameters)==E_FAIL && f->gate.lost && !f->scene && !f->gate.anchored && f->gate.blocked==9 && last_error==444 && !f->resetting && !f->lock.held);
    reset_fail=0;assert(w->lpVtbl->Reset(w,&parameters)==28 && !f->gate.lost && f->gate.blocked==9);
    assert(resource_releases==1 && !f->block_texture.texture);
    present_fail=1;assert(w->lpVtbl->Present(w,(void *)1,(void *)2,(void *)3,(void *)4)==E_FAIL && f->gate.lost && last_error==444);
    assert(events[CUBE_PRESENT_FAILURE]==1 && events[CUBE_RESET]==2 && events[CUBE_RESET_FAILURE]==1);
    GUID unknown={42,0,0,{0}};assert(w->lpVtbl->QueryInterface(w,&unknown,&out)==29 && out==&inner && native_refs==2 && f->escaped && events[CUBE_ESCAPE]==1 && last_error==444);
    assert(w->lpVtbl->BeginScene(w)==S_OK && w->lpVtbl->EndScene(w)==27 && renders==1);
    f->hud_texture=&resource;native_refs++;
    // The escaped interface owns a native reference after the facade dies.
    assert(w->lpVtbl->Release(w)==0 && live==0 && frees==1 && native_refs==1 && last_error==444);
    assert(resource_releases==2);
    assert(inner.lpVtbl->Release(&inner)==0);
    native_refs=1;allocation_fail=1;last_error=777;
    assert(cube_wrap_device(&inner)==&inner && wrap_failures==1 && native_refs==1 && last_error==777);
    allocation_fail=0;mapping_fail=1;assert(cube_wrap_device(&inner)==&inner && allocations==frees && last_error==777);
    mapping_fail=0;lock_fail=1;assert(cube_wrap_device(&inner)==&inner && allocations==frees && wrap_failures==2 && last_error==777);
    assert(!memcmp(&before,&inner,sizeof(inner)));
    lock_fail=0;reset_reentry=cube_wrap_device(&inner);reset_drop_reference=1;unsigned prior_frees=frees;
    assert(reset_reentry->lpVtbl->Reset(reset_reentry,&parameters)==28 && native_refs==0 && live==0 && frees==prior_frees+1 && allocations==frees);
    native_refs=1;reset_reentry=cube_wrap_device(&inner);assert(reset_reentry->lpVtbl->AddRef(reset_reentry)==2 && native_refs==2);
    reset_drop_reference=2;prior_frees=frees;
    assert(reset_reentry->lpVtbl->Reset(reset_reentry,&parameters)==28 && native_refs==0 && live==0 && frees==prior_frees+1 && allocations==frees);
    CubeGate gate={0};CubeCommand cmd={.enabled=1,.epoch=7,.stamp=UINT32_MAX-10};
    assert(!cube_gate(&gate,&cmd,10,0));assert(cube_gate(&gate,&cmd,600,0)==CUBE_STALE);
    assert(cube_gate(&gate,&cmd,10,1)==CUBE_F8 && cube_gate(&gate,&cmd,10,0)==CUBE_F8);
    cmd.epoch=8;assert(!cube_gate(&gate,&cmd,10,0));gate.lost=1;assert(cube_gate(&gate,&cmd,10,0)==CUBE_LOST);
    gate.lost=0;assert(cube_gate(&gate,&cmd,10,0)==CUBE_LOST);cmd.epoch=9;assert(!cube_gate(&gate,&cmd,10,0));
    gate.fatal=1;cmd.epoch=10;assert(cube_gate(&gate,&cmd,10,0)==CUBE_RESTORE);
    puts("Device facade:119 slots, exact passthrough, identity/refcounts, native device untouched, failure fallback, scene/reset/loss, LastError and lease/F8 passed");
}
