AtariHelp.eu EMU-09 FIX261 UI LOCK SINGLE CASSETTE PANEL CORE

Commit summary:
FIX261 UI lock single cassette panel

Co se opravuje:
- FIX260 vratil aplikaci, ale horni nadpis prepisovaly stare zpozdene UI stamp timery z FIX248-FIX251.
- Nejhorsi byl FIX251 timer po 4000 ms: po startu nebo POWER BASIC dokazal vratit hlavicku na FIX251_TURBOBASIC...
- FIX261 tyto stare delayed UI stampy vypina, zachovava core funkce a kazeťakovy WAV pilot z FIX251.

Zmeny:
- zamceny horní nadpis na FIX261_UI_LOCK_SINGLE_CASSETTE_PANEL_CORE,
- vypnute stare delayed stamp timery FIX246/FIX247/FIX248/FIX249/FIX250/FIX251,
- FIX251 kazetakovy pilot zustava jako API/panel: INSERT BUILTIN TURBOBASICXXL.WAV, INSERT WAV/CAS, PLAY, STOP, REWIND, CLOAD,
- kazetak uz neni vypnuty recovery panel 4x; audit hlasi CASSETTE_PANEL_COUNT,
- jedna klavesnice pod obrazem, duplicity panelu FIX253-FIX258 se dal odstraňuji,
- snapshot TXT fallback zachovan jako FIX261,
- BASIC READY a snapshot zustavaji prvni priorita.

Co NENI fake:
- CLOAD neni nahrani do RAM; posila se jako realny BASIC prikaz pres klavesnici/queue.
- WAV se jen nacita/analyzuje/play pilot; zadny RAM inject.
- SELF TEST neni kresleny fake screen.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.

Test plan:
1. Rozbal ZIP overlay do lokalniho repo.
2. Commit: FIX261 UI lock single cassette panel.
3. Postav APK pres GitHub Actions.
4. Spust AtariHelp.eu -> EMU-09.
5. Sleduj 6 sekund horni nadpis: musi zustat FIX261, nesmi problikavat 248/249/250/251.
6. Over, ze je jedna klavesnice pod obrazem.
7. Over, ze je jeden kazetakovy panel s INSERT BUILTIN TURBOBASICXXL.WAV / PLAY / CLOAD, ne 4 vypnute panely.
8. Dej POWER XL/XE BASIC, napis ? FRE(0), ocekavani z FIX239 je 37902.
9. Dej SNAPSHOT a posli TXT. Hledej:
   build=FIX261_UI_LOCK_SINGLE_CASSETTE_PANEL_CORE
   OLD_STAMP_TIMERS_FIX248_251=DISABLED
   HEADER_LOCK=FIX261
   UI_CHAOS_FIX253_258_ELEMENTS=0
   KEYBOARD_COUNT=1
   CASSETTE_PANEL_COUNT=1
   CASSETTE_UI=SINGLE_FIX251_WAV_PILOT_RESTORED / no auto CLOAD / no RAM inject / no fake LOAD
10. Teprve potom test INSERT BUILTIN TURBOBASICXXL.WAV a PLAY. Pokud CLOAD zustane viset, je to realna dalsi chyba kazetove/SIO/POKEY timing vrstvy, ne fake nahravani.
