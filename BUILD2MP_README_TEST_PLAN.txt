AtariHelp.eu EMU-10 BUILD2MP_SEGA_CORE_PICKER_CLARITY_STAGE3

CO TO JE:
- navazuje na BUILD2MO
- NEPRIDAVA fake emulaci
- opravuje matoucí hlasku po vyberu .gen/.bin/.md/.smd v CORE pickeru
- pokud se omylem vybere Sonic ROM pres dlouhy stisk KONZOLE, appka jasne rekne: TOHLE NENI CORE
- ROM picker zustava: SBIRKA / HRY / CARTRIDGE SLOT
- core picker zustava: dlouhy stisk KONZOLE pouze pro .js adapter nebo .wasm + JS glue
- Atari/VBXE/EMU2 se nemeni

KODY JSOU ZMENENE:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/nap_sega_core_slot.js
- app/src/main/assets/emu_sega/nap_sega_core_bridge.js (jen build label/log text)
- app/src/main/assets/emu_sega/cores/README_DROP_REAL_SEGA_CORE_HERE.txt

TEST PLAN:
1) Rozbal ZIP pres koren projektu.
2) Buildni/spust APK.
3) Otevri Sega modul.
4) Klikni HRY/SBIRKA/CARTRIDGE a vyber Sonic .gen.
   Ocekavani: ROM audit OK, core chybi, zadny fake obraz.
5) Podrz KONZOLE cca 1 vterinu a OMYLEM vyber Sonic .gen.
   Ocekavani: appka jasne napise, ze to je ROM vybrana do CORE pickeru a ze ROM patri do HRY/SBIRKA/CARTRIDGE.
6) Klikni ULOZENE a posli log.

LOG MARKERY:
- BUILD2MP_SEGA_CORE_PICKER_CLARITY_STAGE3
- ROM_SELECTED_IN_CORE_PICKER
- CORE STATUS: ROM_VALIDATED_BUT_CORE_MISSING
- CORE SLOT STATUS: ROM_SELECTED_IN_CORE_PICKER

DULEZITE:
- pokud core adapter neni opravdu vlozeny jako JS/wasm+glue, hra jeste nemuze bezet
- to je zamer: zadny fake Sonic, zadne kresleni obrazku misto realne emulace
