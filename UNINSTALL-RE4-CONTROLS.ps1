param([string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32')
$ErrorActionPreference = 'Stop'
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'Cierra RE4 antes de desinstalar.' }
$target = Join-Path (Resolve-Path -LiteralPath $GameBin).Path 'dinput8.dll'
if (-not (Test-Path -LiteralPath $target)) { Write-Host 'La DLL ya esta retirada.'; exit 0 }
$expected = ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'NATIVE-SHA256.txt') -Raw).Trim() -split '\s+')[0]
# Recognize the exact published 0.4.45/44/43/42/41 and shared 0.4.39/40 DLLs. Permit upgrading
# with the new package without accepting filenames, unknown hashes or other mods.
$known = @($expected,'268138899971f1d3d5879ca6aec12334f846c8f2f5d1fbf0db41a6e9c527e476','71a40ff4d5a1299ae9df470dcd9ba0e03646d532bc13fed63a0c1959fbddc289','935ffd79897f5ec8e93754db21b63d77725b888efeda32dd94a9b435661a7ae8','205bc8cd630616ec0d8f71f1468bd3f6615ef388aed280eb1901289c6ef68715','b83d90809e34634ed34d7b77bee7a61600ecca19fd55e8c2a48ceddec480010e','500f81beea5f537df72b2f979d241d3084bc82379614331508a47ae6cee11147')
if ((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -notin $known) { throw 'Esta DLL no coincide con el paquete ni las builds439/441/442/443/444/445 conocidas; no se elimina. Usa el desinstalador de la version instalada.' }
Remove-Item -LiteralPath $target
Write-Host 'Retirada solamente la DLL de RE4 Bridge. Se conservan las partidas y sus copias de seguridad.'
