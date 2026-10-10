@echo off
setlocal
py -3 -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_py
python -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_python
echo Se requiere Python 3.10 o posterior.
pause
exit /b 1

:use_py
py -3 "%~dp0tools\re4_live_host.py" --game-dir "%APPDATA%\.minecraft"
goto finished

:use_python
python "%~dp0tools\re4_live_host.py" --game-dir "%APPDATA%\.minecraft"

:finished
if errorlevel 1 echo Revisa el error anterior. RE4 debe estar abierto y el simulador cerrado.
pause
