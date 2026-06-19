@echo off
setlocal EnableExtensions

title AtariHelp EMU10 BUILD2EQ record SERIN CLOAD installer
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EQ - RECORD SERIN CLOAD CLEAN
echo ============================================================
echo.
echo KODY JSOU STEJNE
echo Meni se jen: app\src\main\assets\emu\index.html
echo CSAVE WAV export / XEX / UI / ROM / Java zustavaji bez zmeny.
echo.

set "PROJ="
set "PAYLOAD="

if exist "%CD%\app\src\main\assets\emu\index.html" set "PROJ=%CD%"
if not defined PROJ if exist "%USERPROFILE%\Documents\GitHub\atarihelpeu-apk\app\src\main\assets\emu\index.html" set "PROJ=%USERPROFILE%\Documents\GitHub\atarihelpeu-apk"
if not defined PROJ if exist "C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\app\src\main\assets\emu\index.html" set "PROJ=C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk"

if exist "%~dp0_EMU10_BUILD2EQ_PAYLOAD\app\src\main\assets\emu\index.html" set "PAYLOAD=%~dp0_EMU10_BUILD2EQ_PAYLOAD\app\src\main\assets\emu\index.html"
if not defined PAYLOAD if exist "%CD%\_EMU10_BUILD2EQ_PAYLOAD\app\src\main\assets\emu\index.html" set "PAYLOAD=%CD%\_EMU10_BUILD2EQ_PAYLOAD\app\src\main\assets\emu\index.html"

echo Project detected: %PROJ%
echo Payload detected: %PAYLOAD%
echo.

if not defined PROJ (
  echo ERROR: Project root not found.
  echo Open GitHub Desktop - Repository - Show in Explorer.
  echo Then copy this CMD and _EMU10_BUILD2EQ_PAYLOAD into that folder and run it there.
  pause
  exit /b 1
)

if not defined PAYLOAD (
  echo ERROR: Payload folder not found next to this CMD.
  echo Copy BOTH items from the ZIP into the project root:
  echo   SPUSTIT_BUILD2EQ_RECORD_SERIN_CLOAD.cmd
  echo   _EMU10_BUILD2EQ_PAYLOAD
  pause
  exit /b 1
)

echo [1/5] Backup current index.html...
copy /Y "%PROJ%\app\src\main\assets\emu\index.html" "%PROJ%\app\src\main\assets\emu\index.html.BACKUP_BEFORE_BUILD2EQ_%RANDOM%" >nul
if errorlevel 1 (
  echo ERROR: Backup failed.
  pause
  exit /b 1
)

echo [2/5] Replace only app\src\main\assets\emu\index.html...
copy /Y "%PAYLOAD%" "%PROJ%\app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo ERROR: Replace failed.
  pause
  exit /b 1
)

echo [3/5] Remove older experimental CLOAD installer payloads from project root if present...
rd /S /Q "%PROJ%\_EMU10_BUILD2EK_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EM_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EN_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EN2_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EO_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EP_PAYLOAD" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EK*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EM*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EN*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EO*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EP*.cmd" 2>nul

echo [4/5] Clear build cache only...
rd /S /Q "%PROJ%\app\build" 2>nul
rd /S /Q "%PROJ%\build" 2>nul
rd /S /Q "%PROJ%\.gradle" 2>nul

echo [5/5] Done.
echo.
echo HOTOVO: BUILD2EQ installed.
echo Changed only: app\src\main\assets\emu\index.html
echo Project: %PROJ%
echo.
echo Now GitHub Desktop:
echo Summary: BUILD2EQ record serin cload clean
echo Commit to main
echo Push origin
echo ============================================================
pause
endlocal
