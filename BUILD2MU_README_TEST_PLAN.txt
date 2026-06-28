AtariHelp.eu EMU-10 BUILD2MU_SEGA_LRUSSO_CLOSURE_EVAL_BOOT_STAGE8

Ucel:
- Oprava slepeho iframe fallbacku z BUILD2MT.
- Lrusso Genesis.htm ma ochranu window.top === window.self, proto v iframe uvnitr naseho monitoru zustane jen tocici se kolecko / nejde kliknout.
- BUILD2MU misto iframe zkousi realnou cestu: stahnout Genesis.min.js jako text, odstranit jen currentScript origin guard a spustit engine v closure-eval rezimu.
- Cilem je ziskat realny embedGenesis() a poslat do nej tvoji lokalni Sonic/Aladdin ROM jako ArrayBuffer.

Co je a neni fake:
- Neni tu zadna namalovana hra ani fake Sonic.
- Pokud se ukaze grafika hry, je to vystup realneho lrusso/PicoDrive core.
- Pokud core selze, monitor ukaze chybu a log rekne presne kde.

KODY JSOU ZMENENE:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/nap_sega_lr_genesis_adapter.js
- app/src/main/assets/emu_sega/nap_sega_core_bridge.js
- app/src/main/assets/emu_sega/nap_sega_core_slot.js
- app/src/main/assets/emu_sega/cores/README_DROP_REAL_SEGA_CORE_HERE.txt
- app/src/main/assets/index.html

TEST PLAN:
1) Rozbal ZIP pres koren projektu.
2) Spust build.
3) Otevri Sega modul.
4) Pres HRY / SBIRKA / CARTRIDGE vyber Sonic .gen nebo Aladdin .gen.
5) Ocekavani A: pokud closure-eval funguje, monitor se prepne do realneho Genesis vystupu.
6) Ocekavani B: pokud selze, uz se nebude zasekavat iframe kolecko; ukaze se presna chyba.
7) Zkus START / A / B / C / D-PAD.
8) Klikni ULOZENE a posli log + screenshot.

HLAVNI LOG MARKERY:
- ENGINE SOURCE SCAN online:lrusso/Genesis.min.js ... hits=...
- PATCH currentScript guard removed bytes=...
- CLOSURE EVAL OK embedGenesis exported...
- embedGenesis CALLBACK STARTED
- nebo CLOSURE EVAL FAILED ...
