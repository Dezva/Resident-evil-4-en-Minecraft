"""Build the standalone Windows presentation helper; no mod or DLL changes."""
import hashlib
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
destination = root / 'dist' / 'view-0.1.0'
destination.mkdir(parents=True, exist_ok=True)
package = destination / 're4bridge-view-0.1.0.zip'
with zipfile.ZipFile(package, 'w', zipfile.ZIP_DEFLATED) as archive:
    for source, target in [('VIEW-RE4.bat','VIEW-RE4.bat'), ('VIEW-RE4.ps1','VIEW-RE4.ps1'),
                           ('tools/RE4View.cs','tools/RE4View.cs'), ('docs/RE4-VIEW.md','README.md')]:
        data = (root / source).read_bytes()
        if source.endswith('.bat'):
            data = data.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
        archive.writestr(target,data)
digest = hashlib.sha256(package.read_bytes()).hexdigest()
(destination / 'SHA256SUMS.txt').write_text(digest + '  ' + package.name + '\n')
print(package.name, package.stat().st_size, 'bytes; SHA256', digest)
