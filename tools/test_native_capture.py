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
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-no-pie',
                            '-I'+str(root/'native'),str(source),'-o',str(binary)],check=True,capture_output=True,text=True)
            subprocess.run([str(binary)],check=True,capture_output=True,text=True)

    def test_bounded_import_lookup(self):
        self.compile_run('test_capture_import.c')

    def test_factory_forwarding_and_lifetime(self):
        self.compile_run('test_capture_factory.c')

    def test_actual_import_install_and_rollback(self):
        root=Path(__file__).resolve().parents[1]
        body=(root/'native/d3d9_capture.h').read_text()
        body=body[body.index('static const unsigned char *capture_image_read'):]
        fixture=(root/'native/test_capture_install.c').read_text()
        self.compile_run('capture_install.c',fixture.replace('/* ACTUAL_INSTALL */',body))


if __name__=='__main__':unittest.main()
