@echo off
chcp 65001 >nul
setlocal
cls
echo ============================================================
echo  AtariHelp.eu EMU-10 BUILD2EF - TVRDA CLOAD/WAV OPRAVA
echo ============================================================
echo.
echo Tento skript se musi spustit V KORENI projektu:
echo   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
echo.

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nevidim app\src\main\assets\emu\index.html
  echo Nejsi v koreni projektu. Presun tento CMD vedle slozky app a pust znovu.
  echo.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EF_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Nevidim payload _EMU10_BUILD2EF_PAYLOAD\app\src\main\assets\emu\index.html
  echo Zkopiruj do korene projektu i celou slozku _EMU10_BUILD2EF_PAYLOAD.
  echo.
  pause
  exit /b 1
)

set BACKUP=_RESTORE_PRED_BUILD2EF_%RANDOM%%RANDOM%
mkdir "%BACKUP%" >nul 2>nul
copy /Y "app\src\main\assets\emu\index.html" "%BACKUP%\index_pred_BUILD2EF.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se zazalohovat puvodni index.html
  pause
  exit /b 1
)

echo Zalohovano do: %BACKUP%\index_pred_BUILD2EF.html
echo.

echo Tvrde prepisuji pouze emu jadro index.html ...
copy /Y "_EMU10_BUILD2EF_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Kopirovani BUILD2EF index.html selhalo.
  pause
  exit /b 1
)

echo OK: app\src\main\assets\emu\index.html nahrazen BUILD2EF.
echo.
echo Cistim jen build cache a stare instalacni bordel-slozky, ne ROM, ne hry, ne Javu...

if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"

if exist "_EMU10_BUILD2EC_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2EC_PAYLOAD"
if exist "_EMU10_BUILD2ED_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2ED_PAYLOAD"
if exist "_EMU10_BUILD2EE_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2EE_PAYLOAD"
if exist "SPUSTIT_BUILD2EC_CLOAD_OPRAVA.cmd" del /Q "SPUSTIT_BUILD2EC_CLOAD_OPRAVA.cmd"
if exist "SPUSTIT_BUILD2ED_REAL_WAV_IRG_AUDIT.cmd" del /Q "SPUSTIT_BUILD2ED_REAL_WAV_IRG_AUDIT.cmd"
if exist "SPUSTIT_BUILD2EE_CLOAD_OPRAVA.cmd" del /Q "SPUSTIT_BUILD2EE_CLOAD_OPRAVA.cmd"

echo.
echo ============================================================
echo HOTOVO - BUILD2EF nainstalovan.
echo ============================================================
echo.
echo V GitHub Desktop ted udelej:
echo   Summary: BUILD2EF hard CLOAD WAV standard
echo   Commit to main
echo   Push origin
echo.
echo Po nove APK hledej v LOGu:
echo   AtariHelp.eu EMU-10 BUILD2EF_HARD_CLOAD_WAV_STANDARD pripraven.
echo   SAVE WAV BUILD2EF
echo   recordAware=ANO
echo   timelineAware=NE

echo.
pause
exit /b 0
