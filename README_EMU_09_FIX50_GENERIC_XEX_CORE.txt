EMU-09 FIX50 GENERIC XEX CORE

Cíl: emulator má být obecný pro vlastní XEX soubory, ne jen vestavěné testy.

Změny proti FIX49:
- build tag všude sjednocen na FIX50_GENERIC_XEX_CORE
- Pitstop snapshot se použije jen při stejné známé fázi; neznámé přechody a menu se kreslí LIVE
- generic DLI cache nyní ukládá nejen GTIA barvy, ale i ANTIC CHBASE/CHACTL/DMACTL/SDMCTL po řádcích
- renderer používá CHBASE podle DLI stavu daného řádku, což pomáhá hrám s přepínaným fontem/charsetem
- default CHBASE nastaven na mini OS font $E000, než si vlastní XEX nastaví hardware sám
- kompatibilitní log vypisuje DLIST bytes a přehled DLI stavů
- app/build.gradle versionName opraveno na EMU-09-FIX50-generic-xex-core

Testovací směr pro René:
1) načíst vlastní XEX z telefonu
2) nechat autostart
3) pokud se obraz rozpadne nebo hra stojí, dát KOMPATIBILITA / SNAPSHOT
4) poslat uložený log z Download/AtariHelpLogs

Poznámka: FIX50 není slib plného Atari 800/130XE emulátoru. Je to další krok k obecnému XEX jádru; profily River/Pitstop/PiTT zůstávají jen jako testovací ochrany.
