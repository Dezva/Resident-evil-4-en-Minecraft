/* External IDirect3D9 facade over a freshly returned, owned COM reference.
 * Never replaces the original object's vtable. Successful devices get a facade.
 * Windows D3D9 declarations / platform functions are supplied by the includer. */
typedef struct {
    IDirect3D9 iface;
    IDirect3D9 *inner;
    volatile LONG references;
} CaptureFactory;
static const GUID capture_iunknown={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID capture_iid9={0x81bdcbca,0x64d4,0x426d,{0xae,0x8d,0xad,1,0x47,0xf4,0x27,0x5c}};
static ULONG WINAPI capture_addref(IDirect3D9 *iface) {
    CaptureFactory *f=(CaptureFactory *)iface;
    f->inner->lpVtbl->AddRef(f->inner);
    return (ULONG)InterlockedIncrement(&f->references);
}
static ULONG WINAPI capture_release(IDirect3D9 *iface) {
    CaptureFactory *f=(CaptureFactory *)iface;
    LONG remaining=InterlockedDecrement(&f->references);
    f->inner->lpVtbl->Release(f->inner);
    DWORD error=GetLastError();
    if (!remaining) {
        capture_record_wrap(-1,0);
        HeapFree(GetProcessHeap(),0,f);
    }
    SetLastError(error);return (ULONG)remaining;
}
static HRESULT WINAPI capture_query(IDirect3D9 *iface,REFIID iid,void **out) {
    CaptureFactory *f=(CaptureFactory *)iface;
    if (!out) return E_POINTER;
    if (iid && (!memcmp(iid,&capture_iunknown,sizeof(GUID)) || !memcmp(iid,&capture_iid9,sizeof(GUID)))) {
        capture_addref(iface);*out=iface;return S_OK;
    }
    /* Unsupported interfaces are forwarded, not masqueraded as our facade. */
    HRESULT hr=f->inner->lpVtbl->QueryInterface(f->inner,iid,out);
    if (SUCCEEDED(hr) && *out) capture_record_escape();
    return hr;
}
static HRESULT WINAPI capture_software(IDirect3D9 *iface,void *init) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->RegisterSoftwareDevice(f->inner,init);
}
static UINT WINAPI capture_count(IDirect3D9 *iface) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetAdapterCount(f->inner);
}
static HRESULT WINAPI capture_identifier(IDirect3D9 *iface,UINT adapter,DWORD flags,D3DADAPTER_IDENTIFIER9 *out) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetAdapterIdentifier(f->inner,adapter,flags,out);
}
static UINT WINAPI capture_mode_count(IDirect3D9 *iface,UINT adapter,D3DFORMAT format) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetAdapterModeCount(f->inner,adapter,format);
}
static HRESULT WINAPI capture_modes(IDirect3D9 *iface,UINT adapter,D3DFORMAT format,UINT mode,D3DDISPLAYMODE *out) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->EnumAdapterModes(f->inner,adapter,format,mode,out);
}
static HRESULT WINAPI capture_display_mode(IDirect3D9 *iface,UINT adapter,D3DDISPLAYMODE *out) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetAdapterDisplayMode(f->inner,adapter,out);
}
static HRESULT WINAPI capture_type(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DFORMAT display,D3DFORMAT back,BOOL windowed) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->CheckDeviceType(f->inner,adapter,type,display,back,windowed);
}
static HRESULT WINAPI capture_format(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DFORMAT format,DWORD usage,D3DRESOURCETYPE resource,D3DFORMAT check) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->CheckDeviceFormat(f->inner,adapter,type,format,usage,resource,check);
}
static HRESULT WINAPI capture_multisample(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DFORMAT format,BOOL windowed,D3DMULTISAMPLE_TYPE sample,DWORD *quality) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->CheckDeviceMultiSampleType(f->inner,adapter,type,format,windowed,sample,quality);
}
static HRESULT WINAPI capture_depth(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DFORMAT format,D3DFORMAT render,D3DFORMAT depth) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->CheckDepthStencilMatch(f->inner,adapter,type,format,render,depth);
}
static HRESULT WINAPI capture_conversion(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DFORMAT source,D3DFORMAT target) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->CheckDeviceFormatConversion(f->inner,adapter,type,source,target);
}
static HRESULT WINAPI capture_caps(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,D3DCAPS9 *out) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetDeviceCaps(f->inner,adapter,type,out);
}
static HMONITOR WINAPI capture_monitor(IDirect3D9 *iface,UINT adapter) {
    CaptureFactory *f=(CaptureFactory *)iface;return f->inner->lpVtbl->GetAdapterMonitor(f->inner,adapter);
}
static HRESULT WINAPI capture_device(IDirect3D9 *iface,UINT adapter,D3DDEVTYPE type,HWND focus,DWORD flags,D3DPRESENT_PARAMETERS *parameters,IDirect3DDevice9 **out) {
    CaptureFactory *f=(CaptureFactory *)iface;
    HRESULT hr=f->inner->lpVtbl->CreateDevice(f->inner,adapter,type,focus,flags,parameters,out);
    DWORD error=GetLastError();
    /* Only wrap the API's successful, owned reference. Failed calls may leave
       output/parameters untouched or invalid. No device address discovery. */
    IDirect3DDevice9 *device=SUCCEEDED(hr) && out?*out:NULL;
    capture_record_device(hr,device,adapter,type,focus,flags,device?parameters:NULL);
    if (device) *out=cube_wrap_device(device);
    SetLastError(error);return hr;
}
static IDirect3D9Vtbl capture_factory_vtable={
    capture_query,capture_addref,capture_release,capture_software,capture_count,
    capture_identifier,capture_mode_count,capture_modes,capture_display_mode,
    capture_type,capture_format,capture_multisample,capture_depth,
    capture_conversion,capture_caps,capture_monitor,capture_device
};
static IDirect3D9 *WINAPI capture_create9(UINT sdk) {
    IDirect3D9 *inner=capture_original_create9(sdk);
    DWORD error=GetLastError();
    capture_record_factory(inner);
    if (!inner) {SetLastError(error);return NULL;}
    CaptureFactory *f=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*f));
    if (!f) {capture_record_wrap(0,1);SetLastError(error);return inner;}
    f->iface.lpVtbl=&capture_factory_vtable;f->inner=inner;f->references=1;
    capture_record_wrap(1,0);
    SetLastError(error);return &f->iface;
}
