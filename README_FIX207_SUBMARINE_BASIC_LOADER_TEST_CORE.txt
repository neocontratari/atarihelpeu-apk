# AtariHelp.eu EMU-09 FIX207 SUBMARINE BASIC LOADER TEST CORE

KODY JSOU ZMENENE pro SUBMARINE BASIC TEST.
GTIA 9/10/11 VERIFY kody zustavaji stejne jako predchozi build.

Co se zmenilo:
- pridano tlacitko SUBMARINE BASIC TEST do mobilni Atari klavesnice,
- do assets pridan SUBMARINE_BASIC_FIX207.txt,
- dodany House Book BASIC kod je opraveny pro cisty Atari BASIC:
  - radek 195: PAUSE 150 nahrazeno GOSUB 900,
  - radek 1050: PAUSE 2 nahrazeno GOSUB 920,
  - delay smycky jsou na radcich 900 a 920,
  - RND hodnoty pro POKE/SOUND jsou obalene INT(), aby nevznikaly float/rozsahove nejasnosti,
- snapshot pridava SUBMARINE BASIC TEST FIX207 a SUBMARINE BASIC CODE FIX207,
- REAL ROM stream loader z FIX206 zustava pod kapotou.

Test plan:
1. Rozbal overlay do lokalniho repo a commit/push pres GitHub Desktop.
2. Postav APK v GitHub Actions.
3. V Noxu/mobilu spust EMU-09.
4. V mobilni Atari klavesnici stiskni SUBMARINE BASIC TEST.
5. Pockej, az se cely BASIC program dopise a spusti.
6. Porovnej s Altirrou: intro, text, zvuk, klavesa, start hry.
7. Posli screenshot + snapshot TXT s radky SUBMARINE BASIC TEST FIX207 a SUBMARINE BASIC CODE FIX207.

Commit summary:
FIX207 submarine basic loader test core
