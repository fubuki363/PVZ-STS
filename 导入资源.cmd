@echo off
setlocal
if "%~1"=="" (
  echo Drag the original GOTY resource ZIP onto this file.
  echo Only main.pak and properties will be imported.
  pause
  exit /b 1
)
python "%~dp0scripts\prepare-windows-resources.py" "%~1" --output "%~dp0dist"
set "PVZ_EXIT=%ERRORLEVEL%"
pause
exit /b %PVZ_EXIT%
