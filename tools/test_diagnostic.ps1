$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Get-Content -LiteralPath (Join-Path $root 'DIAGNOSE-RE4-CONTROLS.ps1') -Raw
# Compile the actual interop declarations; do not call Windows APIs on Linux.
$actual = [regex]::Match($source, "(?s)Add-Type @'.*?\r?\n'@").Value
& ([scriptblock]::Create($actual))
$mock = @'
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class RE4BridgeDiagnosticFixture {
    public static bool Present = true;
    public static string[] DinputPaths(uint pid) { return new string[] { "fixture/dinput8.dll" }; }
    public static IntPtr OpenFileMapping(uint access, bool inherit, string name) {
        if (access != 4 || inherit) throw new Exception("Not read-only");
        return Present ? new IntPtr(1) : IntPtr.Zero;
    }
    public static IntPtr MapViewOfFile(IntPtr handle, uint access, uint high, uint low, UIntPtr size) {
        if (access != 4 || size.ToUInt64() != 40) throw new Exception("Invalid mapping arguments");
        IntPtr view = Marshal.AllocHGlobal(40);
        Marshal.WriteInt32(view, 0, 0x52453443);
        Marshal.WriteInt32(view, 4, 1);
        Marshal.WriteInt32(view, 28, 1);
        Marshal.WriteInt32(view, 32, 12);
        return view;
    }
    public static bool UnmapViewOfFile(IntPtr view) { Marshal.FreeHGlobal(view); return true; }
    public static bool CloseHandle(IntPtr handle) { return true; }
}
"@
'@
$fixtureSource = $source.Replace($actual, $mock).Replace('[RE4BridgeDiagnostic]', '[RE4BridgeDiagnosticFixture]').Replace('$PSScriptRoot', '$root').Replace('; exit 1', '; throw')
$fixture = [scriptblock]::Create($fixtureSource)
function Get-Process {
    param($Name, $ErrorAction)
    [pscustomobject]@{ Id = 123; MainModule = [pscustomobject]@{ FileName = 'fixture/bio4.exe' } }
}
$stage = Join-Path ([IO.Path]::GetTempPath()) ('re4-diagnostic-' + [guid]::NewGuid().ToString('N'))
try {
    [void](New-Item -ItemType Directory -Path $stage)
    [IO.File]::WriteAllText((Join-Path $stage 'dinput8.dll'), 'fixture')
    $output = (& $fixture -GameBin $stage 6>&1 | Out-String)
    if ($output -notmatch 'DINPUT8 CARGADA: fixture/dinput8.dll' -or
        $output -notmatch 'IPC PRESENTE, estado: 1, ticks: 12' -or $output -match 'ERROR:') {
        throw ('Present IPC fixture failed: ' + $output)
    }
    # The second execution reuses the fixture type (do not redefine it).
    [RE4BridgeDiagnosticFixture]::Present = $false
    $fixtureSource = $fixtureSource.Replace($mock, '')
    $output = (& ([scriptblock]::Create($fixtureSource)) -GameBin $stage 6>&1 | Out-String)
    if ($output -notmatch 'IPC NO DISPONIBLE' -or $output -match 'ERROR:') {
        throw ('Missing IPC fixture failed: ' + $output)
    }
    Write-Host 'Diagnostic fixtures passed: interop compiled, UIntPtr argument, read-only IPC, loaded DLL and absent channel.'
} finally { Remove-Item -LiteralPath $stage -Recurse -Force }
