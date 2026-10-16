param(
    [string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32',
    [string]$SavesDir = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\Profile\Player\Saves'
)
$ErrorActionPreference = 'Stop'
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'Cierra RE4 antes de instalar.' }
$game = (Resolve-Path -LiteralPath $GameBin).Path
$saves = (Resolve-Path -LiteralPath $SavesDir).Path
if (-not (Test-Path -LiteralPath (Join-Path $game 'bio4.exe') -PathType Leaf)) { throw 'No se encontro bio4.exe.' }
if (-not (Test-Path -LiteralPath $saves -PathType Container)) { throw 'No se encontro la carpeta de partidas.' }
$target = Join-Path $game 'dinput8.dll'
if (Test-Path -LiteralPath $target) { throw 'Ya existe dinput8.dll. No se reemplaza: puede ser re4_tweaks u otro mod. Envia este mensaje.' }
$source = Join-Path $PSScriptRoot 'native\dinput8.dll'
$expected = ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'NATIVE-SHA256.txt') -Raw).Trim() -split '\s+')[0]
$hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hash -ne $expected) { throw 'El checksum de la DLL no coincide con el paquete.' }
if (-not (Get-ChildItem -LiteralPath $saves -File -Recurse | Select-Object -First 1)) { throw 'La carpeta de partidas esta vacia: comprueba la ruta.' }
$backup = Join-Path $env:LOCALAPPDATA ('RE4Bridge\Backups\' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $backup | Out-Null
# Copy files directly: preserves hidden files and permits simple restoration by folder.
$saveCopy = Join-Path $backup 'Saves'
Copy-Item -LiteralPath $saves -Destination $saveCopy -Recurse -Force
$originals = @(Get-ChildItem -LiteralPath $saves -File -Recurse -Force)
$copied = @(Get-ChildItem -LiteralPath $saveCopy -File -Recurse -Force)
if ($originals.Count -ne $copied.Count) { throw 'La copia de partidas no esta completa. No se instala la DLL.' }
foreach ($file in $originals) {
    $relative = $file.FullName.Substring($saves.TrimEnd('\').Length + 1)
    $copy = Join-Path $saveCopy $relative
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $copy).Hash) { throw 'Fallo verificando la copia de partidas.' }
}
$configCopy = Join-Path $backup 'Config'
New-Item -ItemType Directory -Path $configCopy | Out-Null
Get-ChildItem -LiteralPath $game -Filter '*.ini' -File -Force | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $configCopy }
@{ game_bin=$game; saves_dir=$saves; dll_sha256=$hash; version='0.4.57-depthfix1' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding UTF8
if (Get-Process bio4 -ErrorAction SilentlyContinue) { throw 'RE4 se abrio durante la copia. Cierralo y repite.' }
# CreateNew also refuses a DLL created after the initial existence check.
$input = [IO.File]::OpenRead($source)
$output = $null
$copyError = $null
$created = $false
try {
    $output = [IO.File]::Open($target,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    $created = $true
    $input.CopyTo($output)
} catch {
    $copyError = $_
} finally {
    if ($output) { $output.Dispose() }
    $input.Dispose()
}
if ($copyError) {
    if ($created) { Remove-Item -LiteralPath $target }
    throw $copyError
}
if ((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $expected) {
    Remove-Item -LiteralPath $target
    throw 'Copia de DLL incompleta. Se retiro el archivo propio; comunica el error.'
}
Write-Host "Instalado: $target"
Write-Host "Copia de seguridad verificada: $backup"
Write-Host 'Para retirar: cierra RE4 y ejecuta UNINSTALL-RE4-CONTROLS.ps1.'
