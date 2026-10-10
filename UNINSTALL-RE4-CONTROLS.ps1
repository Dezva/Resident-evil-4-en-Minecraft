param([string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32')
$ErrorActionPreference = 'Stop'
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'Cierra RE4 antes de desinstalar.' }
$target = Join-Path (Resolve-Path -LiteralPath $GameBin).Path 'dinput8.dll'
if (-not (Test-Path -LiteralPath $target)) { Write-Host 'La DLL ya esta retirada.'; exit 0 }
$expected = ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'NATIVE-SHA256.txt') -Raw).Trim() -split '\s+')[0]
if ((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $expected) { throw 'Esta DLL no coincide con la nuestra; no se elimina.' }
Remove-Item -LiteralPath $target
Write-Host 'Retirada solamente la DLL de RE4 Bridge. Se conservan las partidas y sus copias de seguridad.'
