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
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    private struct ModuleEntry {
        public uint size, moduleId, processId, globalUsage, processUsage;
        public IntPtr baseAddress;
        public uint baseSize;
        public IntPtr module;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)] public string name;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string path;
    }
    [DllImport("kernel32.dll", SetLastError=true)]
    private static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint pid);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool Module32FirstW(IntPtr snapshot, ref ModuleEntry entry);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    private static extern bool Module32NextW(IntPtr snapshot, ref ModuleEntry entry);
    public static string[] DinputPaths(uint pid) {
        // Include WOW64 modules when the diagnostic runs in 64-bit PowerShell.
        IntPtr snapshot = CreateToolhelp32Snapshot(0x18, pid);
        if (snapshot == new IntPtr(-1)) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
        try {
            ModuleEntry entry = new ModuleEntry();
            entry.size = (uint)Marshal.SizeOf(typeof(ModuleEntry));
            if (!Module32FirstW(snapshot, ref entry)) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
            var paths = new System.Collections.Generic.List<string>();
            do {
                if (String.Equals(entry.name, "dinput8.dll", StringComparison.OrdinalIgnoreCase)) paths.Add(entry.path);
            } while (Module32NextW(snapshot, ref entry));
            int error = Marshal.GetLastWin32Error();
            if (error != 18) throw new System.ComponentModel.Win32Exception(error);
            return paths.ToArray();
        } finally { CloseHandle(snapshot); }
    }
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
            $modules = @([RE4BridgeDiagnostic]::DinputPaths([uint32]$game.Id))
            if ($modules.Count -eq 0) { Write-Host 'DINPUT8 NO CARGADA en este proceso.' }
            foreach ($module in $modules) { Write-Host ('DINPUT8 CARGADA: ' + $module) }
        } catch { Write-Host ('No se pudieron enumerar los modulos: ' + $_.Exception.Message) }
        $handle = [RE4BridgeDiagnostic]::OpenFileMapping(4, $false, ('Local\RE4Bridge-Control-' + $game.Id))
        if ($handle -eq [IntPtr]::Zero) {
            $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            Write-Host ('IPC NO DISPONIBLE, Windows ' + $code)
            continue
        }
        try {
            $view = [RE4BridgeDiagnostic]::MapViewOfFile($handle, 4, 0, 0, [UIntPtr]::new([uint32]40))
            if ($view -eq [IntPtr]::Zero) {
                throw ('MapViewOfFile Windows ' + [Runtime.InteropServices.Marshal]::GetLastWin32Error())
            }
            try {
                $magic = [Runtime.InteropServices.Marshal]::ReadInt32($view, 0)
                $version = [Runtime.InteropServices.Marshal]::ReadInt32($view, 4)
                if ($magic -ne 0x52453443 -or $version -ne 1) { throw 'IPC con identificador/version inesperados.' }
                $status = [Runtime.InteropServices.Marshal]::ReadInt32($view, 28)
                $ticks = [Runtime.InteropServices.Marshal]::ReadInt32($view, 32)
                Write-Host ('IPC PRESENTE, estado: ' + $status + ', ticks: ' + $ticks)
                Write-Host 'Estados: 1 listo; 2 firmas/version no aceptadas; 3 fallo al instalar hook.'
            } finally { [void][RE4BridgeDiagnostic]::UnmapViewOfFile($view) }
        } finally { [void][RE4BridgeDiagnostic]::CloseHandle($handle) }
    }
} catch { Write-Host ('ERROR: ' + $_.Exception.Message); exit 1 }
