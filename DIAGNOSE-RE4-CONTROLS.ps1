param([string]$GameBin = 'D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32')
$ErrorActionPreference = 'Stop'
# Read-only: inspect files/modules and open IPC with FILE_MAP_READ only.
try {
    $installed = Join-Path $GameBin 'dinput8.dll'
    $bundled = Join-Path $PSScriptRoot 'native\dinput8.dll'
    foreach ($file in @($installed, $bundled)) {
        if (Test-Path -LiteralPath $file) {
            Write-Host ('DLL: ' + $file)
            Write-Host ('SHA256: ' + (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash)
        } else { Write-Host ('NO EXISTE: ' + $file) }
    }
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class RE4BridgeDiagnostic {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr OpenFileMapping(uint access, bool inherit, string name);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern IntPtr MapViewOfFile(IntPtr mapping, uint access, uint high, uint low, UIntPtr length);
    [DllImport("kernel32.dll")] public static extern bool UnmapViewOfFile(IntPtr view);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
    $games = @(Get-Process bio4 -ErrorAction SilentlyContinue)
    if ($games.Count -eq 0) { throw 'Abre RE4 y carga una partida antes del diagnostico.' }
    foreach ($game in $games) {
        Write-Host ('RE4 PID: ' + $game.Id)
        try {
            Write-Host ('EXE: ' + $game.MainModule.FileName)
            $modules = @($game.Modules | Where-Object { $_.ModuleName -ieq 'dinput8.dll' })
            if ($modules.Count -eq 0) { Write-Host 'DINPUT8 NO CARGADA en este proceso.' }
            foreach ($module in $modules) { Write-Host ('DINPUT8 CARGADA: ' + $module.FileName) }
        } catch { Write-Host ('No se pudieron enumerar los modulos: ' + $_.Exception.Message) }
        $handle = [RE4BridgeDiagnostic]::OpenFileMapping(4, $false, ('Local\RE4Bridge-Control-' + $game.Id))
        if ($handle -eq [IntPtr]::Zero) {
            $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            Write-Host ('IPC NO DISPONIBLE, Windows ' + $code)
            continue
        }
        try {
            $view = [RE4BridgeDiagnostic]::MapViewOfFile($handle, 4, 0, 0, [UIntPtr]40)
            if ($view -eq [IntPtr]::Zero) {
                throw ('MapViewOfFile Windows ' + [Runtime.InteropServices.Marshal]::GetLastWin32Error())
            }
            try {
                $status = [Runtime.InteropServices.Marshal]::ReadInt32($view, 28)
                $ticks = [Runtime.InteropServices.Marshal]::ReadInt32($view, 32)
                Write-Host ('IPC PRESENTE, estado: ' + $status + ', ticks: ' + $ticks)
                Write-Host 'Estados: 1 listo; 2 firmas/version no aceptadas; 3 fallo al instalar hook.'
            } finally { [void][RE4BridgeDiagnostic]::UnmapViewOfFile($view) }
        } finally { [void][RE4BridgeDiagnostic]::CloseHandle($handle) }
    }
} catch { Write-Host ('ERROR: ' + $_.Exception.Message); exit 1 }
