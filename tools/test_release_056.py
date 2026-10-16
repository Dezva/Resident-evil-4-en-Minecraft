"""Verify the delivered binaries, source boundary and cached-host upgrade."""
import hashlib
import json
import os
from pathlib import Path
import py_compile
import struct
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile


class Release056Tests(unittest.TestCase):
    def test_delivered_components_and_upgrade_from_original_055_cache(self):
        root=Path(__file__).resolve().parents[1]
        release=root/'releases/0.4.56'
        with zipfile.ZipFile(release/'re4bridge-0.4.56-windows.zip') as archive, zipfile.ZipFile(release/'re4bridge-0.4.56-source.zip') as source:
            names=archive.namelist()
            self.assertFalse(any(Path(n).name.lower() in {'bio4.exe','re4bridge.json'} or n.endswith(('.sav','.pyc','.log')) for n in names))
            dll=archive.read('native/dinput8.dll')
            pe=struct.unpack_from('<I',dll,0x3c)[0]
            self.assertEqual(dll[pe:pe+4],b'PE\0\0')
            self.assertEqual(struct.unpack_from('<H',dll,pe+4)[0],0x14c)
            self.assertEqual(hashlib.sha256(dll).hexdigest(),archive.read('NATIVE-SHA256.txt').decode().split()[0])
            with tempfile.TemporaryDirectory() as directory:
                stage=Path(directory)
                with zipfile.ZipFile(root/'releases/0.4.55/re4bridge-0.4.55-windows.zip') as old:
                    for name in ('tools/re4_combat.py','tools/re4_runtime.py'):
                        path=stage/name
                        path.parent.mkdir(exist_ok=True)
                        path.write_bytes(old.read(name))
                        stamp=time.mktime((*old.getinfo(name).date_time,0,0,-1))
                        os.utime(path,(stamp,stamp));py_compile.compile(str(path),doraise=True)
                for name in names:
                    if name.startswith('tools/') and name.endswith('.py'):
                        path=stage/name;path.write_bytes(archive.read(name))
                        self.assertEqual(path.read_bytes().replace(b'\r\n',b'\n'),source.read(name).replace(b'\r\n',b'\n'),name)
                        stamp=time.mktime((*archive.getinfo(name).date_time,0,0,-1));os.utime(path,(stamp,stamp))
                # Current independent ABI fixtures exercise the packaged modules in a new
                # interpreter. Their original timestamp-valid cache must not survive.
                for name in source.namelist():
                    if name.startswith('tools/test_') and name.endswith('.py'):(stage/name).write_bytes(source.read(name))
                code='import sys,unittest;sys.path[:0]=sys.argv[1:];s=unittest.defaultTestLoader.loadTestsFromNames(["test_combat","test_runtime"]);sys.exit(not unittest.TextTestRunner().run(s).wasSuccessful())'
                result=subprocess.run([sys.executable,'-c',code,str(stage/'tools')],capture_output=True,text=True,timeout=20)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                jar=stage/'mod.jar';jar.write_bytes(archive.read('mods/re4bridge-0.4.56.jar'))
                with zipfile.ZipFile(jar) as mod:
                    self.assertEqual(json.loads(mod.read('fabric.mod.json'))['version'],'0.4.56')
                    self.assertTrue(any(n.endswith('/BridgeProjectileRenderer.class') for n in mod.namelist()))
                    self.assertFalse(any(n.startswith('net/minecraft/') for n in mod.namelist()))
        with zipfile.ZipFile(release/'re4bridge-0.4.56-source.zip') as source:
            self.assertFalse(any(set(Path(n).parts)&{'.gradle','.git','build','__pycache__'} for n in source.namelist()))
            self.assertIn('tools/build_native.ps1',source.namelist())
            self.assertIn('tools/test_release_056.py',source.namelist())


if __name__=='__main__':unittest.main()
