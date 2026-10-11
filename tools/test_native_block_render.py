"""Compile the actual EndScene callback with synthetic COM calls on Linux.
This checks cleanup/control gates, not the Windows driver or device discovery.
"""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


class NativeBlockRenderTests(unittest.TestCase):
    def test_actual_callback_restores_state_and_never_draws_stale_or_stopped(self):
        root=Path(__file__).resolve().parents[1]
        source=(root/'native/block_render.h').read_text()
        body=source[source.index('static HRESULT WINAPI block_end_scene'):source.index('/* Exact device-vtable')]
        enums=sorted(set(re.findall(r'\bD3D[A-Z0-9_]+\b',body))-{'D3DVIEWPORT9'})
        defines='\n'.join(f'#define {name} {i+1}' for i,name in enumerate(enums))
        setters=sorted(set(re.findall(r'IDirect3DDevice9_(Set\w+)\(',body)))
        defines+='\n'+'\n'.join(f'#define IDirect3DDevice9_{name}(...) set_call()' for name in setters)
        prefix=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "control_logic.h"
#include "block_geometry.h"
#define WINAPI
#define VK_F8 119
#define MemoryBarrier() ((void)0)
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
typedef int32_t HRESULT;
typedef struct {int unused;} IDirect3DDevice9;
typedef struct {int saved;} IDirect3DStateBlock9;
typedef struct {unsigned X,Y,Width,Height;} D3DVIEWPORT9;
typedef struct {uint32_t stamp,armed;int32_t stop;} Shared;
typedef struct {uint32_t stamp,armed,generation;} ControlFrame;
typedef struct {uint32_t magic,version;int32_t sequence;uint32_t stamp,enabled,generation;
 float bounds[6];uint32_t status,frames,draws,failures,last_hresult;} BlockShared;
static Shared shared,*state=&shared;
static BlockShared block,*block_state=&block;
static IDirect3DDevice9 dev,*block_device=&dev;
static IDirect3DStateBlock9 backup;
static uint32_t global_address,*global_slot=&global_address;
static unsigned char camera_memory[1024];
static int gfx,sets,draws,applies,releases,original_calls,fail_set,fail_draw,fail_capture,complete,async_key;
static uint32_t generation=7;
static uint32_t GetTickCount(void) {return 1000;}
static int GetAsyncKeyState(int key) {(void)key;return async_key;}
static int32_t InterlockedExchange(int32_t *p,int32_t value) {int32_t old=*p;*p=value;return old;}
static int read_control_frame(ControlFrame *out) {*out=(ControlFrame){state->stamp,state->armed,generation};return complete;}
static int current_player(void) {return 1;}
static int readable(const void *p,unsigned length) {(void)length;return p!=NULL;}
static int set_call(void) {gfx++;return ++sets==fail_set?-1:0;}
static int get_viewport(IDirect3DDevice9 *d,D3DVIEWPORT9 *vp) {(void)d;*vp=(D3DVIEWPORT9){0,0,1920,1080};return 0;}
static int create_backup(IDirect3DDevice9 *d,int type,IDirect3DStateBlock9 **out) {(void)d;(void)type;*out=&backup;return 0;}
static int capture(IDirect3DStateBlock9 *s) {s->saved=gfx;return fail_capture?-1:0;}
static int apply(IDirect3DStateBlock9 *s) {gfx=s->saved;applies++;return 0;}
static int release(IDirect3DStateBlock9 *s) {(void)s;releases++;return 0;}
static int draw(IDirect3DDevice9 *d,int type,unsigned primitives,const void *v,unsigned stride) {
 (void)d;(void)type;(void)v;assert(primitives==12 && stride==20);draws++;return fail_draw?-1:0;
}
static HRESULT original_end(IDirect3DDevice9 *d) {(void)d;assert(gfx==67);original_calls++;return 123;}
static HRESULT (*block_original_end)(IDirect3DDevice9 *)=original_end;
#define IDirect3DDevice9_GetViewport get_viewport
#define IDirect3DDevice9_CreateStateBlock create_backup
#define IDirect3DStateBlock9_Capture capture
#define IDirect3DStateBlock9_Apply apply
#define IDirect3DStateBlock9_Release release
#define IDirect3DDevice9_DrawPrimitiveUP draw
'''
        tests=r'''
static void reset(void) {
 memset(&block,0,sizeof(block));state->armed=1;state->stop=0;state->stamp=1000;
 block.enabled=1;block.sequence=2;block.stamp=1000;block.generation=7;
 float b[6]={-500,0,-3500,500,1000,-2500};memcpy(block.bounds,b,sizeof(b));
 float view[12]={1,0,0,0,0,1,0,0,0,0,1,0};float fov=50;
 global_address=(uint32_t)(uintptr_t)camera_memory;
 memcpy(camera_memory+0x74+0x30,view,sizeof(view));memcpy(camera_memory+0x74+0xC0,&fov,4);
 gfx=67;sets=draws=applies=releases=original_calls=fail_set=fail_draw=fail_capture=async_key=0;complete=1;
}
int main(void) {
 reset();assert(block_end_scene(&dev)==123);assert(draws==1 && applies==1 && releases==1 && block.draws==1);
 int set_count=sets;
 for (int i=1;i<=set_count;++i) {reset();fail_set=i;assert(block_end_scene(&dev)==123);assert(applies==1 && releases==1 && block.failures==1);}
 reset();fail_draw=1;block_end_scene(&dev);assert(applies==1 && releases==1 && block.failures==1);
 reset();fail_capture=1;block_end_scene(&dev);assert(!draws && !applies && releases==1 && block.failures==1);
 for (int gate=0;gate<7;++gate) {
  reset();
  if (gate==0) state->armed=0;
  if (gate==1) state->stop=1;
  if (gate==2) block.stamp=500;
  if (gate==3) state->stamp=500;
  if (gate==4) block.generation=8;
  if (gate==5) block.sequence=3;
  if (gate==6) complete=0;
  assert(block_end_scene(&dev)==123);assert(!draws && !sets && original_calls==1);
 }
 reset();async_key=0x8000;block_end_scene(&dev);assert(state->stop==1 && !draws);
 puts("Actual EndScene callback: state restoration, failures, TTL, torn preview, generation, stop/F8 passed");
}
'''
        with tempfile.TemporaryDirectory() as directory:
            file=Path(directory)/'renderer.c';exe=Path(directory)/'renderer'
            file.write_text(prefix+'\n'+defines+'\n'+body+'\n'+tests)
            compiled=subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-no-pie','-I',str(root/'native'),str(file),'-lm','-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
            self.assertIn('Actual EndScene callback',result.stdout)
