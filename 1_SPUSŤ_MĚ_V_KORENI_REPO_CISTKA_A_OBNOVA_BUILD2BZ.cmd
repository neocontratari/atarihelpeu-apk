@echo off
chcp 65001 >nul
setlocal EnableExtensions
cd /d "%~dp0"

echo ============================================================
echo AtariHelp EMU-10: CISTKA AI POKUSU + TVRDA OBNOVA BUILD2BZ
echo ============================================================
echo.

if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Tento soubor musi byt spusten v koreni repozitare atarihelpeu-apk.
  echo Tady nevidim app\src\main\assets\emu\index.html
  echo Rozbal ZIP primo do slozky atarihelpeu-apk a spust tento CMD znovu.
  pause
  exit /b 2
)

if not exist "_RESTORE_BUILD2BZ_ORIGINAL\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi slozka _RESTORE_BUILD2BZ_ORIGINAL.
  echo ZIP nebyl rozbalen cely.
  pause
  exit /b 3
)

echo KROK 1/3: Mazani vsech mych spatnych testovacich overlay souboru v koreni...
echo POZOR: Mazou se jen build2*.js, NAVOD_BUILD2*.txt a check_*.html v KORENI repo.
echo.

del /f /q "build2*.js" 2>nul
del /f /q "NAVOD_BUILD2*.txt" 2>nul
del /f /q "check_*.html" 2>nul
del /f /q "work_*_check.js" 2>nul
del /f /q "*_AUDIO_*_AUDIT*.txt" 2>nul

echo KROK 2/3: Prepis aplikace cistym BUILD2BZ z puvodniho predavaciho balicku...
robocopy "_RESTORE_BUILD2BZ_ORIGINAL" "." /E /COPY:DAT /R:2 /W:1 /NFL /NDL /NP
set RC=%ERRORLEVEL%
if %RC% GEQ 8 (
  echo.
  echo CHYBA: robocopy vratil kod %RC%.
  echo Obnova se nemusela povest.
  pause
  exit /b %RC%
)

echo KROK 3/3: Kontrola, ze zustal jen BUILD2BZ check soubor...
if exist "build2bz_check.js" (
  echo OK: build2bz_check.js obnoven.
) else (
  echo CHYBA: build2bz_check.js chybi.
  pause
  exit /b 4
)

echo.
echo HOTOVO: Repo je vracene na BUILD2BZ original overlay.
echo NEJSOU zde moje BUILD2C/BUILD2D pokusy.
echo Ted otevri GitHub Desktop: melo by ukazat jen cistou obnovu BUILD2BZ nebo zadny chaos.
echo Pak dej Commit jen pokud vidis, ze je to spravne.
echo.
pause
exit /b 0
