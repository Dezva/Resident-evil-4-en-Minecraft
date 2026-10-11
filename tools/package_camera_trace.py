"""Package the read-only diagnostic separately from mods and native DLLs."""
import hashlib
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
destination = root / 'dist' / 'camera-trace-0.1.0'
destination.mkdir(parents=True, exist_ok=True)
package = destination / 're4bridge-camera-trace-0.1.0.zip'
with zipfile.ZipFile(package, 'w', zipfile.ZIP_DEFLATED) as archive:
    for source, target in [
        ('TRACE-RE4-CAMERA.bat', 'TRACE-RE4-CAMERA.bat'),
        ('tools/re4_camera_trace.py', 'tools/re4_camera_trace.py'),
        ('tools/re4_reader.py', 'tools/re4_reader.py'),
        ('tools/re4_native_probe.py', 'tools/re4_native_probe.py'),
        ('docs/RE4-CAMERA-TRACE.md', 'README.md'),
    ]:
        data = (root / source).read_bytes()
        if source.endswith('.bat'):
            data = data.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        archive.writestr(target, data)
digest = hashlib.sha256(package.read_bytes()).hexdigest()
(destination / 'SHA256SUMS.txt').write_text(digest + '  ' + package.name + '\n')
print(package.name, package.stat().st_size, 'bytes; SHA256', digest)
