@echo off
setlocal
powershell.exe -NoLogo -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0VIEW-RE4.ps1" %*
if errorlevel 1 pause
