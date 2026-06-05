AtariHelp.eu EMU-09 FIX181 REALOS TEST BUTTON RECOVER GTIA CORE

Navazuje na FIX180. Opravuje zaseknuti po testovacich tlacitkach: FIX180 mel v render/snapshot ceste nedefinovane fix181AnticGridInfo/auditAnticGridFix181 (puvodne fix180...), coz hazelo ReferenceError a WebView pusobil zamrzle.

Co je zmeneno:
- doplnen bezpecny ANTIC GRID diagnostic guard
- GTIA 9/10/11 test tlacitka jsou obalena try/catch
- direct GTIA test pauzne CPU, aby OS/BASIC hned nesmazal obraz a UI zustalo aktivni
- keep-alive uz neprestavuje DLIST kazdy frame, jen lehce obnovuje RAM vzor
- klavesnice zustava beze zmeny
- GRAPHICS 7 EDGE test zustava

Test:
1. Spust REAL OS BASIC.
2. GRAPHICS 7 EDGE TEST.
3. GTIA 9/10/11 TEST - tlacitka nesmi zamrznout UI.
4. RESET/READY musi vratit BASIC.
5. Snapshot/log: ANTIC GRID FIX181, GTIA DIRECT FIX181, GTIA RENDER FIX181.
