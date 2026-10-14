"""Compile the actual capture helpers against bounded PE / simulated COM APIs.
Does not execute Windows, a real driver, or bio4.exe.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest


class NativeCaptureTests(unittest.TestCase):
    def test_render_debris_between_snapshots_preserves_colliders(self):
        self.compile_run('test_block_debris.c')

    def test_enemy_manager_discovery_accepts_one_shared_target(self):
        self.compile_run('test_runtime_discovery.c')

    def compile_run(self,name,extra=None):
        root=Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            source=root/'native'/name
            if extra is not None:
                source=Path(directory)/name
                source.write_text(extra)
            binary=Path(directory)/'fixture'
            try:
                subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-no-pie',
                                '-I'+str(root/'native'),str(source),'-o',str(binary),'-lm'],check=True,capture_output=True,text=True)
                subprocess.run([str(binary)],check=True,capture_output=True,text=True)
            except subprocess.CalledProcessError as error:self.fail(error.stdout+'\n'+error.stderr)

    def hud_fixture(self,extra_name):
        root=Path(__file__).resolve().parents[1]
        fixture=(root/'native/test_cube_draw.c').read_text().replace('uint32_t player;','uint32_t player,hud_generation,hud_blocked_generation;')
        fixture='#include "overlay_policy.h"\n#define HEAP_ZERO_MEMORY 8\n'+fixture
        fixture=fixture.replace('!flags && size==sizeof(BlockSurfaceMesh)', '(flags==8 || (!flags && size==sizeof(BlockSurfaceMesh)))')
        fixture=fixture.replace('return malloc(size);','return flags==8?calloc(1,size):malloc(size);')
        fixture=fixture.replace('height==width','height==32')
        fixture=fixture.replace('uint32_t player,hud_generation,hud_blocked_generation;',
            'uint32_t player,hud_generation,hud_blocked_generation;void *overlay;IDirect3DTexture9 *hud_texture;unsigned hud_pixels_valid;uint32_t hud_pixels[8192];')
        # Use the typed simulated driver with the actual HUD renderer.
        a=fixture.index('static HRESULT draw(');b=fixture.index('static IDirect3DDevice9Vtbl table',a)
        fixture=fixture[:a]+"""static HRESULT draw(IDirect3DDevice9 *d,D3DPRIMITIVETYPE type,UINT count,const void *data,UINT stride){
            (void)d;
            if(stride==sizeof(BlockVertex)){const BlockVertex *line=data;assert(type==D3DPT_TRIANGLELIST && count<=24);for(unsigned i=0;i<count*3;i++)assert(line[i].color==0xFF000000);draws++;return S_OK;}
            assert(type==D3DPT_TRIANGLELIST && (count==2 || count==4) && stride==sizeof(BlockSurfaceVertex));
            const BlockSurfaceVertex *v=data;for(unsigned i=0;i<count*3;i++)assert(isfinite(v[i].x+v[i].y) && v[i].x>=9.5f && v[i].x<=650 && v[i].y>=19.5f && v[i].y<=500);
            assert(current.render[D3DRS_ALPHABLENDENABLE]==1);draws++;current.stream=0;return draw_fail?E_FAIL:S_OK;
        }
        """+fixture[b:]
        fixture=fixture.replace('int main(void)', 'int cube_unused_fixture_main(void)')
        extra=(root/'native'/extra_name).read_text()
        ipc=(root/'native/hud_ipc.h').read_text();ipc=ipc[ipc.index('static int hud_command_read'):]
        return fixture+extra.replace('/* ACTUAL_HUD_READER */',ipc)

    def test_actual_hud_draw_restoration_and_failures(self):
        self.compile_run('hud_draw_fixture.c',self.hud_fixture('test_hud_draw_extra.c'))

    def test_actual_overlay_cached_image_selection_and_failure_restoration(self):
        self.compile_run('overlay_draw_fixture.c',self.hud_fixture('test_overlay_draw_extra.c'))

    def test_actual_camera_runtime_callback_order_and_gaze(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/re4_control.c').read_text()
        helpers=body[body.index('static uint32_t weapon_controller'):body.index('/* Restore before native logic.')]
        camera=body[body.index("static int cube_camera"):body.index("/* Creation-owned facades")];
        body=helpers+body[body.index('#define RE4_COCKPIT_CLASSES'):body.index('/* Literal bytes')]
        fixture=(root/'native/test_camera_runtime.c').read_text()
        self.compile_run('camera_runtime.c',fixture.replace('/* ACTUAL_CAMERA_RUNTIME */',(body+camera).replace('#include "health_ipc.h"','')))

    def test_observed_virtual_four_three_projection_matches_block_and_outline(self):
        self.compile_run("test_block_projection.c")

    def test_actual_native_depth_draw_and_restore(self):
        self.compile_run("test_depth_draw.c")

    def test_native_depth_projection_and_reverse_z(self):
        self.compile_run("test_block_depth.c")

    def test_filled_surface_clipping_ordering_and_bounds(self):
        self.compile_run("test_block_surface.c")

    def test_horizontal_collision_geometry(self):
        self.compile_run("test_block_collision.c")

    def test_native_runtime_scene_cache_and_action_publication(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/runtime_ipc.h').read_text()
        prefix=body[:body.index('static void runtime_ipc_open')].replace('static HANDLE runtime_mapping;','').replace('static uint32_t enemy_manager,','static uint32_t ')
        action=body[body.index('static void runtime_action_restore'):]
        fixture=(root/'native/test_runtime.c').read_text().replace('/* ACTUAL_RUNTIME */',prefix+action)
        self.compile_run('runtime.c',fixture)

    def test_enemy_pool_across_committed_regions(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/re4_control.c').read_text();body=body[body.index('static int readable_span'):body.index('/* Only readable committed memory;')]
        fixture="""
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef size_t SIZE_T;
typedef struct {void *BaseAddress;SIZE_T RegionSize;unsigned State,Protect;} MEMORY_BASIC_INFORMATION;
#define MEM_COMMIT 1
#define PAGE_GUARD 2
#define PAGE_NOACCESS 4
#define PAGE_READONLY 8
#define PAGE_READWRITE 16
#define PAGE_WRITECOPY 32
#define PAGE_EXECUTE_READ 64
#define PAGE_EXECUTE_READWRITE 128
#define PAGE_EXECUTE_WRITECOPY 256
static unsigned deny,queries;
static int VirtualQuery(void *p,MEMORY_BASIC_INFORMATION *m,SIZE_T size){(void)size;queries++;uintptr_t n=(uintptr_t)p;m->BaseAddress=(void*)(n&~(uintptr_t)4095);m->RegionSize=4096;m->State=MEM_COMMIT;m->Protect=deny && n>=8192?PAGE_NOACCESS:PAGE_READWRITE;return 1;}
"""+body+"""
int main(void){assert(readable_span((void*)5000,8000));assert(queries==3);deny=1;assert(!readable_span((void*)5000,8000));assert(!readable_span((void*)(UINTPTR_MAX-5),10));assert(!readable_span((void*)5000,4096*66));}
"""
        self.compile_run('enemy-memory.c',fixture)

    def test_native_enemy_callback_and_repeated_walls(self):
        self.compile_run('test_enemy_collision.c')

    def test_health_cumulative_damage_and_death(self):
        self.compile_run('test_health.c')

    def test_actual_health_callback_validates_writes_and_retains_death(self):
        root=Path(__file__).resolve().parents[1]
        source=(root/'native/health_ipc.h').read_text()
        declarations=source[:source.index('static void health_ipc_open')].replace('static HANDLE health_mapping;','')
        callback=source[source.index('static void health_tick'):]
        fixture=(root/'native/test_health_tick.c').read_text().replace('/* ACTUAL_HEALTH_TYPES */',declarations).replace('/* ACTUAL_HEALTH_TICK */',callback)
        self.compile_run('health_tick.c',fixture)

    def test_actual_collision_callback_guards_and_position_writes(self):
        self.compile_run("test_collision_tick.c")

    def test_offscreen_render_scene_keeps_actual_collision_reference_live(self):
        self.compile_run("test_offscreen_collision.c")

    def test_bounded_import_lookup(self):
        self.compile_run('test_capture_import.c')

    def test_factory_forwarding_and_lifetime(self):
        self.compile_run('test_capture_factory.c')

    def test_device_facade_all_methods_and_lifetime(self):
        self.compile_run('test_cube_device.c')

    def test_cube_render_state_failures_and_anchor(self):
        self.compile_run('test_cube_draw.c')

    def test_cube_command_actual_reader_rejects_torn_publications(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/cube_ipc.h').read_text()
        body=body[body.index('static int cube_command_requested'):body.index('static void cube_record_wrap')]
        fixture=(root/'native/test_cube_command.c').read_text()
        self.compile_run('cube_command.c',fixture.replace('/* ACTUAL_READER */',body))

    def test_actual_import_install_and_rollback(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/d3d9_capture.h').read_text()
        body=body[body.index('static const unsigned char *capture_image_read'):]
        fixture=(root/'native/test_capture_install.c').read_text()
        self.compile_run('capture_install.c',fixture.replace('/* ACTUAL_INSTALL */',body))


if __name__=='__main__':unittest.main()
