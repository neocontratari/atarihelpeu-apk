FIX125 ADAPTIVE ROM DLIST CORE

Co se mění:
- Opraveno, že v APK assetu zůstával starý FIX123 script. Teď je i app/src/main/assets/emu09_pmg_gtia_overlay.html skutečně FIX125.
- FIX123 směr není zahozený: přidán AUTO core selector.
- Donkey/Cobra/Pitstop/River/Galaxian/PiTT/KiTT: LEGACY_PROTECTED, bez ROM probe zásahu.
- Arkanoid/Moon Patrol/Pac-Man/generic: ROM_PROBE, ponechává výhody FIX123 tam, kde může pomoct.
- Montezuma: MONTEZUMA_SAFE_LEGACY, protože ROM probe jí podle testů rozbíjel grafiku/přechody.
- Snapshot log vypisuje core mode, aby šlo ověřit, co si hra zvolila.

Test:
1) V aplikaci musí být vidět build FIX125_ADAPTIVE_ROM_DLIST_CORE.
2) Donkey/Cobra: rychle ověřit, že se vrátil základ.
3) Arkanoid/Moon/Pac: zkusit ROM_PROBE cestu a poslat screenshot + snapshot log.
4) Montezuma PRELIM/ALT/ORIG: ověřit, že není horší než před ROM experimentem.
