AtariHelp.eu EMU-09 FIX210_REALOS_TEXT_FAST_BASIC_CORE

KODY JSOU STEJNE:
- SUBMARINE BASIC kod je stejny jako ve FIX207/FIX208/FIX209.
- GTIA 9/10/11 BASIC kody jsou stejne.

ZMENY POD KAPOTOU:
1) BASIC TXT a VLOZIT PROGRAM uz pouzivaji rychlou CH/KBCODE/SKSTAT autopaste cestu jako SUBMARINE.
   - Zadny petiminutovy timed paste.
   - Stary pomaly paste zustava jako fallback pri chybe.

2) REAL OS textove rezimy ANTIC 6/7 / BASIC GRAPHICS 1/2/17/18 uz nejsou nucene do modro-bile READY palety.
   - GRAPHICS 17 Submarine bere skutecne COLPF/COLBK registry.
   - READY/editor ANTIC 2/3 muze zustat modro-bily, ale programove textove grafiky maji realne BASIC barvy.

3) Snapshot diagnostika:
   - FIX210 FAST BASIC
   - FIX210 REALOS TEXT COLORS
   - SUBMARINE CH AUTOPASTE FIX210

TEST PLAN:
1. Kliknout SUBMARINE FAST.
2. Porovnat s Altirrou: ocekavame cerny background / real BASIC barvy, ne modro-bily editor.
3. Vyzkouset BASIC TXT FAST a VLOZIT FAST s kratkym programem.
4. Poslat screenshot + snapshot TXT s radky FIX210 FAST BASIC a FIX210 REALOS TEXT COLORS.

POZNAMKA:
Toto neni herni hack. Je to oprava REAL OS BASIC/ANTIC textove cesty a rychleho vkladani BASIC kodu.
