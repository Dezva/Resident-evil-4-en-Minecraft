"""Build a standalone read-only diagnostic; no DLL, installer or Minecraft mod."""
import hashlib
from pathlib import Path
import zipfile


def main():
    root=Path(__file__).resolve().parents[1];destination=root/'dist';destination.mkdir(exist_ok=True)
    package=destination/'re4bridge-0.4.12-graphics-diagnostic.zip'
    files={'PROBE-RE4-GRAPHICS.bat':'PROBE-RE4-GRAPHICS.bat',
           'tools/re4_graphics_probe.py':'tools/re4_graphics_probe.py',
           'tools/re4_reader.py':'tools/re4_reader.py','README.md':'docs/RE4-GRAPHICS-PROBE.md'}
    with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as archive:
        for name,source in files.items():
            data=(root/source).read_bytes()
            if name.endswith('.bat'):data=data.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
            archive.writestr(name,data)
    sha=hashlib.sha256(package.read_bytes()).hexdigest()
    (destination/'GRAPHICS-SHA256SUMS.txt').write_text(sha+'  '+package.name+'\n')
    print(package.name,package.stat().st_size,'bytes, SHA256',sha)


if __name__=='__main__':main()
