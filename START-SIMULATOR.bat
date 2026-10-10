@echo off
rem SKLauncher instance supplied by the user: %%APPDATA%%\.minecraft
py -3 "%~dp0tools\fake_re4_host.py" --game-dir "%APPDATA%\.minecraft"
if errorlevel 1 (
  echo.
  echo Revisa que Python 3 este instalado y que hayas iniciado Minecraft con el mod.
)
pause
