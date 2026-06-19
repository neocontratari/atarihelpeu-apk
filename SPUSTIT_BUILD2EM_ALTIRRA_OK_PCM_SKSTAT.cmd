@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EM ALTIRRA OK WAV PCM SKSTAT
echo ============================================================
echo.
if not exist "app\src\main\assets\emu" (
  echo CHYBA: Tenhle CMD musi byt spusten v KORENI projektu.
  echo Spravne: C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
  echo Tady musi byt slozka app\src\main\assets\emu
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2EM_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi payload _EMU10_BUILD2EM_PAYLOAD.
  echo Zkopiruj do korene projektu SOUCASNE tento CMD i slozku _EMU10_BUILD2EM_PAYLOAD.
  pause
  exit /b 1
)
set TS=%DATE%_%TIME%
set TS=%TS::=-%
set TS=%TS:/=-%
set TS=%TS:.=-%
set TS=%TS: =_%
if exist "app\src\main\assets\emu\index.html" (
  copy /Y "app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html.BACKUP_BEFORE_BUILD2EM_%TS%" >nul
  echo Zaloha stareho index.html vytvorena.
)
copy /Y "_EMU10_BUILD2EM_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
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

echo Uklizim stare pomocne payloady BUILD2EC/ED/EE/EF/EG/EH/EI/EJ/EK/EL, pokud existuji...
for %%D in (_EMU10_BUILD2EC_PAYLOAD _EMU10_BUILD2ED_PAYLOAD _EMU10_BUILD2EE_PAYLOAD _EMU10_BUILD2EF_PAYLOAD _EMU10_BUILD2EG_PAYLOAD _EMU10_BUILD2EH_PAYLOAD _EMU10_BUILD2EI_PAYLOAD _EMU10_BUILD2EJ_PAYLOAD _EMU10_BUILD2EK_PAYLOAD _EMU10_BUILD2EL_PAYLOAD build2ec_cmd_installer build2ed_cmd_installer build2ef_cmd_installer build2eg_cmd_installer build2eh_cmd_installer build2ei_cmd_installer build2ej_cmd_installer build2ek_cmd_installer build2el_cmd_installer) do (
  if exist "%%D" rmdir /S /Q "%%D"
)

echo.
echo HOTOVO: BUILD2EM nainstalovan.
echo Ted otevri GitHub Desktop: Commit to main, Push origin.
echo Summary: BUILD2EM Altirra OK PCM SKSTAT
echo.
pause
