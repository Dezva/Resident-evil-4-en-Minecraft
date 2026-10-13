param([string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32')
$ErrorActionPreference = 'Stop'
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'Cierra RE4 antes de desinstalar.' }
$target = Join-Path (Resolve-Path -LiteralPath $GameBin).Path 'dinput8.dll'
if (-not (Test-Path -LiteralPath $target)) { Write-Host 'La DLL ya esta retirada.'; exit 0 }
$expected = ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'NATIVE-SHA256.txt') -Raw).Trim() -split '\s+')[0]
# 0.4.39 and 0.4.40 shared this exact published DLL. Permit upgrading it
# with the new package without accepting filenames, unknown hashes or other mods.
$known = @($expected,'500f81beea5f537df72b2f979d241d3084bc82379614331508a47ae6cee11147')
if ((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -notin $known) { throw 'Esta DLL no coincide con el paquete ni la build439 conocida; no se elimina. Usa el desinstalador de la version instalada.' }
Remove-Item -LiteralPath $target
Write-Host 'Retirada solamente la DLL de RE4 Bridge. Se conservan las partidas y sus copias de seguridad.'
