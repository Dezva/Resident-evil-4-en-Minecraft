/* WITHDRAWN after the 0.4.13 startup crash report. Historical fixtures only:
 * readable tables cannot prove COM object lifetime; never include in a DLL. */
#error "Unsafe graphics discovery withdrawn: capture a live device before implementing rendering"
#define COBJMACROS
#include <d3d9.h>
#include "block_geometry.h"
typedef struct {
    uint32_t magic,version;
    volatile LONG sequence;
    volatile uint32_t stamp,enabled,generation;
    volatile float bounds[6];
    volatile uint32_t status,frames,draws,failures,last_hresult;
    volatile uint32_t attempts,slots,tables,devices,window_rejected,roots;
    volatile uint32_t factory_rejected,probe_valid;
} BlockShared;
_Static_assert(sizeof(BlockShared)==100,"Block preview layout");
static BlockShared *block_state;
static HANDLE block_mapping;
static IDirect3DDevice9 *block_device;
static IDirect3DDevice9Vtbl block_vtable;
static HRESULT (WINAPI *block_original_end)(IDirect3DDevice9 *);
static uint32_t block_poll_stamp;
static unsigned block_attempts;
static uintptr_t block_roots[8192];
static const uint32_t *block_factory_slot;
static const int block_factory_pattern[]={0xE8,-1,-1,-1,-1,0xA3,-1,-1,-1,-1,0x85,0xC0,0x75,0x17};

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

/* Exact device-vtable identity is not required: D3D9 has device variants.
   Validate introspection methods against the system probe and every method's
   executable module before invoking COM. Search only game-owned pointer roots. */
