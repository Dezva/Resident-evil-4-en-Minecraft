"""Deliver a mod-only fix; preserve the published DLL456 and every host file."""
import hashlib
import io
import json
from pathlib import Path
import zipfile

VERSION='0.4.56-arrowfix1'
NATIVE_SHA256='5b25e60dd954c1b5e7480418f406cce8ecd43ee8775ada587da4ecae3ee88e6d'


def package(root,output):
    jar=root/f'minecraft/build/libs/re4bridge-{VERSION}.jar'
    data=jar.read_bytes()
    with zipfile.ZipFile(io.BytesIO(data)) as mod:
        if json.loads(mod.read('fabric.mod.json'))['version']!=VERSION:raise ValueError('Wrong mod version')
        if 'dev/dezva/re4bridge/minecraft/BridgeArrow.class' not in mod.namelist():raise ValueError('Missing arrow admission')
    guide=(root/'docs/RE4-ARROWFIX1.md').read_bytes()
    output.mkdir(parents=True,exist_ok=True)
    patch=output/f're4bridge-{VERSION}-mod.zip'
    full=output/f're4bridge-{VERSION}-windows.zip'
    name='mods/'+jar.name
    with zipfile.ZipFile(patch,'w',zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(name,data);archive.writestr('README-ARROWFIX1.md',guide)
    with zipfile.ZipFile(root/'releases/0.4.56/re4bridge-0.4.56-windows.zip') as base:
        if hashlib.sha256(base.read('native/dinput8.dll')).hexdigest()!=NATIVE_SHA256:raise ValueError('Wrong base DLL')
        with zipfile.ZipFile(full,'w',zipfile.ZIP_DEFLATED) as archive:
            for entry in base.infolist():
                if entry.filename=='mods/re4bridge-0.4.56.jar':continue
                archive.writestr(entry,guide if entry.filename=='README.md' else base.read(entry.filename))
            archive.writestr(name,data)
    sums=[]
    for path in (patch,full):
        sums.append(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name)
        print(path.name,path.stat().st_size,'bytes')
    (output/'SHA256SUMS.txt').write_text('\n'.join(sums)+'\n',encoding='utf-8')


if __name__=='__main__':
    root=Path(__file__).resolve().parents[1]
    package(root,root/'dist/arrowfix1')
