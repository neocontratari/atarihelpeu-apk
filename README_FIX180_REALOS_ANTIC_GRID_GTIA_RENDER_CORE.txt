AtariHelp.eu EMU-09 FIX180 REALOS ANTIC GRID GTIA RENDER CORE

Navazuje na FIX179.

Zmeny:
- klavesnice ponechana funkcne jako FIX175/FIX176/FIX179
- overena a doplnena ANTIC grid diagnostika pro BASIC GRAPHICS 7 / ANTIC D, ANTIC E a ANTIC F / GTIA 9-11
- pomaly fallback uz nepocita scanAddr jako textovy GRAPHICS 0 * 40 natvrdo, ale bere lineStrideBytes(mode)
- pridano tlacitko GRAPHICS 7 EDGE TEST pro hranice 159x95
- snapshot vypisuje ANTIC GRID FIX180
- GTIA 9/10/11 direct fill testy z FIX179 zustavaji

Test:
1. REAL OS BASIC
2. GRAPHICS 7 EDGE TEST - ramecek musi drzet okraje 0..159 / 0..95 bez kolapsu
3. GRAPHICS 7 rucni test podle Reneho
4. GTIA 9/10/11 TEST
5. snapshot/log - radky ANTIC GRID FIX180 a GTIA RENDER FIX180
