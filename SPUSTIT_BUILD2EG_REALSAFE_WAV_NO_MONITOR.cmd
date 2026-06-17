@echo off
setlocal
title AtariHelp EMU10 BUILD2EG REALSAFE WAV NO MONITOR

echo.
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EG REALSAFE WAV NO MONITOR
echo ============================================================
echo.
echo Tento skript musi byt spusten v koreni projektu:
echo C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk
echo.
echo Kontroluji slozky...

if not exist "app\src\main\assets\emu" (
  echo.
  echo CHYBA: Nevidim app\src\main\assets\emu
  echo Nejsi v koreni projektu. Spust CMD z adresare, kde je slozka app.
  echo.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EG_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo.
  echo CHYBA: Nevidim payload _EMU10_BUILD2EG_PAYLOAD\app\src\main\assets\emu\index.html
  echo Zkopiruj do korene projektu i slozku _EMU10_BUILD2EG_PAYLOAD.
  echo.
  pause
  exit /b 1
)

echo OK: projekt vypada spravne.
echo.

set BACKUPDIR=_EMU10_BACKUP_BUILD2EG_%DATE:~-4%%DATE:~3,2%%DATE:~0,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%
set BACKUPDIR=%BACKUPDIR: =0%
mkdir "%BACKUPDIR%" >nul 2>nul

if exist "app\src\main\assets\emu\index.html" (
  copy /Y "app\src\main\assets\emu\index.html" "%BACKUPDIR%\index_before_BUILD2EG.html" >nul
  echo ZALOHA: %BACKUPDIR%\index_before_BUILD2EG.html
)

echo.
echo NAHRAZUJI: app\src\main\assets\emu\index.html
copy /Y "_EMU10_BUILD2EG_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Kopirovani index.html selhalo.
  pause
  exit /b 1
)

echo.
echo CISTIM JEN BUILD CACHE, NE ZDROJE:
if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"

echo.
echo MAZU STARE INSTALLER PAYLOADY, ABY SE NEPLETLY:
if exist "_EMU10_BUILD2EC_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2EC_PAYLOAD"
if exist "_EMU10_BUILD2ED_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2ED_PAYLOAD"
if exist "_EMU10_BUILD2EE_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2EE_PAYLOAD"
if exist "_EMU10_BUILD2EF_PAYLOAD" rmdir /S /Q "_EMU10_BUILD2EF_PAYLOAD"

echo.
echo HOTOVO: BUILD2EG nainstalovan.
echo.
echo Zmenen byl jen:
echo app\src\main\assets\emu\index.html
echo.
echo GitHub Desktop:
echo Summary: BUILD2EG realsafe wav no monitor
echo Commit to main
echo Push origin
echo.
pause
