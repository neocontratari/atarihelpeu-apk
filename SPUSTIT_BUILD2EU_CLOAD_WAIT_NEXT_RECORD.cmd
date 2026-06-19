@echo off
setlocal EnableExtensions
chcp 65001 >nul
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EU CLOAD WAIT NEXT RECORD FIX
echo ============================================================

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nejsi v koreni projektu. Nevidim app\src\main\assets\emu\index.html
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer a spust to tam.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EU_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi payload _EMU10_BUILD2EU_PAYLOAD.
  echo Zkopiruj do korene projektu i slozku _EMU10_BUILD2EU_PAYLOAD.
  pause
  exit /b 1
)

if not exist "_backup_index" mkdir "_backup_index"
copy /Y "app\src\main\assets\emu\index.html" "_backup_index\index_before_BUILD2EU.html" >nul

copy /Y "_EMU10_BUILD2EU_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html
  pause
  exit /b 1
)

if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"

echo.
echo HOTOVO: BUILD2EU nainstalovan.
echo Zmenen jen app\src\main\assets\emu\index.html
echo CSAVE WAV export / XEX / ROM / Java / UI beze zmeny.
echo.
pause
exit /b 0
