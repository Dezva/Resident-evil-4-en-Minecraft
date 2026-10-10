#!/usr/bin/env bash
set -euo pipefail
if [ ! -x /workspace/tools/powershell-7.4.7/pwsh ]; then
python3 - <<'PY'
import hashlib, pathlib, tarfile, urllib.request
base='https://github.com/PowerShell/PowerShell/releases/download/v7.4.7/'
name='powershell-7.4.7-linux-x64.tar.gz'
sums=urllib.request.urlopen(base+'hashes.sha256',timeout=30).read().decode('utf-8-sig').splitlines()
expected=next(s for s in sums if name in s).split()[0].lower()
assert len(expected)==64
archive=pathlib.Path('/workspace/tools')/name
with urllib.request.urlopen(base+name,timeout=60) as response, archive.open('wb') as output:
    while data:=response.read(1024*1024): output.write(data)
assert hashlib.sha256(archive.read_bytes()).hexdigest()==expected
root=pathlib.Path('/workspace/tools/powershell-7.4.7')
root.mkdir(exist_ok=True)
with tarfile.open(archive) as package: package.extractall(root,filter='data')
(root/'pwsh').chmod(0o755)
PY
fi
XDG_CACHE_HOME=/workspace/.cache XDG_CONFIG_HOME=/workspace/.config XDG_DATA_HOME=/workspace/.local/share \
  /workspace/tools/powershell-7.4.7/pwsh -NoLogo -NoProfile -Command '$PSVersionTable.PSVersion.ToString()'
