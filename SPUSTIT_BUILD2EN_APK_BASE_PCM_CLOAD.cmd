@echo off
chcp 65001 >nul
echo ============================================================
echo AtariHelp.eu EMU-10 BUILD2EN - APK BASE + PCM CLOAD FIX
echo ============================================================
echo.
echo Tento skript se musi spustit v KORENI projektu:
echo C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
echo.
if not exist "app\src\main\assets\emu\index.html" (
  echo CHYBA: Nejsi v koreni projektu. Nevidim app\src\main\assets\emu\index.html
  echo Otevri GitHub Desktop ^> Repository ^> Show in Explorer a spust to tam.
  pause
  exit /b 1
)
if not exist "_EMU10_BUILD2EN_PAYLOAD\app\src\main\assets\emu\index.html" (
  echo CHYBA: Vedle CMD chybi slozka _EMU10_BUILD2EN_PAYLOAD.
  echo Zkopiruj z ZIPu do korene projektu CMD i slozku _EMU10_BUILD2EN_PAYLOAD.
  pause
  exit /b 1
)
echo [1/5] Zaloha aktualniho index.html...
copy /Y "app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html.BACKUP_BEFORE_BUILD2EN_%RANDOM%" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se vytvorit zalohu index.html
  pause
  exit /b 1
)
echo [2/5] Tvrdě nahrazuji pouze app\src\main\assets\emu\index.html...
copy /Y "_EMU10_BUILD2EN_PAYLOAD\app\src\main\assets\emu\index.html" "app\src\main\assets\emu\index.html" >nul
if errorlevel 1 (
  echo CHYBA: Nepodarilo se nahradit index.html
  pause
  exit /b 1
)
echo [3/5] Mazani rozbitych pokusnych installeru EK/EM, aby se uz nespletly...
rd /S /Q "_EMU10_BUILD2EK_PAYLOAD" 2>nul
rd /S /Q "_EMU10_BUILD2EM_PAYLOAD" 2>nul
rd /S /Q "_EMU10_BUILD2EL_PAYLOAD" 2>nul
del /Q "SPUSTIT_BUILD2EK*.cmd" 2>nul
del /Q "SPUSTIT_BUILD2EM*.cmd" 2>nul
del /Q "SPUSTIT_BUILD2EL*.cmd" 2>nul
echo [4/5] Mazani build cache, aby WWW/GitHub Actions vzalo novy soubor...
rd /S /Q "app\build" 2>nul
rd /S /Q "build" 2>nul
rd /S /Q ".gradle" 2>nul
echo [5/5] Kontrola hotova.
echo.
echo HOTOVO: BUILD2EN nainstalovan.
echo Zmenen byl jen: app\src\main\assets\emu\index.html
echo Zdroj zakladu: index.html vytazeny z tebou poslane posledni funkcni APK.
echo CLOAD zmena: Altirra-OK WAV jde jako PCM hrany na SKSTAT bit4, ne pres decode-SERIN.
echo.
echo Ted otevri GitHub Desktop: Summary = BUILD2EN apk base pcm cload
echo Pak Commit to main a Push origin.
echo ============================================================
pause
