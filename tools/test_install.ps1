param([Parameter(Mandatory=$true)][string]$PackageDir,[string]$LegacyDll,[string]$HealthDll,[string]$RuntimeDll,[string]$SceneDll,[string]$VerticalDll,[string]$ContextDll,[string]$PreviousDll,[string]$LastDll,[string]$LatestDll,[string]$RecentDll,[string]$GroundDll,[string]$MeleeDll,[string]$ImpactDll,[string]$StableDll,[string]$TrapDll,[string]$ArrowDll,[string]$DepthDll,[string]$DepthfixDll)
$ErrorActionPreference = 'Stop'
$sandbox = Join-Path ([IO.Path]::GetTempPath()) ('re4-installer-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $sandbox | Out-Null
$previousLocalAppData = $env:LOCALAPPDATA
# Synthetic process discovery keeps installer fixtures independent of real games.
# It is scoped to this test script; production installers retain their guard.
$script:fixtureGameRunning = $false
function Get-Process {
    [CmdletBinding()] param([Parameter(Position=0)][string]$Name)
    if ($Name -ne 'bio4') { throw 'Unexpected process query in installer fixture' }
    if ($fixtureGameRunning) { [pscustomobject]@{ ProcessName = 'bio4' } }
}
try {
    $env:LOCALAPPDATA = Join-Path $sandbox 'LocalAppData'
    $game = Join-Path $sandbox 'Bin32'
    $saves = Join-Path $sandbox 'Profile/Player/Saves'
    New-Item -ItemType Directory -Path $game,$saves -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $game 'bio4.exe') -Value 'SYNTHETIC FIXTURE NOT A GAME'
    Set-Content -LiteralPath (Join-Path $game 'config.ini') -Value 'synthetic-config'
    Set-Content -LiteralPath (Join-Path $saves 'savegame00.sav') -Value 'synthetic-save'
    $saveHash = (Get-FileHash -LiteralPath (Join-Path $saves 'savegame00.sav')).Hash
    $script:fixtureGameRunning = $true
    $rejected = $false
    try { & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $saves } catch { $rejected = $true }
    if (-not $rejected -or (Test-Path -LiteralPath (Join-Path $game 'dinput8.dll'))) { throw 'Running-game install guard failed' }
    $script:fixtureGameRunning = $false
    & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $saves
    $backups = @(Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'RE4Bridge/Backups') -Directory)
    if ($backups.Count -ne 1) { throw 'Expected one backup' }
    $copy = Join-Path $backups[0].FullName 'Saves/savegame00.sav'
    if ((Get-FileHash -LiteralPath $copy).Hash -ne $saveHash) { throw 'Backup differs' }
    if ((Get-FileHash -LiteralPath (Join-Path $saves 'savegame00.sav')).Hash -ne $saveHash) { throw 'Original save changed' }
    $rejected = $false
    try { & (Join-Path $PackageDir 'INSTALL-RE4-CONTROLS.ps1') -GameBin $game -SavesDir $saves } catch { $rejected = $true }
    if (-not $rejected) { throw 'Existing DLL was not rejected' }
    $script:fixtureGameRunning = $true
    $rejected = $false
    try { & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game } catch { $rejected = $true }
    if (-not $rejected -or -not (Test-Path -LiteralPath (Join-Path $game 'dinput8.dll'))) { throw 'Running-game uninstall guard failed' }
    $script:fixtureGameRunning = $false
    & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
    if (Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')) { throw 'Own DLL remains' }
    if (-not (Test-Path -LiteralPath $copy)) { throw 'Uninstall removed backup' }
    if($DepthfixDll){
        if((Get-FileHash -LiteralPath $DepthfixDll).Hash.ToLowerInvariant() -ne '5250b4ae97f6c1a9f0c15c8f65e7e4be9e5d7b4649f861d118a9e83f2b35b07b'){throw 'Wrong published depthfix1 fixture'}
        Copy-Item -LiteralPath $DepthfixDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Published depthfix1 remains'}
    }
    if($DepthDll){
        if((Get-FileHash -LiteralPath $DepthDll).Hash.ToLowerInvariant() -ne 'be2d89e539bc9a4dc76421b93822ca7b8af73200cd6a033e6f795634117bd378'){throw 'Wrong published build457 fixture'}
        Copy-Item -LiteralPath $DepthDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Published build457 remains'}
    }
    if($ArrowDll){
        if((Get-FileHash -LiteralPath $ArrowDll).Hash.ToLowerInvariant() -ne '5b25e60dd954c1b5e7480418f406cce8ecd43ee8775ada587da4ecae3ee88e6d'){throw 'Wrong published build456 fixture'}
        Copy-Item -LiteralPath $ArrowDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Published build456 remains'}
    }
    if($TrapDll){
        if((Get-FileHash -LiteralPath $TrapDll).Hash.ToLowerInvariant() -ne 'e034d92c2e4e79732da6f66efce630dced46003e80edec2e08f0653c0e3ad362'){throw 'Wrong published build455 fixture'}
        Copy-Item -LiteralPath $TrapDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build455 upgrade removal failed'}
    }
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
    if($StableDll){
        if((Get-FileHash -LiteralPath $StableDll).Hash.ToLowerInvariant() -ne '46836d4f3e609cc9b127d99f6801b868c631cf4c604d2f040179483989a9d546'){throw 'Wrong published build453 fixture'}
        Copy-Item -LiteralPath $StableDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build453 upgrade removal failed'}
    }
    if($ImpactDll){
        if((Get-FileHash -LiteralPath $ImpactDll).Hash.ToLowerInvariant() -ne '665d23b2342632d2579fed94cf0843cbf302968dbbeaa00b67b965f11e84f338'){throw 'Wrong published build452 fixture'}
        Copy-Item -LiteralPath $ImpactDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build452 upgrade removal failed'}
    }
    if($MeleeDll){
        if((Get-FileHash -LiteralPath $MeleeDll).Hash.ToLowerInvariant() -ne '8ddee51be0483cb2bad06b7f966252b3abb669a78d98664ac515c349780f4e10'){throw 'Wrong published build451 fixture'}
        Copy-Item -LiteralPath $MeleeDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build451 upgrade removal failed'}
    }
    if($GroundDll){
        if((Get-FileHash -LiteralPath $GroundDll).Hash.ToLowerInvariant() -ne '0c7e316b6e64cc57c2413abe8dce4e377ff62d3b418fba7b0eae87664dbc96ab'){throw 'Wrong published build450 fixture'}
        Copy-Item -LiteralPath $GroundDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build450 upgrade removal failed'}
    }
    if($RecentDll){
        if((Get-FileHash -LiteralPath $RecentDll).Hash.ToLowerInvariant() -ne 'db52e9a31cedf59cc8a78831b216a07cccc0fd4cc22f8c8ea8e96e4043e330b4'){throw 'Wrong published build449 fixture'}
        Copy-Item -LiteralPath $RecentDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build449 upgrade removal failed'}
    }
    if($LatestDll){
        if((Get-FileHash -LiteralPath $LatestDll).Hash.ToLowerInvariant() -ne '180a36ee6c696328bdfbf64ebadb5c3b9f85c1a22952b74df3cad9056b6a8c80'){throw 'Wrong published build448 fixture'}
        Copy-Item -LiteralPath $LatestDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build448 upgrade removal failed'}
    }
    if($LastDll){
        if((Get-FileHash -LiteralPath $LastDll).Hash.ToLowerInvariant() -ne 'bc56062587aed707f7e1be1a6f3d675fc9fd83eb6e36de81628ca51b175e7be9'){throw 'Wrong published build447 fixture'}
        Copy-Item -LiteralPath $LastDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build447 upgrade removal failed'}
    }
    if($PreviousDll){
        if((Get-FileHash -LiteralPath $PreviousDll).Hash.ToLowerInvariant() -ne 'fed0452d496f5078c16b602c2225293ff21afe236cbe618947837f633b2910bb'){throw 'Wrong published build446 fixture'}
        Copy-Item -LiteralPath $PreviousDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build446 upgrade removal failed'}
    }
    if($ContextDll){
        if((Get-FileHash -LiteralPath $ContextDll).Hash.ToLowerInvariant() -ne '268138899971f1d3d5879ca6aec12334f846c8f2f5d1fbf0db41a6e9c527e476'){throw 'Wrong published build445 fixture'}
        Copy-Item -LiteralPath $ContextDll -Destination (Join-Path $game 'dinput8.dll')
        & (Join-Path $PackageDir 'UNINSTALL-RE4-CONTROLS.ps1') -GameBin $game
        if(Test-Path -LiteralPath (Join-Path $game 'dinput8.dll')){throw 'Known build445 upgrade removal failed'}
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
    $fixtureAbsolute = [IO.Path]::GetFullPath($sandbox)
    $fixtureParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    if (-not $fixtureAbsolute.StartsWith($fixtureParent,[StringComparison]::OrdinalIgnoreCase) -or -not [IO.Path]::GetFileName($fixtureAbsolute).StartsWith('re4-installer-test-',[StringComparison]::Ordinal)) { throw 'Unsafe fixture cleanup path' }
    Remove-Item -LiteralPath $fixtureAbsolute -Recurse -Force
}
