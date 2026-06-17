@echo off
setlocal EnableExtensions
cd /d "%~dp0"

echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EC - CLOAD OPRAVA PRES CMD
echo ============================================================
echo Tento CMD sahne jen na:
echo   app\src\main\assets\emu\index.html
echo Zachova Java cast, XEX, CSAVE, ikony, hlavni menu a ostatni assety.
echo Smaze jen build cache: app\build, build, .gradle
echo ============================================================
echo.

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Tento CMD neni spusten v KORENI Android projektu.
  echo.
  echo Spravna slozka je ta, kde vidis:
  echo   app
  echo   gradle
  echo   build.gradle nebo build.gradle.kts
  echo   settings.gradle nebo settings.gradle.kts
  echo.
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer
  echo a obsah tohoto ZIPu zkopiruj prave TAM.
  echo.
  pause
  exit /b 1
)

if not exist "_EMU10_BUILD2EC_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi slozka _EMU10_BUILD2EC_PAYLOAD.
  echo Zkopiruj do korene projektu VSE z tohoto ZIPu, ne jen samotny CMD.
  echo.
  pause
  exit /b 1
)

set TS=%date:~-4%%date:~3,2%%date:~0,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set TS=%TS: =0%
set BAK=backup_EMU10_before_BUILD2EC_%TS%
mkdir "%BAK%" >nul 2>nul
copy /Y "app\src\main\assets\emu\index.html" "%BAK%\index_BEFORE_BUILD2EC.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se udelat zalohu puvodniho index.html
  echo Nic jsem nemenil.
  echo.
  pause
  exit /b 1
)

echo Zaloha hotova:
echo   %BAK%\index_BEFORE_BUILD2EC.html

echo.
echo Prepisuji jen CLOAD core soubor...
copy /Y "_EMU10_BUILD2EC_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit app\src\main\assets\emu\index.html
  echo Zaloha je ulozena ve slozce %BAK%
  echo.
  pause
  exit /b 1
)

echo OK: CLOAD core obnoven / vycisten.
echo.

echo Cistim jen build cache...
if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"

echo.
echo ============================================================
echo HOTOVO.
echo Ted udelej:
echo 1^) GitHub Desktop - uvidis zmeny
echo 2^) Summary: BUILD2EC CLOAD core restore
echo 3^) Commit to main
echo 4^) Push origin
echo 5^) Na webu/GitHub Actions pockej na APK
echo ============================================================
echo.
echo DULEZITE: Pokud se v GitHub Desktop zmeni jen index.html,
echo je to spravne. Tento fix nema rozhazet Java/XEX/CSAVE cast.
echo.
pause
endlocal
