@echo off
setlocal
rem SKLauncher instance supplied by the user: %%APPDATA%%\.minecraft
rem Prefer the launcher, then an actual compatible python.exe on PATH.
py -3 -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_py
python -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_python
echo No se encontro Python 3.10 o posterior disponible como py o python.
echo Instala Python para Windows desde https://www.python.org/downloads/windows/
echo En el instalador clasico, marca Add python.exe to PATH y el lanzador py.
echo Cierra esta ventana despues de instalarlo y vuelve a abrir START-SIMULATOR.bat.
pause
exit /b 1

:use_py
py -3 "%~dp0tools\fake_re4_host.py" --game-dir "%APPDATA%\.minecraft"
goto finished

:use_python
python "%~dp0tools\fake_re4_host.py" --game-dir "%APPDATA%\.minecraft"

:finished
if errorlevel 1 (
  echo.
  echo Revisa el error anterior y que hayas iniciado Minecraft con el mod.
)
pause
