param([string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32')
$ErrorActionPreference = 'Stop'
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'Cierra RE4 antes de desinstalar.' }
$target = Join-Path (Resolve-Path -LiteralPath $GameBin).Path 'dinput8.dll'
if (-not (Test-Path -LiteralPath $target)) { Write-Host 'La DLL ya esta retirada.'; exit 0 }
$expected = ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'NATIVE-SHA256.txt') -Raw).Trim() -split '\s+')[0]
# Recognize the exact published 0.4.55/54/53/52/51/50/49/48/47/46/45/44/43/42/41 and shared 0.4.39/40 DLLs. Permit upgrading
# with the new package without accepting filenames, unknown hashes or other mods.
$known = @($expected,'e034d92c2e4e79732da6f66efce630dced46003e80edec2e08f0653c0e3ad362','46836d4f3e609cc9b127d99f6801b868c631cf4c604d2f040179483989a9d546','665d23b2342632d2579fed94cf0843cbf302968dbbeaa00b67b965f11e84f338','8ddee51be0483cb2bad06b7f966252b3abb669a78d98664ac515c349780f4e10','0c7e316b6e64cc57c2413abe8dce4e377ff62d3b418fba7b0eae87664dbc96ab','db52e9a31cedf59cc8a78831b216a07cccc0fd4cc22f8c8ea8e96e4043e330b4','180a36ee6c696328bdfbf64ebadb5c3b9f85c1a22952b74df3cad9056b6a8c80','bc56062587aed707f7e1be1a6f3d675fc9fd83eb6e36de81628ca51b175e7be9','fed0452d496f5078c16b602c2225293ff21afe236cbe618947837f633b2910bb','268138899971f1d3d5879ca6aec12334f846c8f2f5d1fbf0db41a6e9c527e476','71a40ff4d5a1299ae9df470dcd9ba0e03646d532bc13fed63a0c1959fbddc289','935ffd79897f5ec8e93754db21b63d77725b888efeda32dd94a9b435661a7ae8','205bc8cd630616ec0d8f71f1468bd3f6615ef388aed280eb1901289c6ef68715','b83d90809e34634ed34d7b77bee7a61600ecca19fd55e8c2a48ceddec480010e','500f81beea5f537df72b2f979d241d3084bc82379614331508a47ae6cee11147')
if ((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -notin $known) { throw 'Esta DLL no coincide con el paquete ni las builds439 y441..455 conocidas; no se elimina. Usa el desinstalador de la version instalada.' }
Remove-Item -LiteralPath $target
Write-Host 'Retirada solamente la DLL de RE4 Bridge. Se conservan las partidas y sus copias de seguridad.'
