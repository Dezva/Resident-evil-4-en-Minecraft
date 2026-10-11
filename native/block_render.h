/* Independent D3D9 diagnostic renderer. No game entities or collision writes. */
#define COBJMACROS
#include <d3d9.h>
#include "block_geometry.h"
typedef struct {
    uint32_t magic,version;
    volatile LONG sequence;
    volatile uint32_t stamp,enabled,generation;
    volatile float bounds[6];
    volatile uint32_t status,frames,draws,failures,last_hresult;
} BlockShared;
_Static_assert(sizeof(BlockShared)==68,"Block preview layout");
static BlockShared *block_state;
static HANDLE block_mapping;
static IDirect3DDevice9 *block_device;
static IDirect3DDevice9Vtbl block_vtable;
static HRESULT (WINAPI *block_original_end)(IDirect3DDevice9 *);
static uint32_t block_poll_stamp;
static unsigned block_attempts;

static HRESULT WINAPI block_end_scene(IDirect3DDevice9 *device) {
    if (device==block_device && block_state) {
        block_state->frames++;
        if (GetAsyncKeyState(VK_F8)&0x8000) InterlockedExchange(&state->stop,1);
        ControlFrame control={0};int complete=read_control_frame(&control);
        uint32_t now=GetTickCount(),seq=(uint32_t)block_state->sequence;
        MemoryBarrier();
        uint32_t enabled=block_state->enabled,stamp=block_state->stamp,generation=block_state->generation;
        float bounds[6];for (unsigned i=0;i<6;++i) bounds[i]=block_state->bounds[i];
        MemoryBarrier();
        if (!(seq&1) && seq==(uint32_t)block_state->sequence && enabled && complete && control.armed
                && state->armed && !state->stop && generation==control.generation && (uint32_t)(now-stamp)<RE4_TTL_MS
                && (uint32_t)(now-control.stamp)<RE4_TTL_MS && current_player()) {
            uint32_t global=global_slot?*global_slot:0;
            if (global>=0x10000 && global<=0xFFFFFE00 && !(global&3u)
                    && readable((void *)(uintptr_t)(global+0x74),0xF8)) {
                unsigned char *camera=(unsigned char *)(uintptr_t)(global+0x74);
                float view[12],fovy;memcpy(view,camera+0x30,sizeof(view));memcpy(&fovy,camera+0xC0,4);
                D3DVIEWPORT9 viewport;
                if (SUCCEEDED(IDirect3DDevice9_GetViewport(device,&viewport))) {
                    BlockVertex vertices[24];
                    unsigned count=block_lines(bounds,view,fovy,(float)viewport.Width,(float)viewport.Height,vertices);
                    if (count) {
                        for (unsigned i=0;i<count;++i) {vertices[i].x+=viewport.X;vertices[i].y+=viewport.Y;}
                        IDirect3DStateBlock9 *backup=NULL;
                        HRESULT hr=IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&backup);
                        if (SUCCEEDED(hr)) {
                            hr=IDirect3DStateBlock9_Capture(backup);
                            if (SUCCEEDED(hr)) {
                                hr=IDirect3DDevice9_SetVertexShader(device,NULL);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetPixelShader(device,NULL);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTexture(device,0,NULL);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetTextureStageState(device,1,D3DTSS_COLOROP,D3DTOP_DISABLE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ZWRITEENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_FOGENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_SCISSORTESTENABLE,FALSE);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_CLIPPLANEENABLE,0);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_SetRenderState(device,D3DRS_COLORWRITEENABLE,15);
                                if (SUCCEEDED(hr)) hr=IDirect3DDevice9_DrawPrimitiveUP(device,D3DPT_LINELIST,count/2,vertices,sizeof(BlockVertex));
                                HRESULT restored=IDirect3DStateBlock9_Apply(backup);
                                if (FAILED(restored)) hr=restored;
                                if (SUCCEEDED(hr)) block_state->draws++;
                            }
                            IDirect3DStateBlock9_Release(backup);
                        }
                        block_state->last_hresult=(uint32_t)hr;
                        if (FAILED(hr)) block_state->failures++;
                    }
                }
            }
        }
    }
    return block_original_end(device);
}

/* Exact device-vtable identity from an official system D3D9 probe, not guessed
   executable addresses. Scan only committed readable/writable PE data sections.
   Accept one unique game-window device, and replace its own vtable pointer. */
