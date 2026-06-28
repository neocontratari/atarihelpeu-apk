AtariHelp.eu EMU-10
BUILD2MO_SEGA_REAL_CORE_SLOT_STAGE2

CO TO JE:
- Vychazi z BUILD2MN, ktery uz René otestoval: Sonic ROM se nacetla, header audit OK, log se ulozil, core chybel spravne.
- BUILD2MO nepridava fake emulaci.
- Pridava real-core SLOT loader:
  1) automaticky hleda lokalni JS adaptery v assets/emu_sega a assets/emu_sega/cores
  2) dlouhy stisk KONZOLE otevre externi picker pro .js/.wasm core adapter
  3) bridge po nacteni adapteru provede rescan a zkusi ROM predat znovu

KODY JSOU ZMENENE:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/nap_sega_core_bridge.js
- app/src/main/assets/emu_sega/nap_sega_core_slot.js
- app/src/main/assets/emu_sega/cores/README_DROP_REAL_SEGA_CORE_HERE.txt

KODY JSOU STEJNE / NEMENENO:
- Atari/VBXE/EMU2 se nema menit.
- Hlavni menu zustava stejne jako v BUILD2MN.
- ROMy nejsou v ZIPu ani APK.

TEST PLAN:
1) Rozbal overlay pres koren projektu tak, aby se prepsalo app/.
2) Buildni/spust APK.
3) Otevri SEGA modul.
4) Cekany log po startu:
   - BUILD2MO pripraven
   - SEGA CORE SLOT zkousim lokalni adapter ...
   - pokud zatim zadny adapter neni: NO_CORE_FILE_IN_ASSETS
   - NO_REAL_CORE_ADAPTER_FOUND
5) Vyber Sonic/Aladdin .gen/.bin/.md/.smd pres HRY/SBIRKA/cartridge slot.
6) Cekany vysledek bez realneho core:
   - SEGA ROM CHECK OK
   - CORE BRIDGE: ROM_VALIDATED_BUT_CORE_MISSING
   - zadna hra se nespusti, protoze bez core by to byl fake
7) Klikni ULOZENE a posli log.
8) Volitelny test: podrz KONZOLE cca 1 vterinu.
   - otevre se picker real core .js/.wasm
   - pokud vyberes samotny .wasm, spravne napise WASM_STORED_JS_ADAPTER_REQUIRED
   - pokud vyberes JS bez kompatibilniho API, spravne napise EXTERNAL_JS_LOADED_BUT_NO_ADAPTER

CO POSLAT ZPATKY:
- screenshot monitoru
- TXT log z Downloads/AtariHelp
- napis, jestli KONZOLE dlouhy stisk otevrel picker
