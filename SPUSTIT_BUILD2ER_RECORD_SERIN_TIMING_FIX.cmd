@echo off
setlocal
title AtariHelp EMU10 BUILD2ER CLOAD timing fix
echo.
echo ============================================================
echo  AtariHelp.eu EMU-10 BUILD2ER RECORD SERIN TIMING FIX
echo ============================================================
echo.
if not exist "app\src\main\assets\emu" (
  echo CHYBA: nejsi v korenovem adresari projektu.
  echo Spust to v adresari kde je slozka app a build.gradle.
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2ER_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: chybi payload _EMU10_BUILD2ER_PAYLOAD.
  echo Zkopiruj CMD i slozku _EMU10_BUILD2ER_PAYLOAD do korene projektu.
  pause
  exit /b 1
)
if not exist "_backup_index" mkdir "_backup_index"
copy /Y "app\src\main\assets\emu\index.html" "_backup_index\index_before_BUILD2ER.html" >nul
copy /Y "_EMU10_BUILD2ER_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: kopirovani index.html selhalo.
  pause
  exit /b 1
)
rmdir /S /Q "app\build" 2>nul
rmdir /S /Q "build" 2>nul
rmdir /S /Q ".gradle" 2>nul
echo.
echo HOTOVO: BUILD2ER nainstalovan.
echo Zmenen byl jen: app\src\main\assets\emu\index.html
echo CSAVE WAV export / XEX / Java / ROM / UI zustava beze zmen.
echo.
pause
endlocal
