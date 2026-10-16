import hashlib,io,json,os,subprocess,sys,tempfile,unittest,zipfile
from pathlib import Path
from package_impactfix import VERSION,NATIVE_SHA256

class ImpactfixPackageTests(unittest.TestCase):
 def test_delivered_host_mod_and_native_artifacts_are_consistent_and_private_files_absent(self):
  root=Path(__file__).resolve().parents[1];folder=root/'releases'/VERSION
  if not folder.exists():folder=root/'dist/impactfix2'
  with zipfile.ZipFile(folder/f're4bridge-{VERSION}-windows.zip') as new,zipfile.ZipFile(folder/f're4bridge-{VERSION}-source.zip') as source,zipfile.ZipFile(root/'releases/0.4.57-depthfix1/re4bridge-0.4.57-depthfix1-windows.zip') as old:
   self.assertEqual(len(new.namelist()),len(set(new.namelist())))
   self.assertEqual(hashlib.sha256(new.read('native/dinput8.dll')).hexdigest(),NATIVE_SHA256)
   self.assertEqual(new.read('NATIVE-SHA256.txt').decode().split()[0],NATIVE_SHA256)
   self.assertEqual([n for n in new.namelist() if n.startswith('mods/re4bridge-')],[f'mods/re4bridge-{VERSION}.jar'])
   self.assertEqual(new.read('README.md'),new.read('docs/RE4-IMPACTFIX2.md'))
   self.assertFalse(any(Path(n).name.lower() in {'bio4.exe','re4bridge.json','steam_api.dll'} or n.endswith(('.sav','.log','.pyc','.env')) for n in new.namelist()))
   self.assertFalse(any(set(Path(n).parts)&{'build','.gradle','.git','__pycache__'} for n in source.namelist()))
   for name in new.namelist():
    if name.startswith('mods/') and not name.startswith('mods/re4bridge-'):self.assertEqual(new.read(name),old.read(name))
    if name.startswith('tools/') and name.endswith('.py'):self.assertEqual(new.read(name),source.read(name))
   with zipfile.ZipFile(io.BytesIO(new.read(f'mods/re4bridge-{VERSION}.jar'))) as jar:
    info=json.loads(jar.read('fabric.mod.json'));self.assertEqual(info['version'],VERSION)
    with zipfile.ZipFile(io.BytesIO(jar.read(info['jars'][0]['file']))) as core:self.assertIn('dev/dezva/re4bridge/TerrainSnapshot.class',core.namelist())
   with tempfile.TemporaryDirectory() as d:
    stage=Path(d)
    for name in new.namelist():
     if name.startswith('tools/') and name.endswith('.py'):(stage/Path(name).name).write_bytes(new.read(name))
    code='import sys,unittest;sys.path[:0]=sys.argv[1:];s=unittest.defaultTestLoader.loadTestsFromNames(["test_depth","test_world_mesh","test_re4_control","test_runtime","test_combat","test_terrain_feed"]);sys.exit(not unittest.TextTestRunner().run(s).wasSuccessful())'
    env=dict(os.environ,PYTHONUTF8='1')
    result=subprocess.run([sys.executable,'-X','utf8','-c',code,str(stage),str(root/'tools')],env=env,capture_output=True,text=True,timeout=60)
    self.assertEqual(result.returncode,0,result.stdout+result.stderr)
