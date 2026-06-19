@echo off
setlocal
cd /d "%~dp0"
echo.
echo ===============================================
echo  AtariHelp.eu EMU-10 BUILD2ET CLOAD checksum fix
echo ===============================================
echo.
if not exist "app\src\main\assets\emu" (
  echo CHYBA: Tento CMD musi byt spusten v koreni projektu.
  echo Spravna slozka obsahuje: app, gradle, build.gradle, settings.gradle
  echo Aktualni slozka:
  cd
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2ET_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Chybi payload _EMU10_BUILD2ET_PAYLOAD.
  echo Zkopiruj do korene projektu CMD i slozku _EMU10_BUILD2ET_PAYLOAD.
  pause
  exit /b 1
)
echo Zalohuji puvodni index.html...
copy /Y "app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index_BACKUP_BEFORE_BUILD2ET.html" >nul
if errorlevel 1 (
  echo CHYBA: nepodarilo se vytvorit zalohu index.html
  pause
  exit /b 1
)
echo Nahrazuji jen app\src\main\assets\emu\index.html ...
copy /Y "_EMU10_BUILD2ET_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: nepodarilo se nahradit index.html
  pause
  exit /b 1
)
echo Cistim pouze build cache...
if exist "app\build" rmdir /S /Q "app\build"
if exist "build" rmdir /S /Q "build"
if exist ".gradle" rmdir /S /Q ".gradle"
echo.
echo HOTOVO: BUILD2ET nainstalovan.
echo Zmenen byl pouze app\src\main\assets\emu\index.html
echo CSAVE WAV export / XEX / Java / ROM / UI se nemenily.
echo.
pause
exit /b 0
