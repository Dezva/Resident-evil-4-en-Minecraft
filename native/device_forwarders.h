/* Generated standard IDirect3DDevice9 ABI. See tools/generate_device_forwarders.py. */
static HRESULT WINAPI cube_TestCooperativeLevel(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->TestCooperativeLevel(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static UINT WINAPI cube_GetAvailableTextureMem(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    UINT result=f->inner->lpVtbl->GetAvailableTextureMem(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_EvictManagedResources(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->EvictManagedResources(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetDirect3D(IDirect3DDevice9 *iface, IDirect3D9** ppD3D9) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetDirect3D(f->inner, ppD3D9);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetDeviceCaps(IDirect3DDevice9 *iface, D3DCAPS9* pCaps) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetDeviceCaps(f->inner, pCaps);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetDisplayMode(IDirect3DDevice9 *iface, UINT iSwapChain, D3DDISPLAYMODE* pMode) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetDisplayMode(f->inner, iSwapChain, pMode);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetCreationParameters(IDirect3DDevice9 *iface, D3DDEVICE_CREATION_PARAMETERS * pParameters) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetCreationParameters(f->inner, pParameters);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetCursorProperties(IDirect3DDevice9 *iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetCursorProperties(f->inner, XHotSpot, YHotSpot, pCursorBitmap);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static void WINAPI cube_SetCursorPosition(IDirect3DDevice9 *iface, int X, int Y, DWORD Flags) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    f->inner->lpVtbl->SetCursorPosition(f->inner, X, Y, Flags);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return;
}
static WINBOOL WINAPI cube_ShowCursor(IDirect3DDevice9 *iface, WINBOOL bShow) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    WINBOOL result=f->inner->lpVtbl->ShowCursor(f->inner, bShow);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateAdditionalSwapChain(IDirect3DDevice9 *iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateAdditionalSwapChain(f->inner, pPresentationParameters, pSwapChain);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetSwapChain(IDirect3DDevice9 *iface, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetSwapChain(f->inner, iSwapChain, pSwapChain);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static UINT WINAPI cube_GetNumberOfSwapChains(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    UINT result=f->inner->lpVtbl->GetNumberOfSwapChains(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetBackBuffer(IDirect3DDevice9 *iface, UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetBackBuffer(f->inner, iSwapChain, iBackBuffer, Type, ppBackBuffer);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetRasterStatus(IDirect3DDevice9 *iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetRasterStatus(f->inner, iSwapChain, pRasterStatus);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetDialogBoxMode(IDirect3DDevice9 *iface, WINBOOL bEnableDialogs) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetDialogBoxMode(f->inner, bEnableDialogs);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static void WINAPI cube_SetGammaRamp(IDirect3DDevice9 *iface, UINT swapchain_idx, DWORD flags, const D3DGAMMARAMP * ramp) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    f->inner->lpVtbl->SetGammaRamp(f->inner, swapchain_idx, flags, ramp);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return;
}
static void WINAPI cube_GetGammaRamp(IDirect3DDevice9 *iface, UINT iSwapChain, D3DGAMMARAMP* pRamp) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    f->inner->lpVtbl->GetGammaRamp(f->inner, iSwapChain, pRamp);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return;
}
static HRESULT WINAPI cube_CreateTexture(IDirect3DDevice9 *iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateTexture(f->inner, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateVolumeTexture(IDirect3DDevice9 *iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateVolumeTexture(f->inner, Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateCubeTexture(IDirect3DDevice9 *iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateCubeTexture(f->inner, EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateVertexBuffer(IDirect3DDevice9 *iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateVertexBuffer(f->inner, Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateIndexBuffer(IDirect3DDevice9 *iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateIndexBuffer(f->inner, Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateRenderTarget(IDirect3DDevice9 *iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateRenderTarget(f->inner, Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateDepthStencilSurface(IDirect3DDevice9 *iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateDepthStencilSurface(f->inner, Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_UpdateSurface(IDirect3DDevice9 *iface, IDirect3DSurface9 * src_surface, const RECT * src_rect, IDirect3DSurface9 * dst_surface, const POINT * dst_point) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->UpdateSurface(f->inner, src_surface, src_rect, dst_surface, dst_point);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_UpdateTexture(IDirect3DDevice9 *iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->UpdateTexture(f->inner, pSourceTexture, pDestinationTexture);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetRenderTargetData(IDirect3DDevice9 *iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetRenderTargetData(f->inner, pRenderTarget, pDestSurface);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetFrontBufferData(IDirect3DDevice9 *iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetFrontBufferData(f->inner, iSwapChain, pDestSurface);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_StretchRect(IDirect3DDevice9 *iface, IDirect3DSurface9 * src_surface, const RECT * src_rect, IDirect3DSurface9 * dst_surface, const RECT * dst_rect, D3DTEXTUREFILTERTYPE filter) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->StretchRect(f->inner, src_surface, src_rect, dst_surface, dst_rect, filter);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_ColorFill(IDirect3DDevice9 *iface, IDirect3DSurface9 * surface, const RECT * rect, D3DCOLOR color) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->ColorFill(f->inner, surface, rect, color);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateOffscreenPlainSurface(IDirect3DDevice9 *iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateOffscreenPlainSurface(f->inner, Width, Height, Format, Pool, ppSurface, pSharedHandle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetRenderTarget(IDirect3DDevice9 *iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetRenderTarget(f->inner, RenderTargetIndex, pRenderTarget);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetRenderTarget(IDirect3DDevice9 *iface, DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetRenderTarget(f->inner, RenderTargetIndex, ppRenderTarget);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetDepthStencilSurface(IDirect3DDevice9 *iface, IDirect3DSurface9* pNewZStencil) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetDepthStencilSurface(f->inner, pNewZStencil);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetDepthStencilSurface(IDirect3DDevice9 *iface, IDirect3DSurface9** ppZStencilSurface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetDepthStencilSurface(f->inner, ppZStencilSurface);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_Clear(IDirect3DDevice9 *iface, DWORD rect_count, const D3DRECT * rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->Clear(f->inner, rect_count, rects, flags, color, z, stencil);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetTransform(IDirect3DDevice9 *iface, D3DTRANSFORMSTATETYPE state, const D3DMATRIX * matrix) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetTransform(f->inner, state, matrix);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetTransform(IDirect3DDevice9 *iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetTransform(f->inner, State, pMatrix);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_MultiplyTransform(IDirect3DDevice9 *iface, D3DTRANSFORMSTATETYPE state, const D3DMATRIX * matrix) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->MultiplyTransform(f->inner, state, matrix);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetViewport(IDirect3DDevice9 *iface, const D3DVIEWPORT9 * viewport) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetViewport(f->inner, viewport);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetViewport(IDirect3DDevice9 *iface, D3DVIEWPORT9* pViewport) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetViewport(f->inner, pViewport);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetMaterial(IDirect3DDevice9 *iface, const D3DMATERIAL9 * material) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetMaterial(f->inner, material);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetMaterial(IDirect3DDevice9 *iface, D3DMATERIAL9* pMaterial) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetMaterial(f->inner, pMaterial);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetLight(IDirect3DDevice9 *iface, DWORD index, const D3DLIGHT9 * light) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetLight(f->inner, index, light);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetLight(IDirect3DDevice9 *iface, DWORD Index, D3DLIGHT9* arg1) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetLight(f->inner, Index, arg1);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_LightEnable(IDirect3DDevice9 *iface, DWORD Index, WINBOOL Enable) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->LightEnable(f->inner, Index, Enable);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetLightEnable(IDirect3DDevice9 *iface, DWORD Index, WINBOOL* pEnable) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetLightEnable(f->inner, Index, pEnable);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetClipPlane(IDirect3DDevice9 *iface, DWORD index, const float * plane) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetClipPlane(f->inner, index, plane);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetClipPlane(IDirect3DDevice9 *iface, DWORD Index, float* pPlane) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetClipPlane(f->inner, Index, pPlane);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetRenderState(IDirect3DDevice9 *iface, D3DRENDERSTATETYPE State, DWORD Value) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetRenderState(f->inner, State, Value);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetRenderState(IDirect3DDevice9 *iface, D3DRENDERSTATETYPE State, DWORD* pValue) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetRenderState(f->inner, State, pValue);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateStateBlock(IDirect3DDevice9 *iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateStateBlock(f->inner, Type, ppSB);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_BeginStateBlock(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->BeginStateBlock(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_EndStateBlock(IDirect3DDevice9 *iface, IDirect3DStateBlock9** ppSB) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->EndStateBlock(f->inner, ppSB);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetClipStatus(IDirect3DDevice9 *iface, const D3DCLIPSTATUS9 * clip_status) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetClipStatus(f->inner, clip_status);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetClipStatus(IDirect3DDevice9 *iface, D3DCLIPSTATUS9* pClipStatus) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetClipStatus(f->inner, pClipStatus);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetTexture(IDirect3DDevice9 *iface, DWORD Stage, IDirect3DBaseTexture9** ppTexture) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetTexture(f->inner, Stage, ppTexture);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetTexture(IDirect3DDevice9 *iface, DWORD Stage, IDirect3DBaseTexture9* pTexture) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetTexture(f->inner, Stage, pTexture);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetTextureStageState(IDirect3DDevice9 *iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetTextureStageState(f->inner, Stage, Type, pValue);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetTextureStageState(IDirect3DDevice9 *iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetTextureStageState(f->inner, Stage, Type, Value);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetSamplerState(IDirect3DDevice9 *iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetSamplerState(f->inner, Sampler, Type, pValue);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetSamplerState(IDirect3DDevice9 *iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetSamplerState(f->inner, Sampler, Type, Value);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_ValidateDevice(IDirect3DDevice9 *iface, DWORD* pNumPasses) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->ValidateDevice(f->inner, pNumPasses);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetPaletteEntries(IDirect3DDevice9 *iface, UINT palette_idx, const PALETTEENTRY * entries) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetPaletteEntries(f->inner, palette_idx, entries);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetPaletteEntries(IDirect3DDevice9 *iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetPaletteEntries(f->inner, PaletteNumber, pEntries);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetCurrentTexturePalette(IDirect3DDevice9 *iface, UINT PaletteNumber) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetCurrentTexturePalette(f->inner, PaletteNumber);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetCurrentTexturePalette(IDirect3DDevice9 *iface, UINT * PaletteNumber) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetCurrentTexturePalette(f->inner, PaletteNumber);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetScissorRect(IDirect3DDevice9 *iface, const RECT * rect) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetScissorRect(f->inner, rect);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetScissorRect(IDirect3DDevice9 *iface, RECT* pRect) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetScissorRect(f->inner, pRect);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetSoftwareVertexProcessing(IDirect3DDevice9 *iface, WINBOOL bSoftware) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetSoftwareVertexProcessing(f->inner, bSoftware);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static WINBOOL WINAPI cube_GetSoftwareVertexProcessing(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    WINBOOL result=f->inner->lpVtbl->GetSoftwareVertexProcessing(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetNPatchMode(IDirect3DDevice9 *iface, float nSegments) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetNPatchMode(f->inner, nSegments);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static float WINAPI cube_GetNPatchMode(IDirect3DDevice9 *iface) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    float result=f->inner->lpVtbl->GetNPatchMode(f->inner);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawPrimitive(IDirect3DDevice9 *iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawPrimitive(f->inner, PrimitiveType, StartVertex, PrimitiveCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawIndexedPrimitive(IDirect3DDevice9 *iface, D3DPRIMITIVETYPE arg0, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawIndexedPrimitive(f->inner, arg0, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawPrimitiveUP(IDirect3DDevice9 *iface, D3DPRIMITIVETYPE primitive_type, UINT primitive_count, const void * data, UINT stride) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawPrimitiveUP(f->inner, primitive_type, primitive_count, data, stride);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawIndexedPrimitiveUP(IDirect3DDevice9 *iface, D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count, UINT primitive_count, const void * index_data, D3DFORMAT index_format, const void * data, UINT stride) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawIndexedPrimitiveUP(f->inner, primitive_type, min_vertex_idx, vertex_count, primitive_count, index_data, index_format, data, stride);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_ProcessVertices(IDirect3DDevice9 *iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->ProcessVertices(f->inner, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateVertexDeclaration(IDirect3DDevice9 *iface, const D3DVERTEXELEMENT9 * elements, IDirect3DVertexDeclaration9 ** declaration) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateVertexDeclaration(f->inner, elements, declaration);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetVertexDeclaration(IDirect3DDevice9 *iface, IDirect3DVertexDeclaration9* pDecl) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetVertexDeclaration(f->inner, pDecl);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetVertexDeclaration(IDirect3DDevice9 *iface, IDirect3DVertexDeclaration9** ppDecl) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetVertexDeclaration(f->inner, ppDecl);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetFVF(IDirect3DDevice9 *iface, DWORD FVF) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetFVF(f->inner, FVF);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetFVF(IDirect3DDevice9 *iface, DWORD* pFVF) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetFVF(f->inner, pFVF);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateVertexShader(IDirect3DDevice9 *iface, const DWORD * byte_code, IDirect3DVertexShader9 ** shader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateVertexShader(f->inner, byte_code, shader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetVertexShader(IDirect3DDevice9 *iface, IDirect3DVertexShader9* pShader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetVertexShader(f->inner, pShader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetVertexShader(IDirect3DDevice9 *iface, IDirect3DVertexShader9** ppShader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetVertexShader(f->inner, ppShader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetVertexShaderConstantF(IDirect3DDevice9 *iface, UINT reg_idx, const float * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetVertexShaderConstantF(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetVertexShaderConstantF(IDirect3DDevice9 *iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetVertexShaderConstantF(f->inner, StartRegister, pConstantData, Vector4fCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetVertexShaderConstantI(IDirect3DDevice9 *iface, UINT reg_idx, const int * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetVertexShaderConstantI(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetVertexShaderConstantI(IDirect3DDevice9 *iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetVertexShaderConstantI(f->inner, StartRegister, pConstantData, Vector4iCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetVertexShaderConstantB(IDirect3DDevice9 *iface, UINT reg_idx, const WINBOOL * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetVertexShaderConstantB(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetVertexShaderConstantB(IDirect3DDevice9 *iface, UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetVertexShaderConstantB(f->inner, StartRegister, pConstantData, BoolCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetStreamSource(IDirect3DDevice9 *iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetStreamSource(f->inner, StreamNumber, pStreamData, OffsetInBytes, Stride);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetStreamSource(IDirect3DDevice9 *iface, UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetStreamSource(f->inner, StreamNumber, ppStreamData, OffsetInBytes, pStride);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetStreamSourceFreq(IDirect3DDevice9 *iface, UINT StreamNumber, UINT Divider) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetStreamSourceFreq(f->inner, StreamNumber, Divider);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetStreamSourceFreq(IDirect3DDevice9 *iface, UINT StreamNumber, UINT* Divider) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetStreamSourceFreq(f->inner, StreamNumber, Divider);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetIndices(IDirect3DDevice9 *iface, IDirect3DIndexBuffer9* pIndexData) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetIndices(f->inner, pIndexData);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetIndices(IDirect3DDevice9 *iface, IDirect3DIndexBuffer9** ppIndexData) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetIndices(f->inner, ppIndexData);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreatePixelShader(IDirect3DDevice9 *iface, const DWORD * byte_code, IDirect3DPixelShader9 ** shader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreatePixelShader(f->inner, byte_code, shader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetPixelShader(IDirect3DDevice9 *iface, IDirect3DPixelShader9* pShader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetPixelShader(f->inner, pShader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetPixelShader(IDirect3DDevice9 *iface, IDirect3DPixelShader9** ppShader) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetPixelShader(f->inner, ppShader);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetPixelShaderConstantF(IDirect3DDevice9 *iface, UINT reg_idx, const float * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetPixelShaderConstantF(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetPixelShaderConstantF(IDirect3DDevice9 *iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetPixelShaderConstantF(f->inner, StartRegister, pConstantData, Vector4fCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetPixelShaderConstantI(IDirect3DDevice9 *iface, UINT reg_idx, const int * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetPixelShaderConstantI(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetPixelShaderConstantI(IDirect3DDevice9 *iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetPixelShaderConstantI(f->inner, StartRegister, pConstantData, Vector4iCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_SetPixelShaderConstantB(IDirect3DDevice9 *iface, UINT reg_idx, const WINBOOL * data, UINT count) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->SetPixelShaderConstantB(f->inner, reg_idx, data, count);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_GetPixelShaderConstantB(IDirect3DDevice9 *iface, UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->GetPixelShaderConstantB(f->inner, StartRegister, pConstantData, BoolCount);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawRectPatch(IDirect3DDevice9 *iface, UINT handle, const float * segment_count, const D3DRECTPATCH_INFO * patch_info) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawRectPatch(f->inner, handle, segment_count, patch_info);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DrawTriPatch(IDirect3DDevice9 *iface, UINT handle, const float * segment_count, const D3DTRIPATCH_INFO * patch_info) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DrawTriPatch(f->inner, handle, segment_count, patch_info);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_DeletePatch(IDirect3DDevice9 *iface, UINT Handle) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->DeletePatch(f->inner, Handle);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static HRESULT WINAPI cube_CreateQuery(IDirect3DDevice9 *iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) {
    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);
    HRESULT result=f->inner->lpVtbl->CreateQuery(f->inner, Type, ppQuery);
    DWORD error=GetLastError();cube_method_leave(f,error);
    return result;
}
static IDirect3DDevice9Vtbl cube_device_vtable={cube_QueryInterface,cube_AddRef,cube_Release,cube_TestCooperativeLevel,cube_GetAvailableTextureMem,cube_EvictManagedResources,cube_GetDirect3D,cube_GetDeviceCaps,cube_GetDisplayMode,cube_GetCreationParameters,cube_SetCursorProperties,cube_SetCursorPosition,cube_ShowCursor,cube_CreateAdditionalSwapChain,cube_GetSwapChain,cube_GetNumberOfSwapChains,cube_Reset,cube_Present,cube_GetBackBuffer,cube_GetRasterStatus,cube_SetDialogBoxMode,cube_SetGammaRamp,cube_GetGammaRamp,cube_CreateTexture,cube_CreateVolumeTexture,cube_CreateCubeTexture,cube_CreateVertexBuffer,cube_CreateIndexBuffer,cube_CreateRenderTarget,cube_CreateDepthStencilSurface,cube_UpdateSurface,cube_UpdateTexture,cube_GetRenderTargetData,cube_GetFrontBufferData,cube_StretchRect,cube_ColorFill,cube_CreateOffscreenPlainSurface,cube_SetRenderTarget,cube_GetRenderTarget,cube_SetDepthStencilSurface,cube_GetDepthStencilSurface,cube_BeginScene,cube_EndScene,cube_Clear,cube_SetTransform,cube_GetTransform,cube_MultiplyTransform,cube_SetViewport,cube_GetViewport,cube_SetMaterial,cube_GetMaterial,cube_SetLight,cube_GetLight,cube_LightEnable,cube_GetLightEnable,cube_SetClipPlane,cube_GetClipPlane,cube_SetRenderState,cube_GetRenderState,cube_CreateStateBlock,cube_BeginStateBlock,cube_EndStateBlock,cube_SetClipStatus,cube_GetClipStatus,cube_GetTexture,cube_SetTexture,cube_GetTextureStageState,cube_SetTextureStageState,cube_GetSamplerState,cube_SetSamplerState,cube_ValidateDevice,cube_SetPaletteEntries,cube_GetPaletteEntries,cube_SetCurrentTexturePalette,cube_GetCurrentTexturePalette,cube_SetScissorRect,cube_GetScissorRect,cube_SetSoftwareVertexProcessing,cube_GetSoftwareVertexProcessing,cube_SetNPatchMode,cube_GetNPatchMode,cube_DrawPrimitive,cube_DrawIndexedPrimitive,cube_DrawPrimitiveUP,cube_DrawIndexedPrimitiveUP,cube_ProcessVertices,cube_CreateVertexDeclaration,cube_SetVertexDeclaration,cube_GetVertexDeclaration,cube_SetFVF,cube_GetFVF,cube_CreateVertexShader,cube_SetVertexShader,cube_GetVertexShader,cube_SetVertexShaderConstantF,cube_GetVertexShaderConstantF,cube_SetVertexShaderConstantI,cube_GetVertexShaderConstantI,cube_SetVertexShaderConstantB,cube_GetVertexShaderConstantB,cube_SetStreamSource,cube_GetStreamSource,cube_SetStreamSourceFreq,cube_GetStreamSourceFreq,cube_SetIndices,cube_GetIndices,cube_CreatePixelShader,cube_SetPixelShader,cube_GetPixelShader,cube_SetPixelShaderConstantF,cube_GetPixelShaderConstantF,cube_SetPixelShaderConstantI,cube_GetPixelShaderConstantI,cube_SetPixelShaderConstantB,cube_GetPixelShaderConstantB,cube_DrawRectPatch,cube_DrawTriPatch,cube_DeletePatch,cube_CreateQuery};
