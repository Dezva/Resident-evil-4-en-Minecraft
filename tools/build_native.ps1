param([string]$Compiler = $env:RE4_MINGW_CC)
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (-not $Compiler) { $Compiler = Join-Path (Get-Location) '.gradle\native-toolchain\mingw32\bin\gcc.exe' }
    if (-not (Test-Path -LiteralPath $Compiler -PathType Leaf)) { throw 'Indica un compilador MinGW x86 con -Compiler o RE4_MINGW_CC.' }
    $oldPath = $env:PATH
    $env:PATH = (Split-Path $Compiler -Parent) + ';' + $env:PATH
    New-Item -ItemType Directory -Path 'native\build' -Force | Out-Null
    & $Compiler -D_WIN32_WINNT=0x0601 -msse2 -mfpmath=sse -std=c11 -O2 -Wall -Wextra -Werror -Wno-cast-function-type -shared native/re4_control.c -o native/build/dinput8.dll '-Wl,--kill-at,--no-insert-timestamp' -static-libgcc -lm
    if ($LASTEXITCODE -ne 0) { throw 'Fallo compilando la DLL.' }
    Write-Host 'Compilada native\build\dinput8.dll. No se instala ni se modifica el juego.'
} finally {
    if ($null -ne $oldPath) { $env:PATH = $oldPath }
    Pop-Location
}
