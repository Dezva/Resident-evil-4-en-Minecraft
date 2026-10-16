"""Exercise imports after overlaying a release onto timestamp-valid old caches."""
import importlib.util
import json
import os
from pathlib import Path
import py_compile
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile


class HostfixPackageTests(unittest.TestCase):
    def test_patch_invalidates_original_055_bytecode_and_preserves_other_files(self):
        root=Path(__file__).resolve().parents[1]
        release=root/'releases/0.4.55-hostfix2'
        names={'tools/re4_runtime.py':0x52453452,'tools/re4_combat.py':0x52453446}
        with zipfile.ZipFile(root/'releases/0.4.55/re4bridge-0.4.55-windows.zip') as original:
            with tempfile.TemporaryDirectory() as directory:
                target=Path(directory)
                for name in names:
                    file=target/Path(name).name
                    file.write_bytes(original.read(name))
                    stamp=time.mktime((*original.getinfo(name).date_time,0,0,-1))
                    os.utime(file,(stamp,stamp))
                    py_compile.compile(str(file),doraise=True)
                    self.assertTrue(Path(importlib.util.cache_from_source(str(file))).is_file())
                with zipfile.ZipFile(release/'re4bridge-0.4.55-hostfix2.zip') as patch:
                    for name in names:
                        file=target/Path(name).name
                        file.write_bytes(patch.read(name))
                        stamp=time.mktime((*patch.getinfo(name).date_time,0,0,-1))
                        os.utime(file,(stamp,stamp))
                code=('import sys,json;sys.path[:0]=sys.argv[1:];'
                      'import re4_runtime,re4_combat;'
                      'print(json.dumps([re4_runtime.MAGIC,re4_combat.MAGIC]))')
                result=subprocess.run([sys.executable,'-c',code,str(target),str(root/'tools')],
                                      check=True,capture_output=True,text=True,timeout=10)
                self.assertEqual(json.loads(result.stdout),list(names.values()))
            with zipfile.ZipFile(release/'re4bridge-0.4.55-hostfix2-windows.zip') as full:
                changed={name for name in original.namelist() if original.read(name)!=full.read(name)}
                self.assertEqual(changed,set(names))
                self.assertEqual(set(full.namelist()),set(original.namelist())|{'HOSTFIX2.txt'})
                for name in names:
                    self.assertNotEqual(full.getinfo(name).date_time,original.getinfo(name).date_time)


if __name__=='__main__':unittest.main()
