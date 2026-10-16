"""Build0.4.57 from verified project artifacts; never inspect the game installation."""
import hashlib,io,json
from pathlib import Path
import zipfile
VERSION='0.4.57'
NATIVE_SHA256='be2d89e539bc9a4dc76421b93822ca7b8af73200cd6a033e6f795634117bd378'
BASE_SHA256='50a060c2365e28b3314689857b26e8f0442f23d95a11693ce2f5642f41dba1bf'
def package(root,output):
 base=root/'releases/0.4.56/re4bridge-0.4.56-windows.zip'
 if hashlib.sha256(base.read_bytes()).hexdigest()!=BASE_SHA256:raise ValueError('Base package changed')
 dll=(root/'native/build/dinput8.dll').read_bytes()
 if hashlib.sha256(dll).hexdigest()!=NATIVE_SHA256:raise ValueError('Unverified native build')
 jar=root/f'minecraft/build/libs/re4bridge-{VERSION}.jar';data=jar.read_bytes()
 with zipfile.ZipFile(io.BytesIO(data)) as mod:
  if json.loads(mod.read('fabric.mod.json'))['version']!=VERSION:raise ValueError('Wrong mod version')
  core=json.loads(mod.read('fabric.mod.json'))['jars'][0]['file']
  with zipfile.ZipFile(io.BytesIO(mod.read(core))) as nested:
   if 'dev/dezva/re4bridge/WorldMesh.class' not in nested.namelist():raise ValueError('Missing mesh core')
 output.mkdir(parents=True,exist_ok=True)
 full=output/f're4bridge-{VERSION}-windows.zip';source=output/f're4bridge-{VERSION}-source.zip'
 with zipfile.ZipFile(base) as old,zipfile.ZipFile(full,'w',zipfile.ZIP_DEFLATED) as out:
  for name in old.namelist():
   if name.startswith('mods/re4bridge-'):continue
   if name.startswith('mods/'):content=old.read(name) # pinned original Fabric API
   elif name=='native/dinput8.dll':content=dll
   elif name=='NATIVE-SHA256.txt':content=(NATIVE_SHA256+'  native/dinput8.dll\n').encode()
   elif name=='README-TRACE.md':content=(root/'docs/RE4-CAMERA-TRACE.md').read_bytes()
   elif name=='README.md':content=(root/'docs/RE4-BLOCK-0.4.57.md').read_bytes()
   else:content=(root/name).read_bytes()
   out.writestr(name,content)
  out.writestr('mods/'+jar.name,data)
  out.writestr('tools/re4_world_mesh.py',(root/'tools/re4_world_mesh.py').read_bytes())
 with zipfile.ZipFile(root/'releases/0.4.56/re4bridge-0.4.56-source.zip') as old:
  names=set(old.namelist())
 for folder in ('bridge-core/src','minecraft/src','native','tools','docs'):
  names.update(p.relative_to(root).as_posix() for p in (root/folder).rglob('*') if p.is_file() and not set(p.relative_to(root).parts)&{'build','__pycache__'})
 with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as out:
  for name in sorted(names):
   if set(Path(name).parts)&{'build','__pycache__','.git','.gradle'} or name.endswith(('.pyc','.log','.env')):continue
   out.writestr(name,(root/name).read_bytes())
 sums=[]
 for p in (full,source):
  sums.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name);print(p.name,p.stat().st_size,'bytes')
 (output/'SHA256SUMS.txt').write_text('\n'.join(sums)+'\n',encoding='utf-8')
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1];package(root,root/'dist/world-depth')
