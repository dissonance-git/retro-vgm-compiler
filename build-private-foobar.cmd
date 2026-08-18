@echo off
setlocal
cd /d "%~dp0"

echo Retro VGM Compiler private foobar build
echo =======================================
echo.

where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Windows PowerShell was not found.
  pause
  exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build_private_foobar_components.ps1"
if errorlevel 1 (
  echo.
  echo BUILD FAILED. The canonical builder printed the missing prerequisite or failing validation above.
  echo Nothing was installed into foobar2000.
  pause
  exit /b 1
)

echo.
echo BUILD VERIFIED.
echo Output: %~dp0dist\private-components
echo.
echo Manual replacement payloads are inside the component packages:
echo   VGM: foo_input_vgm.dll + omniphony_source.dll
echo   SPC: foo_snesapu.dll + spcplayer.exe + SNESAPU.dll + omniphony_source.dll
echo.
start "" explorer.exe "%~dp0dist\private-components"
pause
exit /b 0
