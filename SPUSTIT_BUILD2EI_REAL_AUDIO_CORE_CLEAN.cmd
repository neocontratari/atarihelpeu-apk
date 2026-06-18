@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EI REAL AUDIO CORE CLEAN
echo ============================================================
echo.
if not exist "app\src\main\assets\emu" (
  echo CHYBA: Tenhle CMD musi byt spusten v KORENI projektu.
  echo Spravne: C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
  echo Tady musi byt slozka app\src\main\assets\emu
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2EI_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi payload _EMU10_BUILD2EI_PAYLOAD.
  echo Zkopiruj do korene projektu SOUCASNE tento CMD i slozku _EMU10_BUILD2EI_PAYLOAD.
  pause
  exit /b 1
)
set TS=%DATE%_%TIME%
set TS=%TS::=-%
set TS=%TS:/=-%
set TS=%TS:.=-%
set TS=%TS: =_%
if exist "app\src\main\assets\emu\index.html" (
  copy /Y "app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html.BACKUP_BEFORE_BUILD2EI_%TS%" >nul
  echo Zaloha stareho index.html vytvorena.
)
copy /Y "_EMU10_BUILD2EI_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html.
  pause
  exit /b 1
)
echo Nahrano: app\src\main\assets\emu\index.html

echo Cistim jen build cache, ne zdroje...
if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"

echo Uklizim stare pomocne payloady BUILD2EC/ED/EE/EF/EG/EH, pokud existuji...
for %%D in (_EMU10_BUILD2EC_PAYLOAD _EMU10_BUILD2ED_PAYLOAD _EMU10_BUILD2EE_PAYLOAD _EMU10_BUILD2EF_PAYLOAD _EMU10_BUILD2EG_PAYLOAD _EMU10_BUILD2EH_PAYLOAD build2ec_cmd_installer build2ed_cmd_installer build2ef_cmd_installer build2eg_cmd_installer build2eh_cmd_installer) do (
  if exist "%%D" rmdir /S /Q "%%D"
)

echo.
echo HOTOVO: BUILD2EI nainstalovan.
echo Ted otevri GitHub Desktop: Commit to main, Push origin.
echo Summary: BUILD2EI real audio core clean
echo.
pause
