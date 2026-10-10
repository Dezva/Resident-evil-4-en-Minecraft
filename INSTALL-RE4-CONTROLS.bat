@echo off
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0INSTALL-RE4-CONTROLS.ps1"
if errorlevel 1 echo Instalacion detenida. Revisa el error anterior; no reemplaces DLL existentes.
pause
