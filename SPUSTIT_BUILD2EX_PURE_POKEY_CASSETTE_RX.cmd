@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EX PURE POKEY CASSETTE RX
echo ============================================================
echo.
if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nejsi v koreni projektu. Nevidim app\src\main\assets\emu\index.html
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer a spust to tam.
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2EX_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi _EMU10_BUILD2EX_PAYLOAD.
  echo Zkopiruj do korene projektu i slozku _EMU10_BUILD2EX_PAYLOAD.
  pause
  exit /b 1
)
if not exist "_backup_index" mkdir "_backup_index"
copy /Y "app\src\main\assets\emu\index.html" "_backup_index\index_before_BUILD2EX_%DATE:/=-%_%TIME::=-%.html" >nul
copy /Y "_EMU10_BUILD2EX_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html
  pause
  exit /b 1
)
rmdir /S /Q "app\build" 2>nul
rmdir /S /Q "build" 2>nul
rmdir /S /Q ".gradle" 2>nul
echo.
echo HOTOVO: BUILD2EX nainstalovan.
echo Menen jen app\src\main\assets\emu\index.html
echo CSAVE WAV export, XEX, ROM, Java, UI, klavesnice a joystick zustaly beze zmeny.
echo.
pause
