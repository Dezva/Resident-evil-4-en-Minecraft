#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
installer_stage=$(mktemp -d /tmp/re4-install-fixture.XXXXXX)
trap 'rm -rf "$installer_stage"' EXIT
mkdir "$installer_stage/native"
cp INSTALL-RE4-CONTROLS.ps1 UNINSTALL-RE4-CONTROLS.ps1 "$installer_stage/"
cp native/build/dinput8.dll "$installer_stage/native/"
python3 - "$installer_stage" <<'PY'
import hashlib,pathlib,sys
import zipfile
root=pathlib.Path(sys.argv[1])
(root/'NATIVE-SHA256.txt').write_text(hashlib.sha256((root/'native/dinput8.dll').read_bytes()).hexdigest()+'  native/dinput8.dll\n')
with zipfile.ZipFile('releases/0.4.39/re4bridge-0.4.39-windows.zip') as archive:
    (root/'legacy.dll').write_bytes(archive.read('native/dinput8.dll'))
PY
XDG_CACHE_HOME=/workspace/.cache XDG_CONFIG_HOME=/workspace/.config XDG_DATA_HOME=/workspace/.local/share \
  /workspace/tools/powershell-7.4.7/pwsh -NoLogo -NoProfile -File tools/test_install.ps1 -PackageDir "$installer_stage" -LegacyDll "$installer_stage/legacy.dll"
XDG_CACHE_HOME=/workspace/.cache XDG_CONFIG_HOME=/workspace/.config XDG_DATA_HOME=/workspace/.local/share \
  /workspace/tools/powershell-7.4.7/pwsh -NoLogo -NoProfile -File tools/test_diagnostic.ps1
