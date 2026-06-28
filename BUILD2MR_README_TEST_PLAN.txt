AtariHelp.eu EMU-10 - BUILD2MR_SEGA_LRUSSO_GENESIS_ONLINE_CORE_PROBE_STAGE5

CIL:
- Posun od pouheho core-missing stavu k prvnimu realnemu core probe.
- Pridany NAP adapter pro lrusso Genesis/PicoDrive browser core API.
- Adapter zkousi lokalni engine v assets/emu_sega/cores/Genesis.min.js nebo Genesis.js.
- Kdyz lokalni engine chybi, zkusi online probe z raw GitHub Genesis.min.js, patchne pouze cross-origin guard v pameti a zavola embedGenesis.
- Zadne ROM v ZIPu, zadny fake Sonic, zadny screen paint.

DULEZITE:
- Pokud Android WebView/Nox nepovoli fetch na GitHub, zustane stav CORE ENGINE LOAD FAILED.
- To neni chyba ROM. Znamena to jen, ze pro dalsi offline build musime mit realny core soubor lokalne v assets/emu_sega/cores/.
- Finalni produkcni cesta ma byt lokalni core + license/source notice, ne zavislost na internetu.

TEST:
1. Rozbal ZIP pres koren projektu.
2. Spust build v Noxu/mobilu.
3. Otevri Sega modul.
4. Pres HRY/SBIRKA/CARTRIDGE vyber Sonic .gen.
5. Cekany pokrok proti BUILD2MQ:
   - core status by mel najit NAP_SEGA_REAL_CORE adapter,
   - po vyberu ROM se zkusi lokalni cores/Genesis.min.js / Genesis.js,
   - kdyz chybi, zkusi online lrusso Genesis probe,
   - pri uspechu se v monitoru ma objevit realny emulatorovy vystup, ne nase fake kresba.
6. Zkus START / A / B / C / D-PAD.
7. Klikni ULOZENE a posli log.

LOG MARKERY:
- SEGA LRUSSO ADAPTER BUILD2MR: adapter init OK
- REAL_CORE_ADAPTER_READY:NAP_SEGA_REAL_CORE
- loadRom prijato title=...
- zkousim lokalni lrusso Genesis engine cores/Genesis.min.js
- lokalni engine chybi, zkousim ONLINE probe lrusso Genesis.min.js
- CORE ENGINE READY ...
- embedGenesis CALLBACK STARTED - real core reported start

KDYZ SELZE:
- CORE ENGINE LOAD FAILED / FETCH_NOT_AVAILABLE / ONLINE_FETCH_HTTP / SCRIPT_LOAD_FAILED = musime vlozit lokalni core soubor do assets/emu_sega/cores/ a navazat offline build.
