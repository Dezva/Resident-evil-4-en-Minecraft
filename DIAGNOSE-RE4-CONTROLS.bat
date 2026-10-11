@echo off
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0DIAGNOSE-RE4-CONTROLS.ps1"
pause
