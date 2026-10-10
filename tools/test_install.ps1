param([Parameter(Mandatory=$true)][string]$PackageDir)
$ErrorActionPreference = 'Stop'
$sandbox = Join-Path ([IO.Path]::GetTempPath()) ('re4-installer-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $sandbox | Out-Null
$previousLocalAppData = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = Join-Path $sandbox 'LocalAppData'
    $game = Join-Path $sandbox 'Bin32'
    $saves = Join-Path $sandbox 'Profile/Player/Saves'
    New-Item -ItemType Directory -Path $game,$saves -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $game 'bio4.exe') -Value 'SYNTHETIC FIXTURE NOT A GAME'
    Set-Content -LiteralPath (Join-Path $game 'config.ini') -Value 'synthetic-config'
    Set-Content -LiteralPath (Join-Path $saves 'savegame00.sav') -Value 'synthetic-save'
    $saveHash = (Get-FileHash -LiteralPath (Join-Path $saves 'savegame00.sav')).Hash
    & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $saves
    $backups = @(Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'RE4Bridge/Backups') -Directory)
    if ($backups.Count -ne 1) { throw 'Expected one backup' }
    $copy = Join-Path $backups[0].FullName 'Saves/savegame00.sav'
    if ((Get-FileHash -LiteralPath $copy).Hash -ne $saveHash) { throw 'Backup differs' }
    if ((Get-FileHash -LiteralPath (Join-Path $saves 'savegame00.sav')).Hash -ne $saveHash) { throw 'Original save changed' }
    $rejected = $false
    try { & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $saves } catch { $rejected = $true }
    if (-not $rejected) { throw 'Existing DLL was not rejected' }
    & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
    if (Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')) { throw 'Own DLL remains' }
    if (-not (Test-Path -LiteralPath $copy)) { throw 'Uninstall removed backup' }
    Set-Content -LiteralPath (Join-Path $game 'dinput8.dll') -Value 'FOREIGN DLL FIXTURE'
    $foreignHash = (Get-FileHash -LiteralPath (Join-Path $game 'dinput8.dll')).Hash
    $rejected = $false
    try { & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game } catch { $rejected = $true }
    if (-not $rejected -or (Get-FileHash -LiteralPath (Join-Path $game 'dinput8.dll')).Hash -ne $foreignHash) { throw 'Foreign DLL not preserved' }
    # Remove only our synthetic foreign fixture to exercise the empty-save guard.
    Remove-Item -LiteralPath (Join-Path $game 'dinput8.dll')
    $empty = Join-Path $sandbox 'EmptySaves'
    New-Item -ItemType Directory -Path $empty | Out-Null
    $rejected = $false
    try { & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $empty } catch { $rejected = $true }
    if (-not $rejected -or (Test-Path -LiteralPath (Join-Path $game 'dinput8.dll'))) { throw 'Empty saves guard failed' }
    Write-Host 'Installer fixtures passed: verified backup/install, no overwrite, own-only removal, empty saves rejection.'
} finally {
    $env:LOCALAPPDATA = $previousLocalAppData
    Remove-Item -LiteralPath $sandbox -Recurse -Force
}
