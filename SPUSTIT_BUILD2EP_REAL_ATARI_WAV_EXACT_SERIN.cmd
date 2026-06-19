@echo off
setlocal EnableExtensions

echo ============================================================
echo  AtariHelp.eu EMU-10 BUILD2EP install
echo  REAL_ATARI_WAV_EXACT_SERIN_NO_SKIP
echo ============================================================
echo.

set "ROOT=%CD%"
set "TARGET=%ROOT%\app\src\main\assets\emu\index.html"
set "PAYLOAD=%ROOT%\_EMU10_BUILD2EP_PAYLOAD\app\src\main\assets\emu\index.html"

if not exist "%ROOT%\app\src\main\assets\emu" (
  echo CHYBA: Nejsi v koreni projektu.
  echo Nevidim slozku: app\src\main\assets\emu
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer
  echo a spust tento CMD primo tam, vedle slozky app.
  echo.
  pause
  exit /b 1
)

if not exist "%TARGET%" (
  echo CHYBA: Nevidim soubor:
  echo %TARGET%
  echo Nechci nic mazat naslepo.
  echo.
  pause
  exit /b 1
)

if not exist "%PAYLOAD%" (
  echo CHYBA: Nevidim payload index.html:
  echo %PAYLOAD%
  echo Zkopiruj do korene projektu i slozku _EMU10_BUILD2EP_PAYLOAD.
  echo.
  pause
  exit /b 1
)

for /f "tokens=1-4 delims=/-. " %%a in ("%date%") do set D=%%d%%b%%c
for /f "tokens=1-3 delims=:,. " %%a in ("%time%") do set T=%%a%%b%%c
set "BACKUP=%TARGET%.backup_BUILD2EP_%D%_%T%"

echo Zalohuji puvodni index.html...
copy /Y "%TARGET%" "%BACKUP%" >nul
if errorlevel 1 (
  echo CHYBA: Nejde vytvorit zaloha. Koncim.
  pause
  exit /b 1
)

echo Nahrazuji pouze app\src\main\assets\emu\index.html ...
copy /Y "%PAYLOAD%" "%TARGET%" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html.
  pause
  exit /b 1
)

echo Cistim jen build cache, ne zdroje...
if exist "%ROOT%\app\build" rmdir /S /Q "%ROOT%\app\build"
if exist "%ROOT%\build" rmdir /S /Q "%ROOT%\build"
if exist "%ROOT%\.gradle" rmdir /S /Q "%ROOT%\.gradle"

echo.
echo HOTOVO: BUILD2EP nainstalovan.
echo Zmenen jen app\src\main\assets\emu\index.html
echo Zaloha: %BACKUP%
echo.
echo GitHub Desktop:
echo Summary: BUILD2EP exact SERIN no skip
echo Commit to main
echo Push origin
echo.
pause
exit /b 0