static void block_poll(void) {
    if (!block_state || block_device || state->hook_ticks<120 || block_attempts>=5
            || (uint32_t)(GetTickCount()-block_poll_stamp)<2000) return;
    block_poll_stamp=GetTickCount();block_attempts++;
    wchar_t path[MAX_PATH];UINT n=GetSystemDirectoryW(path,MAX_PATH);
    if (!n || n>MAX_PATH-12) return;
    memcpy(path+n,L"\\d3d9.dll",sizeof(L"\\d3d9.dll"));
    HMODULE library=LoadLibraryExW(path,NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) {block_state->status=2;return;}
    typedef IDirect3D9 *(WINAPI *Create)(UINT);
    Create create=(Create)GetProcAddress(library,"Direct3DCreate9");
    IDirect3D9 *d3d=create?create(D3D_SDK_VERSION):NULL;
    if (!d3d) {FreeLibrary(library);block_state->status=2;return;}
    WNDCLASSW wc={0};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(NULL);
    wc.lpszClassName=L"RE4Bridge-DeviceProbe";
    ATOM atom=RegisterClassW(&wc);
    HWND window=atom?CreateWindowExW(0,wc.lpszClassName,L"",WS_POPUP,0,0,16,16,NULL,NULL,wc.hInstance,NULL):NULL;
    D3DPRESENT_PARAMETERS pp={0};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow=window;pp.BackBufferWidth=16;pp.BackBufferHeight=16;pp.BackBufferFormat=D3DFMT_UNKNOWN;
    D3DCAPS9 caps={0};IDirect3D9_GetDeviceCaps(d3d,0,D3DDEVTYPE_HAL,&caps);
    DWORD flags=(caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)?D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    IDirect3DDevice9 *probe=NULL;
    HRESULT hr=window?IDirect3D9_CreateDevice(d3d,0,D3DDEVTYPE_HAL,window,flags,&pp,&probe):E_FAIL;
    IDirect3DDevice9 *found=NULL;int ambiguous=0;
    if (SUCCEEDED(hr)) {
        IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)image_base;
        IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)(image_base+dos->e_lfanew);
        IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);
        for (unsigned s=0;s<nt->FileHeader.NumberOfSections;++s) {
            DWORD rva=sections[s].VirtualAddress,length=sections[s].Misc.VirtualSize;
            if (!(sections[s].Characteristics&IMAGE_SCN_MEM_WRITE) || (sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE)
                    || rva>=image_size || length>image_size-rva || !readable(image_base+rva,length)) continue;
            for (DWORD offset=0;offset+4<=length;offset+=4) {
                uint32_t value;memcpy(&value,image_base+rva+offset,4);
                if (value<0x10000 || value>0xFFFFFF00 || (value&3u) || value==(uint32_t)(uintptr_t)probe
                        || !readable((void *)(uintptr_t)value,4) || !writable((void *)(uintptr_t)value,4)) continue;
                IDirect3DDevice9 *candidate=(IDirect3DDevice9 *)(uintptr_t)value;
                if (candidate->lpVtbl!=probe->lpVtbl) continue;
                D3DDEVICE_CREATION_PARAMETERS creation;
                if (FAILED(IDirect3DDevice9_GetCreationParameters(candidate,&creation)) || creation.hFocusWindow==window) continue;
                DWORD pid=0;DWORD thread=GetWindowThreadProcessId(creation.hFocusWindow,&pid);
                if (pid!=GetCurrentProcessId() || thread!=GetCurrentThreadId() || !IsWindowVisible(creation.hFocusWindow)) continue;
                if (found && candidate!=found) ambiguous=1;
                found=candidate;
            }
        }
        if (found && !ambiguous) {
            const IDirect3DDevice9Vtbl *previous=found->lpVtbl;
            memcpy(&block_vtable,previous,sizeof(block_vtable));
            block_original_end=block_vtable.EndScene;block_vtable.EndScene=block_end_scene;
            block_device=found;
            if (InterlockedCompareExchangePointer((PVOID volatile *)found,(void *)&block_vtable,(void *)previous)==previous) {
                block_device=found;block_state->status=1;
            } else {block_device=NULL;block_state->status=4;}
        } else block_state->status=ambiguous?4:3;
        IDirect3DDevice9_Release(probe);
    } else {block_state->status=2;block_state->last_hresult=(uint32_t)hr;}
    if (window) DestroyWindow(window);
    if (atom) UnregisterClassW(wc.lpszClassName,wc.hInstance);
    IDirect3D9_Release(d3d);FreeLibrary(library);
}
static void block_initialize(void) {
    wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-BlockPreview-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(BlockShared),name);
    if (!mapping) return;
    if (GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(mapping);return;}
    block_state=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(BlockShared));
    if (!block_state) {CloseHandle(mapping);return;}
    block_mapping=mapping;memset(block_state,0,sizeof(*block_state));
    block_state->magic=0x52453442;block_state->version=1;
}
