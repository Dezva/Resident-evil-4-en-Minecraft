@echo off
setlocal
py -3 -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_py
python -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)" >nul 2>nul
if not errorlevel 1 goto use_python
echo Se requiere Python 3.10 o posterior. Usa el mismo Python que el simulador.
pause
exit /b 1

:use_py
py -3 "%~dp0tools\re4_reader.py"
goto finished

:use_python
python "%~dp0tools\re4_reader.py"

:finished
if errorlevel 1 echo Revisa el error anterior. Abre RE4 y carga una partida antes de iniciar el lector.
pause
