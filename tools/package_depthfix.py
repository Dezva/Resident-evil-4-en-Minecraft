"""Native0.4.57 depthfix1; preserve the installed mod and native IPC contract."""
import hashlib
from pathlib import Path
import zipfile
VERSION='0.4.57-depthfix1'
NATIVE_SHA256='5250b4ae97f6c1a9f0c15c8f65e7e4be9e5d7b4649f861d118a9e83f2b35b07b'
BASE_SHA256='d9310cf0405eed9ed64b65272ca082043c5f6d482b924dac966b2f96a84e5e7c'
def package(root,output):
 base=root/'releases/0.4.57/re4bridge-0.4.57-windows.zip'
 if hashlib.sha256(base.read_bytes()).hexdigest()!=BASE_SHA256:raise ValueError('Wrong verified base package')
 dll=(root/'native/build/dinput8.dll').read_bytes()
 if hashlib.sha256(dll).hexdigest()!=NATIVE_SHA256:raise ValueError('Unverified native patch')
 overrides={'native/dinput8.dll':dll,'NATIVE-SHA256.txt':(NATIVE_SHA256+'  native/dinput8.dll\n').encode(),'README.md':(root/'docs/RE4-DEPTHFIX1.md').read_bytes()}
 for name in ('INSTALL-RE4-CONTROLS.ps1','UNINSTALL-RE4-CONTROLS.ps1','tools/re4_depth.py','tools/re4_camera_trace.py','docs/CONTRACT.md','docs/VALIDATION.md'):overrides[name]=(root/name).read_bytes()
 output.mkdir(parents=True,exist_ok=True);full=output/f're4bridge-{VERSION}-windows.zip';source=output/f're4bridge-{VERSION}-source.zip'
 with zipfile.ZipFile(base) as old,zipfile.ZipFile(full,'w',zipfile.ZIP_DEFLATED) as out:
  for entry in old.infolist():out.writestr(entry.filename,overrides.get(entry.filename,old.read(entry.filename)))
 with zipfile.ZipFile(root/'releases/0.4.57/re4bridge-0.4.57-source.zip') as old:names=set(old.namelist())
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
 root=Path(__file__).resolve().parents[1];package(root,root/'dist/depthfix1')
