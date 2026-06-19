@echo off
setlocal EnableExtensions

title AtariHelp EMU10 BUILD2EN2 fixed installer
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EN2 - FIXED CMD INSTALLER CRLF
echo ============================================================
echo.

rem This CMD is intentionally ASCII only and CRLF line-ended.
rem It can be started from the project root OR from the extracted installer folder.

set "PROJ="
set "PAYLOAD="

if exist "%CD%\app\src\main\assets\emu\index.html" set "PROJ=%CD%"
if not defined PROJ if exist "%USERPROFILE%\Documents\GitHub\atarihelpeu-apk\app\src\main\assets\emu\index.html" set "PROJ=%USERPROFILE%\Documents\GitHub\atarihelpeu-apk"
if not defined PROJ if exist "C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\app\src\main\assets\emu\index.html" set "PROJ=C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk"

if exist "%~dp0_EMU10_BUILD2EN2_PAYLOAD\app\src\main\assets\emu\index.html" set "PAYLOAD=%~dp0_EMU10_BUILD2EN2_PAYLOAD\app\src\main\assets\emu\index.html"
if not defined PAYLOAD if exist "%CD%\_EMU10_BUILD2EN2_PAYLOAD\app\src\main\assets\emu\index.html" set "PAYLOAD=%CD%\_EMU10_BUILD2EN2_PAYLOAD\app\src\main\assets\emu\index.html"

echo Project detected: %PROJ%
echo Payload detected: %PAYLOAD%
echo.

if not defined PROJ (
  echo ERROR: Project root not found.
  echo Open GitHub Desktop - Repository - Show in Explorer.
  echo Then copy this CMD and _EMU10_BUILD2EN2_PAYLOAD into that folder and run it there.
  pause
  exit /b 1
)

if not defined PAYLOAD (
  echo ERROR: Payload folder not found next to this CMD.
  echo Copy BOTH items from the ZIP into the project root:
  echo   SPUSTIT_BUILD2EN2_FIXED_CMD.cmd
  echo   _EMU10_BUILD2EN2_PAYLOAD
  pause
  exit /b 1
)

echo [1/5] Backup current index.html...
copy /Y "%PROJ%\app\src\main\assets\emu\index.html" "%PROJ%\app\src\main\assets\emu\index.html.BACKUP_BEFORE_BUILD2EN2_%RANDOM%" >nul
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

echo [3/5] Remove broken experimental EK/EM payloads if present...
rd /S /Q "%PROJ%\_EMU10_BUILD2EK_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EM_PAYLOAD" 2>nul
rd /S /Q "%PROJ%\_EMU10_BUILD2EL_PAYLOAD" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EK*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EM*.cmd" 2>nul
del /Q "%PROJ%\SPUSTIT_BUILD2EL*.cmd" 2>nul

echo [4/5] Clear build cache only...
rd /S /Q "%PROJ%\app\build" 2>nul
rd /S /Q "%PROJ%\build" 2>nul
rd /S /Q "%PROJ%\.gradle" 2>nul

echo [5/5] Done.
echo.
echo HOTOVO: BUILD2EN2 installed.
echo Changed only: app\src\main\assets\emu\index.html
echo Project: %PROJ%
echo.
echo Now GitHub Desktop:
echo Summary: BUILD2EN2 fixed cmd apk base pcm cload
echo Commit to main
echo Push origin
echo ============================================================
pause
endlocal
