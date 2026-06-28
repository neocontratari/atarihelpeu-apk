AtariHelp.eu EMU-10
BUILD2MQ_SEGA_ADAPTER_FACTORY_PROBE_STAGE4

Ucel buildu:
- Posun od pouheho core pickeru k realnemu adapter factory probe.
- Zadna fake emulace. Pokud neni skutecny Mega Drive core adapter, hra se nespusti.
- Bridge ted umi zkusit vice realnych tvaru adapteru: objekt, factory, constructor/class, default/core/instance.
- Pri nalezeni symbolu bez spravneho API zaloguje API mismatch misto mlceni.

Postup testu bez realneho core:
1) Rozbal ZIP overlay do korene projektu pres app/.
2) Buildni/spust APK.
3) Otevri SEGA Mega Drive modul.
4) Pres HRY / SBIRKA / CARTRIDGE SLOT vyber Sonic .gen.
5) Cekany vysledek:
   - ROM HEADER OK
   - Title SONIC THE HEDGEHOG
   - CRC32 F9394E97 pro testovanou Sonic ROM
   - CORE BRIDGE: ROM_VALIDATED_BUT_CORE_MISSING
   - zadny fake obraz
6) Klikni ULOZENE a posli AtariHelp_SEGA_LOG_*.txt.

Servisni test omylu:
1) Podrz KONZOLE cca 1 vterinu.
2) Vyber schvalne Sonic .gen.
3) Cekany vysledek:
   - ROM_SELECTED_IN_CORE_PICKER
   - vysvetleni, ze ROM patri pod HRY/SBIRKA/CARTRIDGE.

Test s realnym core adapterem, az bude dostupny:
1) Vloz realny JS adapter do app/src/main/assets/emu_sega/cores/nap_sega_real_core.js
   nebo ho vyber dlouhym stiskem KONZOLE.
2) Adapter musi vytvorit jeden z globalnich symbolu:
   NAP_SEGA_REAL_CORE, GenesisPlusGX, GPGX, JSMooGenesis, MegaDriveCore, MDCore...
3) Adapter musi mit loadRom/loadROM/insertCartridge/loadGame.
4) Ocekavany posun v logu:
   REAL_CORE_ADAPTER_READY
   ROM_SENT_TO_REAL_CORE
   INPUT_DOWN_SENT / INPUT_UP_SENT pri tlacitkach.

Co poslat zpet:
- screenshot Sega obrazovky po ROM loadu
- ULOZENE log z Downloads/AtariHelp
- pokud se objevi API_MISMATCH, poslat presny log, nehadat
