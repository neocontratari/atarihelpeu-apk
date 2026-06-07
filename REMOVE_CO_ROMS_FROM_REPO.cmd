@echo off
setlocal
cd /d "%~dp0"
echo AtariHelp.eu EMU-09 FIX267 ROM quarantine cleaner
echo Maze jen duplicitni CO ROM assety. Zadny kod neupravuje.

del /f /q "app\src\main\assets\co24947a.rom" 2>nul
del /f /q "app\src\main\assets\co60302a.rom" 2>nul
del /f /q "app\src\main\assets\co61598b.rom" 2>nul
del /f /q "app\src\main\assets\rom_audit\co24947a.rom" 2>nul
del /f /q "app\src\main\assets\rom_audit\co60302a.rom" 2>nul
del /f /q "app\src\main\assets\rom_audit\co61598b.rom" 2>nul
rmdir "app\src\main\assets\rom_audit" 2>nul

echo HOTOVO. Zkontroluj v GitHub Desktop, ze jsou CO ROM soubory smazane.
pause
