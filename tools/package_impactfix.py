"""Package verified project outputs; never read or modify a game installation."""
import hashlib,io,json
from pathlib import Path
import zipfile
VERSION='0.4.57-impactfix2'
NATIVE_SHA256='24427265a508527318669a7b0cbd9a82a77f66132d930936dc0b09b2cf37c041'
BASE_SHA256='0f6ad20cadd18a2db1fe259d20bf65265bc3fca3bd48265f4a9bc5935bda939d'

def package(root,output):
 base=root/'releases/0.4.57-depthfix1/re4bridge-0.4.57-depthfix1-windows.zip'
 if hashlib.sha256(base.read_bytes()).hexdigest()!=BASE_SHA256:raise ValueError('Verified base changed')
 dll=(root/'native/build/dinput8.dll').read_bytes()
 if hashlib.sha256(dll).hexdigest()!=NATIVE_SHA256:raise ValueError('Unverified native output')
 jar=root/f'minecraft/build/libs/re4bridge-{VERSION}.jar';data=jar.read_bytes()
 with zipfile.ZipFile(io.BytesIO(data)) as mod:
  info=json.loads(mod.read('fabric.mod.json'))
  if info['version']!=VERSION:raise ValueError('Wrong mod version')
  with zipfile.ZipFile(io.BytesIO(mod.read(info['jars'][0]['file']))) as core:
   if 'dev/dezva/re4bridge/TerrainSnapshot.class' not in core.namelist():raise ValueError('Missing core')
 output.mkdir(parents=True,exist_ok=True);full=output/f're4bridge-{VERSION}-windows.zip';source=output/f're4bridge-{VERSION}-source.zip'
 with zipfile.ZipFile(base) as old,zipfile.ZipFile(full,'w',zipfile.ZIP_DEFLATED) as out:
  for name in old.namelist():
   if name.startswith('mods/re4bridge-'):continue
   if name.startswith('mods/'):content=old.read(name)
   elif name=='native/dinput8.dll':content=dll
   elif name=='NATIVE-SHA256.txt':content=(NATIVE_SHA256+'  native/dinput8.dll\n').encode()
   elif name=='README.md':content=(root/'docs/RE4-IMPACTFIX2.md').read_bytes()
   elif name=='README-TRACE.md':content=(root/'docs/RE4-CAMERA-TRACE.md').read_bytes()
   else:content=(root/name).read_bytes()
   out.writestr(name,content)
  out.writestr('mods/'+jar.name,data)
  out.writestr('docs/RE4-IMPACTFIX2.md',(root/'docs/RE4-IMPACTFIX2.md').read_bytes())
 with zipfile.ZipFile(root/'releases/0.4.57-depthfix1/re4bridge-0.4.57-depthfix1-source.zip') as old:names=set(old.namelist())
 for folder in ('bridge-core/src','minecraft/src','native','tools','docs'):
  names.update(p.relative_to(root).as_posix() for p in (root/folder).rglob('*') if p.is_file() and not set(p.relative_to(root).parts)&{'build','__pycache__'})
 with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as out:
  for name in sorted(names):
   if set(Path(name).parts)&{'build','__pycache__','.git','.gradle'} or name.endswith(('.pyc','.log','.env')):continue
   out.writestr(name,(root/name).read_bytes())
 lines=[]
 for p in (full,source):
  lines.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name);print(p.name,p.stat().st_size,'bytes')
 (output/'SHA256SUMS.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1];package(root,root/'dist/impactfix2')
