AtariHelp.eu EMU-09 FIX179 REALOS GTIA DIRECT FILL RENDER CORE

Navazuje na FIX178.

Co se meni:
- Klavesnice zustava funkcne jako FIX175/FIX176/FIX177/FIX178. Do klavesnice uz nesahame.
- GTIA 9/10/11 test tlacitka uz nepouzivaji pomale BASIC POKE smycky.
- Tlacitka GTIA 9 TEST / GTIA 10 TEST / GTIA 11 TEST primo nastavi GTIA PRIOR, barvy, DLIST a SAVMSC a okamzite naplni obrazovou RAM vzorem.
- Pridan keep-alive pro direct test: kdyz OS/BASIC po GRAPHICS rezimu smaze screen RAM, testovy vzor se znovu doplni, aby neproblikl a nezmizel.
- Diagnostika: GTIA DIRECT FIX179 + GTIA RENDER FIX179 ukazuji DLIST, SAVMSC, PRIOR, pocet nenulovych bajtu a sample.
- GRAPHICS 7 zustava referencni funkcni mod.
- Zadny herni hack.

Test plan:
1. BASIC jen rychle: LET A=5, ? "AHOJ", PRINT 2>1.
2. GRAPHICS 7 jako reference.
3. GTIA 9 TEST - mel by zustat viditelny vzor, ne jen problik.
4. GTIA 10 TEST.
5. GTIA 11 TEST.
6. Poslat screenshoty + snapshot/log. Hledat GTIA DIRECT FIX179 a GTIA RENDER FIX179.

Commit summary:
FIX179 realos gtia direct fill render core
