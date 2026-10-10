@echo off
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0UNINSTALL-RE4-CONTROLS.ps1"
if errorlevel 1 echo Desinstalacion detenida. Revisa el error anterior.
pause
