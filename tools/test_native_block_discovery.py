"""Exercise the actual device validation/root traversal against synthetic memory.
No Windows driver is used; these are rejection and discovery regression checks.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest


class NativeBlockDiscoveryTests(unittest.TestCase):
    def test_actual_discovery_variants_roots_split_regions_and_rejections(self):
        root=Path(__file__).resolve().parents[1]
        source=(root/'native/block_render.h').read_text()
        body=source[source.index('typedef struct {\n    IDirect3DDevice9 *probe'):source.index('static void block_poll(void) {')]
        prefix=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define MEM_COMMIT 0x1000
#define MEM_PRIVATE 0x20000
#define MEM_IMAGE 0x1000000
#define PAGE_READWRITE 4
#define PAGE_READONLY 2
#define PAGE_NOACCESS 1
#define PAGE_GUARD 0x100
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define FAILED(hr) ((hr)<0)
typedef size_t SIZE_T;
typedef unsigned DWORD;
typedef uintptr_t HWND;
typedef struct {void *BaseAddress,*AllocationBase;size_t RegionSize;DWORD State,Protect,Type;} MEMORY_BASIC_INFORMATION;
typedef struct {HWND hFocusWindow;} D3DDEVICE_CREATION_PARAMETERS;
typedef struct IDirect3DDevice9 IDirect3DDevice9;
typedef struct {unsigned releases;} IDirect3D9;
typedef struct {
 int (*GetCreationParameters)(IDirect3DDevice9 *,D3DDEVICE_CREATION_PARAMETERS *);
 int (*GetDirect3D)(IDirect3DDevice9 *,IDirect3D9 **);
 void (*GetDeviceCaps)(void),(*rest[116])(void);
} IDirect3DDevice9Vtbl;
struct IDirect3DDevice9 {const IDirect3DDevice9Vtbl *lpVtbl;HWND window;IDirect3D9 *factory;};
static struct {unsigned slots,tables,devices,window_rejected,roots,factory_rejected;} shared,*block_state=&shared;
static uintptr_t block_roots[8192];
static MEMORY_BASIC_INFORMATION regions[32];static unsigned region_count;
static uintptr_t library_start;static unsigned introspections;
static void code(void) {}
static void alternate(void) {}
static void foreign(void) {}
static void add(void *p,size_t size,DWORD protection,DWORD type) {
 regions[region_count++]=(MEMORY_BASIC_INFORMATION){p,p,size,MEM_COMMIT,protection,type};
}
static int get_direct(IDirect3DDevice9 *,IDirect3D9 **);
static int VirtualQuery(const void *p,MEMORY_BASIC_INFORMATION *out,size_t size) {
 (void)size;uintptr_t addr=(uintptr_t)p;
 if (addr==(uintptr_t)get_direct || addr==(uintptr_t)code || addr==(uintptr_t)alternate || addr==(uintptr_t)foreign) {
  *out=(MEMORY_BASIC_INFORMATION){(void *)p,(void *)(addr==(uintptr_t)foreign?123:library_start),1,MEM_COMMIT,PAGE_EXECUTE_READ,MEM_IMAGE};return 1;
 }
 for (unsigned i=0;i<region_count;++i) if (addr>=(uintptr_t)regions[i].BaseAddress && addr<(uintptr_t)regions[i].BaseAddress+regions[i].RegionSize) {*out=regions[i];return 1;}
 return 0;
}
static int readable(const void *p,size_t size) {
 MEMORY_BASIC_INFORMATION m;if (!VirtualQuery(p,&m,sizeof(m)) || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return 0;
 return size<=m.RegionSize-((uintptr_t)p-(uintptr_t)m.BaseAddress);
}
static int writable(const void *p,size_t size) {
 MEMORY_BASIC_INFORMATION m;return readable(p,size) && VirtualQuery(p,&m,sizeof(m)) && m.Protect==PAGE_READWRITE;
}
static int creation(IDirect3DDevice9 *device,D3DDEVICE_CREATION_PARAMETERS *out) {introspections++;out->hFocusWindow=device->window;return 0;}
static int get_direct(IDirect3DDevice9 *device,IDirect3D9 **out) {*out=device->factory;return 0;}
static int different_creation(IDirect3DDevice9 *device,D3DDEVICE_CREATION_PARAMETERS *out) {(void)device;(void)out;assert(0);return -1;}
static unsigned GetWindowThreadProcessId(HWND window,DWORD *pid) {*pid=window==2?999:42;return 567;}
static unsigned GetCurrentProcessId(void) {return 42;}
static int IsWindowVisible(HWND window) {return window!=3;}
#define IDirect3DDevice9_GetDirect3D(d,c) ((d)->lpVtbl->GetDirect3D(d,c))
#define IDirect3D9_Release(f) ((f)->releases++)
#define IDirect3DDevice9_GetCreationParameters(d,c) ((d)->lpVtbl->GetCreationParameters(d,c))
static IDirect3DDevice9Vtbl tables[3],private_table;
static IDirect3D9 game_factory,foreign_factory;
static IDirect3DDevice9 probe,game,other;
static uint32_t roots[8],structure[1080];
'''
        # Introspection function itself must also be classified as system code.
        prefix=prefix.replace('addr==(uintptr_t)code ||', 'addr==(uintptr_t)creation || addr==(uintptr_t)code ||')
        prefix=prefix.replace('static int VirtualQuery(', 'static int creation(IDirect3DDevice9 *,D3DDEVICE_CREATION_PARAMETERS *);\nstatic int VirtualQuery(')
        suffix=r'''
static BlockSearch reset(void) {
 region_count=introspections=0;game_factory.releases=foreign_factory.releases=0;memset(&shared,0,sizeof(shared));memset(block_roots,0,sizeof(block_roots));
 memset(roots,0,sizeof(roots));memset(structure,0,sizeof(structure));
 for (unsigned t=0;t<3;++t) {
  tables[t].GetCreationParameters=creation;tables[t].GetDirect3D=get_direct;tables[t].GetDeviceCaps=code;
  for (unsigned i=0;i<116;++i) tables[t].rest[i]=code;
 }
 tables[1].rest[100]=alternate; // Valid device variant: a different method table.
 library_start=(uintptr_t)tables;
 add(tables,sizeof(tables),PAGE_READONLY,MEM_IMAGE);
 probe=(IDirect3DDevice9){&tables[0],100,&game_factory};game=(IDirect3DDevice9){&tables[1],1,&game_factory};other=(IDirect3DDevice9){&tables[1],1,&game_factory};
 private_table=tables[1];add(&private_table,sizeof(private_table),PAGE_READWRITE,MEM_PRIVATE);
 add(&probe,sizeof(probe),PAGE_READWRITE,MEM_PRIVATE);add(&game,sizeof(game),PAGE_READWRITE,MEM_PRIVATE);add(&other,sizeof(other),PAGE_READWRITE,MEM_PRIVATE);
 return (BlockSearch){&probe,NULL,100,(uintptr_t)tables,(uintptr_t)tables+sizeof(tables),0,&game_factory};
}
int main(void) {
 BlockSearch s=reset();block_candidate((uintptr_t)&game,&s);
 assert(s.found==&game && shared.devices==1 && introspections==1); // Window thread need not be pad thread.
 block_candidate((uintptr_t)&game,&s);assert(!s.ambiguous);
 block_candidate((uintptr_t)&other,&s);assert(s.ambiguous);
 s=reset();game.lpVtbl=&private_table;block_candidate((uintptr_t)&game,&s);assert(s.found==&game && game_factory.releases==1); // Private storage with validated system methods.
 s=reset();game.factory=&foreign_factory;block_candidate((uintptr_t)&game,&s);assert(!s.found && shared.factory_rejected==1 && foreign_factory.releases==1);
 s=reset();private_table.rest[15]=foreign;game.lpVtbl=&private_table;block_candidate((uintptr_t)&game,&s);assert(!s.found && !introspections);
 s=reset();private_table.GetDeviceCaps=alternate;game.lpVtbl=&private_table;block_candidate((uintptr_t)&game,&s);assert(!s.found && !introspections); // All executable methods alone are insufficient.
 s=reset();tables[1].GetCreationParameters=different_creation;block_candidate((uintptr_t)&game,&s);assert(!s.found && !introspections);
 s=reset();tables[1].rest[15]=foreign;block_candidate((uintptr_t)&game,&s);assert(!s.found && !introspections);
 s=reset();game.window=2;block_candidate((uintptr_t)&game,&s);assert(!s.found && shared.window_rejected==1);
 s=reset();game.window=3;block_candidate((uintptr_t)&game,&s);assert(!s.found && shared.window_rejected==1);
 s=reset();block_candidate((uintptr_t)&probe,&s);block_candidate(0,&s);block_candidate((uintptr_t)&game+1,&s);assert(!introspections);
 s=reset();roots[4]=(uint32_t)(uintptr_t)&game;
 add(roots,16,PAGE_READWRITE,MEM_IMAGE);add(roots+4,16,PAGE_READWRITE,MEM_IMAGE);
 block_scan_range((uintptr_t)roots,sizeof(roots),&s,0);assert(s.found==&game); // Split writable regions.
 s=reset();roots[4]=(uint32_t)(uintptr_t)&game;
 add(roots,16,PAGE_READWRITE,MEM_IMAGE);add(roots+4,16,PAGE_READWRITE|PAGE_GUARD,MEM_IMAGE);
 block_scan_range((uintptr_t)roots,sizeof(roots),&s,0);assert(!s.found && !introspections);
 s=reset();game.lpVtbl=&private_table;roots[0]=roots[1]=(uint32_t)(uintptr_t)structure;structure[0xc30/4]=(uint32_t)(uintptr_t)&game;
 add(roots,sizeof(roots),PAGE_READWRITE,MEM_IMAGE);add(structure,sizeof(structure),PAGE_READWRITE,MEM_PRIVATE);
 block_scan_range((uintptr_t)roots,sizeof(roots),&s,1);assert(s.found==&game && shared.roots==1); // One-hop root, duplicate skipped.
 s=reset();roots[0]=(uint32_t)(uintptr_t)structure;structure[1030]=(uint32_t)(uintptr_t)&game;
 add(roots,sizeof(roots),PAGE_READWRITE,MEM_IMAGE);add(structure,sizeof(structure),PAGE_READWRITE,MEM_PRIVATE);
 block_scan_range((uintptr_t)roots,sizeof(roots),&s,1);assert(!s.found && shared.roots==1); // Bounded structure scan.
 puts("Actual discovery: device variants, thread ownership, split regions, rooted pointers, ambiguity and foreign/guard rejection passed");
}
'''
        with tempfile.TemporaryDirectory() as directory:
            file=Path(directory)/'discovery.c';exe=Path(directory)/'discovery'
            file.write_text(prefix+'\n'+body+'\n'+suffix)
            build=subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-no-pie',str(file),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(build.returncode,0,build.stderr)
            result=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertIn('Actual discovery',result.stdout)
