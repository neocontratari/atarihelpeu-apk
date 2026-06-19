@echo off
setlocal
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EW RECORD5 BUFFER FLAG FIX
echo ============================================================
echo.

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nejsi v koreni projektu. Nevidim app\src\main\assets\emu\index.html
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer a spust to tam.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EW_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi _EMU10_BUILD2EW_PAYLOAD. Zkopiruj CMD i payload do korene projektu.
  pause
  exit /b 1
)

set BACKUPDIR=_backup_index_BUILD2EW_%DATE:/=-%_%TIME::=-%
set BACKUPDIR=%BACKUPDIR: =_%
mkdir "%BACKUPDIR%" >nul 2>nul
copy /Y "app\src\main\assets\emu\index.html" "%BACKUPDIR%\index.html" >nul

copy /Y "_EMU10_BUILD2EW_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html
  pause
  exit /b 1
)

echo Mazani build cache...
rmdir /S /Q "app\build" 2>nul
rmdir /S /Q "build" 2>nul
rmdir /S /Q ".gradle" 2>nul

echo.
echo HOTOVO: BUILD2EW nainstalovan.
echo Zmenen jen app\src\main\assets\emu\index.html
echo KODY JSOU STEJNE
echo.
pause
exit /b 0
