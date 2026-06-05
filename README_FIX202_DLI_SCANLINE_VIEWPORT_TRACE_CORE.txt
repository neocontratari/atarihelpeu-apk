ATARIHELP.EU EMU-09 - FIX202_DLI_SCANLINE_VIEWPORT_TRACE_CORE
================================================================

Commit summary:
FIX202 dli scanline viewport trace core

Co je ve FIX202:
- Navazuje primo na FIX201_DLI_WSYNC_CAPTURE_CORE.
- Zadny Donkey-only ani Cobra-only hard-lock.
- CAS/WAV pilot zustava vypnuty, hry jdou pres cisty XEX loader.
- DLI/WSYNC capture ted uklada stav podle realneho ANTIC scanline a zaroven podle aktualniho viewport posunu rendereru.
- Renderer tedy nepouziva DLI barvy/CHBASE/PMG stav posunuty o spatny radek, coz mohlo delat sum, duchy a rozpad barev.
- DLI capture bere i PRIOR $D01B, nejen COLPF/COLBK. To je dulezite pro GTIA/PMG priority a duchy.
- Snapshot log ma novou stopu:
  - DLI WSYNC CAPTURE FIX202
  - DLI VIEWPORT TRACE FIX202
  - DLI CAP FIX202

Test plan:
1. TEST SUPER COBRA XEX
   - menu, zacatek hry, jestli je sum mensi/vetsi/stejny
   - screenshot menu + hra
   - snapshot TXT log
2. TEST DONKEY KONG XEX
   - intro prechody + hra
   - jestli jsou duchy/sum mensi/vetsi/stejne
   - screenshot + snapshot TXT log
3. G7 TUNNEL 96 TEST smoke
4. ZVUK TEST smoke

Dulezite:
- Pokud bude obraz horsi, neposuzovat podle dojmu samostatne: poslat snapshot, hlavne radky DLI VIEWPORT TRACE FIX202 a DLIST RESOLVE FIX201/FIX202.
- Cilem je obecne jadro ANTIC/DLI/GTIA/PMG timing, ne profilovy hack jedne hry.
