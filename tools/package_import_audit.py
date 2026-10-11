"""Package the read-only import audit without any new DLL or installer."""
import hashlib
from pathlib import Path
import zipfile


def main():
    root=Path(__file__).resolve().parents[1]
    destination=root/'dist';destination.mkdir(exist_ok=True)
    package=destination/'re4bridge-0.4.15-import-diagnostic.zip'
    files={'AUDIT-RE4-IMPORTS.bat':'AUDIT-RE4-IMPORTS.bat',
           'tools/re4_import_audit.py':'tools/re4_import_audit.py',
           'tools/re4_reader.py':'tools/re4_reader.py',
           'tools/re4_graphics_probe.py':'tools/re4_graphics_probe.py',
           'tools/re4_graphics_capture.py':'tools/re4_graphics_capture.py',
           'README.md':'docs/RE4-IMPORT-AUDIT.md'}
    with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as archive:
        for name,source in files.items():
            data=(root/source).read_bytes()
            if name.endswith('.bat'):data=data.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
            archive.writestr(name,data)
    digest=hashlib.sha256(package.read_bytes()).hexdigest()
    (destination/'IMPORT-SHA256SUMS.txt').write_text(digest+'  '+package.name+'\n')
    print(package.name,package.stat().st_size,'bytes, SHA256',digest)


if __name__=='__main__':main()
