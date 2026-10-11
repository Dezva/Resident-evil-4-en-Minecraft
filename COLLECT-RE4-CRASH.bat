@echo off
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0COLLECT-RE4-CRASH.ps1"
if errorlevel 1 echo No se pudo completar el informe. Revisa el mensaje anterior.
pause
