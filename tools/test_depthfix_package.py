import hashlib,io,json,os,subprocess,sys,tempfile,unittest,zipfile
from pathlib import Path
class DepthfixPackageTests(unittest.TestCase):
 def test_delivered_patch_preserves_mod_host_control_and_every_unchanged_file(self):
  root=Path(__file__).resolve().parents[1];folder=root/'releases/0.4.57-depthfix1'
  if not folder.exists():folder=root/'dist/depthfix1'
  changed={'native/dinput8.dll','NATIVE-SHA256.txt','README.md','INSTALL-RE4-CONTROLS.ps1','UNINSTALL-RE4-CONTROLS.ps1','tools/re4_depth.py','tools/re4_camera_trace.py','docs/CONTRACT.md','docs/VALIDATION.md'}
  with zipfile.ZipFile(folder/'re4bridge-0.4.57-depthfix1-windows.zip') as new,zipfile.ZipFile(root/'releases/0.4.57/re4bridge-0.4.57-windows.zip') as base,zipfile.ZipFile(folder/'re4bridge-0.4.57-depthfix1-source.zip') as source:
   self.assertEqual(set(new.namelist()),set(base.namelist()))
   for name in base.namelist():
    if name not in changed:self.assertEqual(new.read(name),base.read(name),name)
   dll=new.read('native/dinput8.dll');self.assertEqual(hashlib.sha256(dll).hexdigest(),'5250b4ae97f6c1a9f0c15c8f65e7e4be9e5d7b4649f861d118a9e83f2b35b07b')
   self.assertEqual(new.read('NATIVE-SHA256.txt').decode().split()[0],hashlib.sha256(dll).hexdigest())
   self.assertFalse(any(Path(n).name.lower() in {'bio4.exe','re4bridge.json'} or n.endswith(('.sav','.log','.pyc')) for n in new.namelist()))
   self.assertFalse(any(set(Path(n).parts)&{'build','.gradle','.git','__pycache__'} for n in source.namelist()))
   for name in new.namelist():
    if name.startswith('tools/') and name.endswith('.py'):self.assertEqual(new.read(name),source.read(name),name)
   with tempfile.TemporaryDirectory() as d:
    stage=Path(d)
    tests=stage/'archive-tests';tests.mkdir()
    # This immutable historical package must run its own versioned tests.
    # Future optional GPU channels are not requirements of the old DLL/reader.
    for name in ('test_depth','test_world_mesh','test_re4_control','test_runtime','test_combat'):
     (tests/(name+'.py')).write_bytes(source.read('tools/'+name+'.py'))
    for name in new.namelist():
     if name.startswith('tools/') and name.endswith('.py'):(stage/Path(name).name).write_bytes(new.read(name))
    code='import sys,unittest;sys.path[:0]=sys.argv[1:];s=unittest.defaultTestLoader.loadTestsFromNames(["test_depth","test_world_mesh","test_re4_control","test_runtime","test_combat"]);sys.exit(not unittest.TextTestRunner().run(s).wasSuccessful())'
    result=subprocess.run([sys.executable,'-X','utf8','-c',code,str(stage),str(tests),str(root/'tools')],capture_output=True,text=True,timeout=30);self.assertEqual(result.returncode,0,result.stdout+result.stderr)
