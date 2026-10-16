"""Verify the exact download, native identity, private-file boundary and host upgrade."""
import hashlib,io,json,os,py_compile,struct,subprocess,sys,tempfile,time,unittest,zipfile
from pathlib import Path
class Release057Tests(unittest.TestCase):
 def test_verified_complete_package_and_old_bytecode_upgrade(self):
  root=Path(__file__).resolve().parents[1];folder=root/'releases/0.4.57'
  if not folder.exists():folder=root/'dist/world-depth'
  with zipfile.ZipFile(folder/'re4bridge-0.4.57-windows.zip') as package,zipfile.ZipFile(folder/'re4bridge-0.4.57-source.zip') as source:
   names=package.namelist();self.assertEqual(len(names),len(set(names)))
   self.assertFalse(any(Path(n).name.lower() in {'bio4.exe','re4bridge.json'} or n.endswith(('.sav','.log','.pyc')) for n in names))
   dll=package.read('native/dinput8.dll');pe=struct.unpack_from('<I',dll,0x3c)[0];self.assertEqual(struct.unpack_from('<H',dll,pe+4)[0],0x14c)
   self.assertEqual(hashlib.sha256(dll).hexdigest(),'be2d89e539bc9a4dc76421b93822ca7b8af73200cd6a033e6f795634117bd378')
   self.assertEqual(package.read('NATIVE-SHA256.txt').decode().split()[0],hashlib.sha256(dll).hexdigest())
   self.assertIn('tools/re4_world_mesh.py',names)
   with zipfile.ZipFile(io.BytesIO(package.read('mods/re4bridge-0.4.57.jar'))) as mod:
    metadata=json.loads(mod.read('fabric.mod.json'));self.assertEqual(metadata['version'],'0.4.57')
    self.assertFalse(any(n.startswith('net/minecraft/') for n in mod.namelist()))
    with zipfile.ZipFile(io.BytesIO(mod.read(metadata['jars'][0]['file']))) as core:self.assertIn('dev/dezva/re4bridge/WorldMesh.class',core.namelist())
   self.assertFalse(any(set(Path(n).parts)&{'.git','.gradle','build','__pycache__'} for n in source.namelist()))
   for n in names:
    if n.startswith('tools/') and n.endswith('.py'):self.assertEqual(package.read(n),source.read(n),n)
   for n in ('docs/RE4-BLOCK-0.4.57.md','README.md','MODLOG.md','docs/CONTRACT.md'):
    text=source.read(n).decode('utf-8');self.assertNotIn('Ã',text)
   with tempfile.TemporaryDirectory() as directory:
    stage=Path(directory);(stage/'tools').mkdir()
    with zipfile.ZipFile(root/'releases/0.4.56/re4bridge-0.4.56-windows.zip') as old:
     for n in old.namelist():
      if n.startswith('tools/') and n.endswith('.py'):
       p=stage/n;p.write_bytes(old.read(n));stamp=time.mktime((*old.getinfo(n).date_time,0,0,-1));os.utime(p,(stamp,stamp));py_compile.compile(str(p),doraise=True)
    for n in names:
     if n.startswith('tools/') and n.endswith('.py'):
      p=stage/n;p.write_bytes(package.read(n));stamp=time.mktime((*package.getinfo(n).date_time,0,0,-1));os.utime(p,(stamp,stamp))
    code='import sys,unittest;sys.path[:0]=sys.argv[1:];s=unittest.defaultTestLoader.loadTestsFromNames(["test_world_mesh","test_combat","test_runtime"]);sys.exit(not unittest.TextTestRunner().run(s).wasSuccessful())'
    result=subprocess.run([sys.executable,'-c',code,str(stage/'tools'),str(root/'tools')],capture_output=True,text=True,timeout=30)
    self.assertEqual(result.returncode,0,result.stdout+result.stderr)
