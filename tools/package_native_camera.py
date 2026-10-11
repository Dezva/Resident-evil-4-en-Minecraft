"""Build native-only camera update, compatible with the existing 0.4.3 mod."""
import hashlib
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
destination = root / 'dist' / 'native-camera-0.4.4'
destination.mkdir(parents=True, exist_ok=True)
package = destination / 're4bridge-camera-0.4.4-windows.zip'
sources = [(name, name) for name in (
    'INSTALL-RE4-CONTROLS.bat', 'INSTALL-RE4-CONTROLS.ps1',
    'UNINSTALL-RE4-CONTROLS.bat', 'UNINSTALL-RE4-CONTROLS.ps1',
    'DIAGNOSE-RE4-CONTROLS.bat', 'DIAGNOSE-RE4-CONTROLS.ps1',
    'TRACE-RE4-CAMERA.bat', 'tools/re4_camera_trace.py',
    'tools/re4_reader.py', 'tools/re4_native_probe.py')]
sources += [('native/build/dinput8.dll', 'native/dinput8.dll'),
            ('docs/RE4-CAMERA-0.4.4.md', 'README.md'),
            ('docs/RE4-CAMERA-TRACE.md', 'README-TRACE.md')]
with zipfile.ZipFile(package, 'w', zipfile.ZIP_DEFLATED) as archive:
    for source, target in sources:
        data = (root / source).read_bytes()
        if source.endswith('.bat'):
            data = data.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        archive.writestr(target, data)
    native_hash = hashlib.sha256((root/'native/build/dinput8.dll').read_bytes()).hexdigest()
    archive.writestr('NATIVE-SHA256.txt', native_hash+'  native/dinput8.dll\n')
digest = hashlib.sha256(package.read_bytes()).hexdigest()
(destination / 'SHA256SUMS.txt').write_text(digest+'  '+package.name+'\n')
print(package.name, package.stat().st_size, 'bytes; SHA256', digest)
