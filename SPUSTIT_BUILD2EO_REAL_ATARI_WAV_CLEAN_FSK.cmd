@echo off
setlocal
title BUILD2EO REAL ATARI WAV CLEAN FSK CLOAD

echo ============================================================
echo BUILD2EO REAL ATARI WAV CLEAN FSK CLOAD
echo ============================================================
echo.

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nejsi v koreni projektu.
  echo Nevidim app\src\main\assets\emu\index.html
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer
  echo a spust CMD v te slozce, kde je app, gradle, build.gradle.
  echo.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EO_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Vedle CMD neni slozka _EMU10_BUILD2EO_PAYLOAD.
  echo Zkopiruj do korene projektu oba soubory/slozky z installeru pres F5.
  echo.
  pause
  exit /b 1
)

if not exist "_BACKUP_INDEX_HTML" mkdir "_BACKUP_INDEX_HTML"
copy /Y "app\src\main\assets\emu\index.html" "_BACKUP_INDEX_HTML\index_before_BUILD2EO.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se udelat zalohu index.html
  pause
  exit /b 1
)

copy /Y "_EMU10_BUILD2EO_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit app\src\main\assets\emu\index.html
  pause
  exit /b 1
)

echo HOTOVO: BUILD2EO nainstalovan.
echo Zmenen byl jen app\src\main\assets\emu\index.html
echo Zaloha je v _BACKUP_INDEX_HTML\index_before_BUILD2EO.html
echo.
echo Ted GitHub Desktop: Summary BUILD2EO real Atari wav clean FSK cload
echo Commit to main, Push origin, potom WWW/Actions APK.
echo.
pause
exit /b 0
