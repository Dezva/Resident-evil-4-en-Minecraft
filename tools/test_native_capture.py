"""Compile the actual capture helpers against bounded PE / simulated COM APIs.
Does not execute Windows, a real driver, or bio4.exe.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest


class NativeCaptureTests(unittest.TestCase):
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

    def test_actual_hud_draw_restoration_and_failures(self):
        root=Path(__file__).resolve().parents[1]
        fixture=(root/'native/test_cube_draw.c').read_text().replace('uint32_t player;','uint32_t player,hud_generation,hud_blocked_generation;')
        fixture=fixture.replace('height==width','height==32')
        fixture=fixture.replace('uint32_t player,hud_generation,hud_blocked_generation;',
            'uint32_t player,hud_generation,hud_blocked_generation;IDirect3DTexture9 *hud_texture;unsigned hud_pixels_valid;uint32_t hud_pixels[8192];')
        # Use the typed simulated driver with the actual HUD renderer.
        a=fixture.index('static HRESULT draw(');b=fixture.index('static IDirect3DDevice9Vtbl table',a)
        fixture=fixture[:a]+"""static HRESULT draw(IDirect3DDevice9 *d,D3DPRIMITIVETYPE type,UINT count,const void *data,UINT stride){
            (void)d;assert(type==D3DPT_TRIANGLELIST && (count==2 || count==4) && stride==sizeof(BlockSurfaceVertex));
            const BlockSurfaceVertex *v=data;for(unsigned i=0;i<count*3;i++)assert(isfinite(v[i].x+v[i].y) && v[i].x>=10 && v[i].x<=650 && v[i].y>=20 && v[i].y<=500);
            assert(current.render[D3DRS_ALPHABLENDENABLE]==1);draws++;current.stream=0;return draw_fail?E_FAIL:S_OK;
        }
        """+fixture[b:]
        fixture=fixture.replace('int main(void)', 'void cube_unused_fixture_main(void)')
        extra=(root/'native/test_hud_draw_extra.c').read_text()
        ipc=(root/'native/hud_ipc.h').read_text();ipc=ipc[ipc.index('static int hud_command_read'):]
        self.compile_run('hud_draw_fixture.c',fixture+extra.replace('/* ACTUAL_HUD_READER */',ipc))

    def test_actual_camera_runtime_callback_order_and_gaze(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/re4_control.c').read_text()
        helpers=body[body.index('static uint32_t weapon_controller'):body.index('/* Restore before native logic.')]
        camera=body[body.index("static int cube_camera"):body.index("/* Creation-owned facades")];
        body=helpers+body[body.index('static void native_hud_restore'):body.index('/* Literal bytes')]
        fixture=(root/'native/test_camera_runtime.c').read_text()
        self.compile_run('camera_runtime.c',fixture.replace('/* ACTUAL_CAMERA_RUNTIME */',body+camera))

    def test_filled_surface_clipping_ordering_and_bounds(self):
        self.compile_run("test_block_surface.c")

    def test_horizontal_collision_geometry(self):
        self.compile_run("test_block_collision.c")

    def test_actual_collision_callback_guards_and_position_writes(self):
        self.compile_run("test_collision_tick.c")

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
