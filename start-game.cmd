@echo off
setlocal
set "PVZ_EXE=%~dp0dist\pvz-portable-glass.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable-stoke.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable-sandbox.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable-debug.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable-defect.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable-hd.exe"
if not exist "%PVZ_EXE%" set "PVZ_EXE=%~dp0dist\pvz-portable.exe"
if not exist "%PVZ_EXE%" (
  echo Missing dist\pvz-portable.exe. Build the Windows version first.
  pause
  exit /b 1
)
if not exist "%~dp0dist\main.pak" (
  echo Missing game resources. Import a GOTY resource ZIP first.
  pause
  exit /b 1
)
pushd "%~dp0dist" || exit /b 1
REM Preserve the player's progress from the first HD preview on first launch.
if not exist "%~dp0dist\.hd-save-migrated" if exist "%~dp0.tools\hd-smoke-save\userdata" (
  xcopy "%~dp0.tools\hd-smoke-save\*" "%~dp0dist\savedata\" /E /I /Y /D >nul
  type nul > "%~dp0dist\.hd-save-migrated"
)
if not exist "%~dp0dist\.defect-save-migrated" if exist "%~dp0.tools\defect-smoke-save\userdata" (
  xcopy "%~dp0.tools\defect-smoke-save\*" "%~dp0dist\savedata\" /E /I /Y /D >nul
  type nul > "%~dp0dist\.defect-save-migrated"
)
"%PVZ_EXE%" -savedir="%~dp0dist\savedata" %*
set "PVZ_EXIT=%ERRORLEVEL%"
popd
if not "%PVZ_EXIT%"=="0" (
  echo Game exited with error %PVZ_EXIT%.
  pause
)
exit /b %PVZ_EXIT%
