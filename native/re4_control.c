/* Independent RE4 UHD 1.1.0 input bridge; public pattern references in README.md. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "control_logic.h"
#include "branch_logic.h"

typedef struct {
    uint32_t magic, version;
    volatile LONG sequence;
    volatile uint32_t stamp, armed, buttons;
    volatile LONG stop, status, hook_ticks;
    volatile uint32_t native_buttons;
} Shared;
_Static_assert(sizeof(Shared) == 40, "IPC layout");
static Shared *state;
/* Retain the named mapping handle until process exit, independently of its view. */
static HANDLE shared_mapping;
static HMODULE system_dinput;
static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
static void (__cdecl *original_pad)(void);
static volatile uint64_t *key_on;

/* Only readable committed memory; never infer fallback addresses. */
static int readable(const void *pointer, SIZE_T length) {
    MEMORY_BASIC_INFORMATION m;
    if (!pointer || !VirtualQuery(pointer, &m, sizeof(m)) || m.State != MEM_COMMIT
            || (m.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    DWORD p = m.Protect & 0xff;
    if (p != PAGE_READONLY && p != PAGE_READWRITE && p != PAGE_WRITECOPY
            && p != PAGE_EXECUTE_READ && p != PAGE_EXECUTE_READWRITE && p != PAGE_EXECUTE_WRITECOPY) return 0;
    return (uintptr_t)pointer >= (uintptr_t)m.BaseAddress
        && length <= m.RegionSize - ((uintptr_t)pointer - (uintptr_t)m.BaseAddress);
}

static void __cdecl pad_hook(void) {
    original_pad();
    InterlockedIncrement(&state->hook_ticks);
    if (GetAsyncKeyState(VK_F8) & 0x8000) InterlockedExchange(&state->stop, 1);
    LONG before = state->sequence;
    MemoryBarrier();
    uint32_t stamp = state->stamp, armed = state->armed, buttons = state->buttons;
    MemoryBarrier();
    LONG after = state->sequence;
    uint32_t now = GetTickCount();
    if (before == after && !(before & 1) && armed && (uint32_t)(now - stamp) >= RE4_TTL_MS)
        InterlockedCompareExchange(&state->stop, 2, 0);
    if (before == after && !(before & 1) && armed && !state->stop && (uint32_t)(now - stamp) < RE4_TTL_MS) {
        uint32_t input = re4_buttons((uint32_t)before, (uint32_t)after, now, stamp, armed, (uint32_t)state->stop, buttons);
        *key_on = (*key_on & ~(uint64_t)RE4_MOVEMENT_MASK) | input;
        state->native_buttons = input;
    } else state->native_buttons = 0;
}

/* Literal bytes with -1 wildcards; these are interoperable signatures, not absolute addresses. */
static const int version_pattern[] = {0x68,-1,-1,-1,-1,0x8B,0xC8,0xE8,-1,-1,-1,-1,0xDC,0x2D,-1,-1,-1,-1,0xA1,-1,-1,-1,-1,0xDC,0x25};
static const int key_pattern[] = {0xA1,-1,-1,-1,-1,0x33,0xC9,0x83,0xE0,-1,0x83,0x3D};
static const int pad_pattern[] = {0xE8,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0x68,-1,-1,-1,-1,0x6A,0x04,0xE8,-1,-1,-1,-1,0x83,0xC4,0x18,0xE8,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0xA1};

static unsigned char *unique_pattern(unsigned char *base, DWORD image_size,
        IMAGE_NT_HEADERS32 *nt, const int *pattern, SIZE_T length) {
    unsigned char *found = NULL;
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    if (!readable(sections, nt->FileHeader.NumberOfSections * sizeof(*sections))) return NULL;
    for (unsigned s = 0; s < nt->FileHeader.NumberOfSections; ++s) {
        DWORD rva = sections[s].VirtualAddress, size = sections[s].Misc.VirtualSize;
        if (!(sections[s].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        if (rva >= image_size || size > image_size - rva || size > 32u*1024u*1024u || size < length
                || !readable(base + rva, size)) return NULL;
        for (SIZE_T i = 0; i <= size - length; ++i) {
            if (base[rva + i] != pattern[0]) continue;
            SIZE_T j;
            for (j = 1; j < length; ++j) if (pattern[j] >= 0 && base[rva+i+j] != pattern[j]) break;
            if (j == length) { if (found) return NULL; found = base+rva+i; }
        }
    }
    return found;
}

static void install_hook(void) {
    wchar_t name[80];
    swprintf(name, 80, L"Local\\RE4Bridge-Control-%lu", (unsigned long)GetCurrentProcessId());
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Shared), name);
    if (!mapping) return;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mapping); return; }
    state = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!state) { CloseHandle(mapping); return; }
    shared_mapping = mapping;
    memset(state, 0, sizeof(*state)); state->magic = RE4_MAGIC; state->version = 1; state->status = 2;
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!readable(base, sizeof(IMAGE_DOS_HEADER))) return;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 64 || dos->e_lfanew > 1024*1024) return;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
    if (!readable(nt, sizeof(*nt)) || nt->Signature != IMAGE_NT_SIGNATURE
            || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
            || nt->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER32)
            || nt->FileHeader.NumberOfSections > 96) return;
    DWORD size = nt->OptionalHeader.SizeOfImage;
    if (size < 4096 || size > 128u*1024u*1024u) return;
    unsigned char *v = unique_pattern(base,size,nt,version_pattern,sizeof(version_pattern)/sizeof(int));
    unsigned char *k = unique_pattern(base,size,nt,key_pattern,sizeof(key_pattern)/sizeof(int));
    unsigned char *call = unique_pattern(base,size,nt,pad_pattern,sizeof(pad_pattern)/sizeof(int));
    if (!v || !k || !call) return;
    uint32_t version_address, keys_address;
    memcpy(&version_address,v+1,4); memcpy(&keys_address,k+1,4);
    if (version_address < (uintptr_t)base || version_address > (uintptr_t)base+size-6
            || !readable((void *)(uintptr_t)version_address,6) || memcmp((void *)(uintptr_t)version_address,"1.1.0",6)) return;
    if (keys_address < (uintptr_t)base || keys_address > (uintptr_t)base+size-8
            || !readable((void *)(uintptr_t)keys_address,8)) return;
    MEMORY_BASIC_INFORMATION key_memory;
    VirtualQuery((void *)(uintptr_t)keys_address,&key_memory,sizeof(key_memory));
    if (!(key_memory.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) return;
    int32_t displacement;
    memcpy(&displacement,call+1,4);
    uintptr_t wrapper = (uintptr_t)(call+5) + displacement;
    if (wrapper < (uintptr_t)base || wrapper > (uintptr_t)base+size-5 || !readable((void *)wrapper,5)) return;
    /* The observed UHD entry is an E9 tail thunk. Preserve CALL/JMP semantics. */
    call = (unsigned char *)wrapper;
    uint32_t target_rva;
    if (!re4_relative_target(call, (uint32_t)(wrapper - (uintptr_t)base), size, &target_rva)) return;
    uintptr_t target = (uintptr_t)base + target_rva;
    if (!readable((void *)target,1)) return;
    MEMORY_BASIC_INFORMATION code_memory;
    VirtualQuery((void *)target,&code_memory,sizeof(code_memory));
    if (!(code_memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return;
    original_pad = (void (__cdecl *)(void))target;
    key_on = (volatile uint64_t *)(uintptr_t)keys_address;
    DWORD old_protect;
    state->status = 3;
    if (!VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&old_protect)) return;
    int32_t replacement = (int32_t)((uintptr_t)pad_hook - (uintptr_t)(call+5));
    memcpy(call+1,&replacement,4);
    FlushInstructionCache(GetCurrentProcess(),call,5);
    DWORD ignored;
    if (!VirtualProtect(call,5,old_protect,&ignored)) return;
    state->status = 1;
}

static BOOL CALLBACK initialize(PINIT_ONCE unused, PVOID parameter, PVOID *context) {
    (void)unused; (void)parameter; (void)context;
    wchar_t path[MAX_PATH];
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (!length || length > MAX_PATH-14) return FALSE;
    memcpy(path+length,L"\\dinput8.dll",sizeof(L"\\dinput8.dll"));
    system_dinput = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return system_dinput != NULL;
}
static FARPROC system_export(const char *name) {
    if (!InitOnceExecuteOnce(&once,initialize,NULL,NULL)) return NULL;
    return GetProcAddress(system_dinput,name);
}

__declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version,
        const IID *iid, LPVOID *output, LPVOID outer) {
    typedef HRESULT (WINAPI *Create)(HINSTANCE,DWORD,const IID *,LPVOID *,LPVOID);
    Create original = (Create)system_export("DirectInput8Create");
    if (!original) return E_FAIL;
    HRESULT result = original(instance,version,iid,output,outer);
    static LONG installed;
    if (SUCCEEDED(result) && InterlockedCompareExchange(&installed,1,0) == 0) install_hook();
    return result;
}
__declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void) { return S_FALSE; }
__declspec(dllexport) HRESULT WINAPI DllGetClassObject(const IID *clsid,const IID *iid,LPVOID *output) {
    typedef HRESULT (WINAPI *GetClass)(const IID *,const IID *,LPVOID *);
    GetClass original = (GetClass)system_export("DllGetClassObject");
    return original ? original(clsid,iid,output) : E_FAIL;
}
__declspec(dllexport) HRESULT WINAPI DllRegisterServer(void) { return E_NOTIMPL; }
__declspec(dllexport) HRESULT WINAPI DllUnregisterServer(void) { return E_NOTIMPL; }
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(instance);
    return TRUE;
}