typedef struct {
    IDirect3DDevice9 *probe,*found;
    HWND probe_window;
    uintptr_t library_start,library_end;
    unsigned ambiguous;
    IDirect3D9 *game_factory;
} BlockSearch;
static int block_new_root(uintptr_t value) {
    unsigned slot=(unsigned)((value>>4)*2654435761u)&8191u;
    while (block_roots[slot] && block_roots[slot]!=value) slot=(slot+1)&8191u;
    if (block_roots[slot]) return 0;
    block_roots[slot]=value;return 1;
}
static int block_system_table(const IDirect3DDevice9Vtbl *table,BlockSearch *search) {
    /* Windows can allocate a per-device method table on the heap. Its storage
       does not establish identity: validate every function before COM calls. */
    if ((uintptr_t)table<0x10000 || ((uintptr_t)table&3u) || !readable(table,sizeof(*table))) return 0;
    const IDirect3DDevice9Vtbl *reference=search->probe->lpVtbl;
    if (table->GetCreationParameters!=reference->GetCreationParameters
            || table->GetDirect3D!=reference->GetDirect3D || table->GetDeviceCaps!=reference->GetDeviceCaps) return 0;
    for (unsigned i=0;i<sizeof(*table)/sizeof(void *);++i) {
        uintptr_t method;memcpy(&method,(const unsigned char *)table+i*sizeof(void *),sizeof(method));
        MEMORY_BASIC_INFORMATION memory;
        if (!VirtualQuery((void *)method,&memory,sizeof(memory)) || memory.State!=MEM_COMMIT
                || (uintptr_t)memory.AllocationBase!=search->library_start
                || (memory.Protect&(PAGE_GUARD|PAGE_NOACCESS))
                || !(memory.Protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return 0;
    }
    return 1;
}
static void block_candidate(uintptr_t value,BlockSearch *search) {
    if (value<0x10000 || value>UINT32_MAX-0x100 || (value&3u) || value==(uintptr_t)search->probe
            || !readable((void *)value,sizeof(void *)) || !writable((void *)value,sizeof(void *))) return;
    block_state->slots++;
    IDirect3DDevice9 *candidate=(IDirect3DDevice9 *)value;
    if (!block_system_table(candidate->lpVtbl,search)) return;
    block_state->tables++;
    D3DDEVICE_CREATION_PARAMETERS creation={0};
    if (FAILED(IDirect3DDevice9_GetCreationParameters(candidate,&creation))) return;
    DWORD pid=0;
    GetWindowThreadProcessId(creation.hFocusWindow,&pid);
    /* Window ownership, not equality with the pad thread. The hook itself runs
       when RE4 calls EndScene on its render thread. */
    if (pid!=GetCurrentProcessId() || creation.hFocusWindow==search->probe_window
            || !IsWindowVisible(creation.hFocusWindow)) {block_state->window_rejected++;return;}
    IDirect3D9 *factory=NULL;
    if (FAILED(IDirect3DDevice9_GetDirect3D(candidate,&factory)) || !factory) {block_state->factory_rejected++;return;}
    int same_factory=factory==search->game_factory;
    IDirect3D9_Release(factory); /* GetDirect3D returns an owned COM reference. */
    if (!same_factory) {block_state->factory_rejected++;return;}
    block_state->devices++;
    if (search->found && candidate!=search->found) search->ambiguous=1;
    search->found=candidate;
}
/* Region-wise traversal: PE data may span multiple protections/allocations. */
static void block_scan_range(uintptr_t start,SIZE_T length,BlockSearch *search,int indirect) {
    uintptr_t end=start+length;
    for (uintptr_t cursor=start;cursor<end;) {
        MEMORY_BASIC_INFORMATION memory;
        if (!VirtualQuery((void *)cursor,&memory,sizeof(memory))) break;
        uintptr_t region_end=(uintptr_t)memory.BaseAddress+memory.RegionSize;
        if (region_end<=cursor) break;
        uintptr_t stop=region_end<end?region_end:end;
        if (memory.State==MEM_COMMIT && !(memory.Protect&(PAGE_GUARD|PAGE_NOACCESS))
                && (memory.Protect&0xff)==PAGE_READWRITE) {
            for (uintptr_t address=(cursor+3u)&~(uintptr_t)3u;address+4<=stop;address+=4) {
                uint32_t value;memcpy(&value,(void *)address,4);
                block_candidate(value,search);
                if (indirect && value>=0x10000 && !(value&3u) && block_state->roots<4096) {
                    MEMORY_BASIC_INFORMATION root;
                    if (VirtualQuery((void *)(uintptr_t)value,&root,sizeof(root)) && root.State==MEM_COMMIT
                            && root.Type==MEM_PRIVATE && !(root.Protect&(PAGE_GUARD|PAGE_NOACCESS))
                            && (root.Protect&0xff)==PAGE_READWRITE && block_new_root(value)) {
                        /* A bounded prefix of a structure referenced by game data.
                           No process-wide heap scan or recursive pointer guessing. */
                        SIZE_T remaining=(uintptr_t)root.BaseAddress+root.RegionSize-value;
                        SIZE_T bytes=remaining<4096?remaining:4096;
                        block_state->roots++;
                        block_scan_range(value,bytes,search,0);
                    }
                }
            }
        }
        cursor=stop;
    }
}

static void block_poll(void) {
    if (!block_state || block_device || state->hook_ticks<120 || block_attempts>=5
            || (uint32_t)(GetTickCount()-block_poll_stamp)<2000) return;
    block_poll_stamp=GetTickCount();block_attempts++;
    memset(block_roots,0,sizeof(block_roots));
    block_state->slots=block_state->tables=block_state->devices=block_state->window_rejected=block_state->roots=0;
    block_state->factory_rejected=block_state->probe_valid=0;
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)image_base;
    IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)(image_base+dos->e_lfanew);
    if (!block_factory_slot) {
        unsigned char *call=unique_pattern(image_base,image_size,nt,block_factory_pattern,sizeof(block_factory_pattern)/sizeof(int));
        uint32_t slot=0;if (call) memcpy(&slot,call+6,4);
        if (slot>=(uintptr_t)image_base && slot<=(uintptr_t)image_base+image_size-4 && !(slot&3u))
            block_factory_slot=(const uint32_t *)(uintptr_t)slot;
    }
    uint32_t game_factory=block_factory_slot && readable(block_factory_slot,4)?*block_factory_slot:0;
    if (game_factory<0x10000 || (game_factory&3u) || !readable((void *)(uintptr_t)game_factory,4)) {
        block_state->status=3;block_state->attempts=block_attempts;return;
    }
    wchar_t path[MAX_PATH];UINT n=GetSystemDirectoryW(path,MAX_PATH);
    if (!n || n>MAX_PATH-12) return;
    memcpy(path+n,L"\\d3d9.dll",sizeof(L"\\d3d9.dll"));
    HMODULE library=LoadLibraryExW(path,NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) {block_state->status=2;block_state->attempts=block_attempts;return;}
    typedef IDirect3D9 *(WINAPI *Create)(UINT);
    Create create=(Create)GetProcAddress(library,"Direct3DCreate9");
    IDirect3D9 *d3d=create?create(D3D_SDK_VERSION):NULL;
    if (!d3d) {FreeLibrary(library);block_state->status=2;block_state->attempts=block_attempts;return;}
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
        IMAGE_DOS_HEADER *library_dos=(IMAGE_DOS_HEADER *)library;
        IMAGE_NT_HEADERS32 *library_nt=(IMAGE_NT_HEADERS32 *)((unsigned char *)library+library_dos->e_lfanew);
        BlockSearch search={probe,NULL,window,(uintptr_t)library,
            (uintptr_t)library+library_nt->OptionalHeader.SizeOfImage,0,(IDirect3D9 *)(uintptr_t)game_factory};
        block_state->probe_valid=(uint32_t)block_system_table(probe->lpVtbl,&search);
        IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);
        for (unsigned pass=0;block_state->probe_valid && pass<2 && !search.found;++pass) {
            for (unsigned s=0;s<nt->FileHeader.NumberOfSections;++s) {
                DWORD rva=sections[s].VirtualAddress,length=sections[s].Misc.VirtualSize;
                if (!(sections[s].Characteristics&IMAGE_SCN_MEM_WRITE) || (sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE)
                        || rva>=image_size || length>image_size-rva) continue;
                block_scan_range((uintptr_t)image_base+rva,length,&search,pass==1);
            }
        }
        found=search.found;ambiguous=search.ambiguous;
        if (found && !ambiguous && readable(block_factory_slot,4) && *block_factory_slot==game_factory) {
            const IDirect3DDevice9Vtbl *previous=found->lpVtbl;
            memcpy(&block_vtable,previous,sizeof(block_vtable));
            block_original_end=block_vtable.EndScene;block_vtable.EndScene=block_end_scene;
            block_device=found;
            if (InterlockedCompareExchangePointer((PVOID volatile *)found,(void *)&block_vtable,(void *)previous)==previous) {
                block_device=found;block_state->status=1;
            } else {block_device=NULL;block_state->status=4;}
        } else block_state->status=(ambiguous || found)?4:3;
        IDirect3DDevice9_Release(probe);
    } else {block_state->status=2;block_state->last_hresult=(uint32_t)hr;}
    if (window) DestroyWindow(window);
    if (atom) UnregisterClassW(wc.lpszClassName,wc.hInstance);
    IDirect3D9_Release(d3d);FreeLibrary(library);
    /* Commit the attempt counter last so the host reports completed scans. */
    MemoryBarrier();block_state->attempts=block_attempts;
}
static void block_initialize(void) {
    wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-BlockPreview-%lu",(unsigned long)GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(BlockShared),name);
    if (!mapping) return;
    if (GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(mapping);return;}
    block_state=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(BlockShared));
    if (!block_state) {CloseHandle(mapping);return;}
    block_mapping=mapping;memset(block_state,0,sizeof(*block_state));
    block_state->magic=0x52453442;block_state->version=3;
}
