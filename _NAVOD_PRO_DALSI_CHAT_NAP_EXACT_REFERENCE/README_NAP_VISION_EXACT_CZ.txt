ATARIHELP / N&P VISION - PRESNA KOPIE NAVRHU V JAVA OVERLAY
============================================================

PRO KOHO:
- René Frank alias Renatus Honda
- Czech tester/designér, ne programátor
- chce ZIP overlay, ne ruční přepisování dlouhých návodů

CO JE V ZIPU:
1) app/src/main/assets/nap_ui/nap_intro_command_center.png
   - přesná grafická kopie vybraného úvodu N&P VISION / COMMAND CENTER

2) app/src/main/assets/nap_ui/nap_player_cassette.png
   - přesná grafická kopie vybraného přehrávače/kazeťáku

3) app/src/main/assets/nap_ui/nap_original_logo.png
   - původní N&P logo/pečeť

4) app/src/main/java/eu/atarihelp/nap/NapExactImageScreens.java
   - čistá Java třída, která obrázek kreslí na celou obrazovku
   - přes tlačítka má neviditelné hit-zóny
   - žádné XML

5) INTEGRACE_DO_MAINACTIVITY.java.txt
   - přesný napojovací příklad pro MainActivity

HLAVNI PRINCIP:
- Pro opravdu přesnou kopii AI grafického návrhu se použije PNG jako full-screen UI.
- Java nedokáže sama o sobě rychle a přesně přemalovat fotorealistický kov, světla, textury a detaily 1:1 bez toho, aby se použil obrázek.
- Proto je správná cesta: obrázek jako přesná skin vrstva + neviditelné Java klik-zóny.
- To není fake emulator. Je to jen přesný grafický skin / menu. Emulator se musí po kliknutí otevřít skutečný existující.

JAK NAPOJIT:
- Zkopírovat složku app/src/main/assets/nap_ui do projektu.
- Zkopírovat NapExactImageScreens.java do projektu.
- Pokud se package liší, upravit první řádek v NapExactImageScreens.java nebo import.
- Ve starém intro kódu NELEPIT grafiku přes starou stránku.
- Starý úvod s kazetákem se má nahradit voláním showNapIntroExact().

TLAČÍTKA V INTRO:
- EMULATOR      -> otevřít existující real Atari 130XE emulator screen
- PREHRAVAC     -> otevřít přesnou obrazovku přehrávače/kazeťáku
- ATARIHELP.EU  -> otevřít https://atarihelp.eu/ přes browser Intent
- HELP          -> otevřít help / info v aplikaci

DŮLEŽITÉ PRO ATARIHELP.EU A YOUTUBE:
- ATARIHELP.EU nesmí skákat do emulatoru.
- YOUTUBE nesmí skákat do emulatoru.
- Obě tlačítka mají používat Intent.ACTION_VIEW + CATEGORY_BROWSABLE.

STAV KÓDŮ:
KODY JSOU ZMENENE - mění se UI úvod / přehrávač / navigace.
BASIC / Altirra testovací kódy: KODY JSOU STEJNE - tento overlay se jich netýká.

TEST PLAN:
1) Spustit aplikaci.
   Očekávaný výsledek: místo starého kazeťáku se ukáže N&P VISION úvod.

2) Kliknout EMULATOR.
   Očekávaný výsledek: otevře se existující real Atari emulator, ne obrázek a ne fake obrazovka.

3) Kliknout PREHRAVAC.
   Očekávaný výsledek: otevře se přesná obrazovka přehrávače/kazeťáku.

4) Kliknout ATARIHELP.EU.
   Očekávaný výsledek: otevře se https://atarihelp.eu/ v prohlížeči / externím webu.

5) V přehrávači kliknout YOUTUBE.
   Očekávaný výsledek: otevře se https://www.youtube.com/ v prohlížeči / externím webu.

6) Pokud tlačítko netrefuje přesně:
   Upravit normalizované souřadnice Zone v NapExactImageScreens.java.
   Zapnout debug zóny pomocí setDebugZones(true), pokud je potřeba.

POZOR:
- Toto je přesná grafická kopie návrhu, protože obraz je použit jako skin.
- Další krok může být převést tento skin do reálných Android View prvků, ale nebude to 1:1 fotorealistické bez obrázků/textur.
