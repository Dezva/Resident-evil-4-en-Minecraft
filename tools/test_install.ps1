param([Parameter(Mandatory=$true)][string]$PackageDir,[string]$LegacyDll,[string]$HealthDll,[string]$RuntimeDll,[string]$SceneDll,[string]$VerticalDll)
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
    if($LegacyDll){
        if((Get-FileHash -LiteralPath $LegacyDll).Hash.ToLowerInvariant() -ne '500f81beea5f537df72b2f979d241d3084bc82379614331508a47ae6cee11147'){throw 'Wrong published legacy fixture'}
        Copy-Item -LiteralPath $LegacyDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build439 upgrade removal failed'}
    }
    if($HealthDll){
        if((Get-FileHash -LiteralPath $HealthDll).Hash.ToLowerInvariant() -ne 'b83d90809e34634ed34d7b77bee7a61600ecca19fd55e8c2a48ceddec480010e'){throw 'Wrong published build441 fixture'}
        Copy-Item -LiteralPath $HealthDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build441 upgrade removal failed'}
    }
    if($RuntimeDll){
        if((Get-FileHash -LiteralPath $RuntimeDll).Hash.ToLowerInvariant() -ne '205bc8cd630616ec0d8f71f1468bd3f6615ef388aed280eb1901289c6ef68715'){throw 'Wrong published build442 fixture'}
        Copy-Item -LiteralPath $RuntimeDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build442 upgrade removal failed'}
    }
    if($SceneDll){
        if((Get-FileHash -LiteralPath $SceneDll).Hash.ToLowerInvariant() -ne '935ffd79897f5ec8e93754db21b63d77725b888efeda32dd94a9b435661a7ae8'){throw 'Wrong published build443 fixture'}
        Copy-Item -LiteralPath $SceneDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build443 upgrade removal failed'}
    }
    if($VerticalDll){
        if((Get-FileHash -LiteralPath $VerticalDll).Hash.ToLowerInvariant() -ne '71a40ff4d5a1299ae9df470dcd9ba0e03646d532bc13fed63a0c1959fbddc289'){throw 'Wrong published build444 fixture'}
        Copy-Item -LiteralPath $VerticalDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build444 upgrade removal failed'}
    }
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
