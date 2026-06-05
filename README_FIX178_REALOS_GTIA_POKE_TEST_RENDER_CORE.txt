AtariHelp.eu EMU-09 FIX178 REALOS GTIA POKE TEST RENDER CORE

Navazuje na FIX177.

Co se meni:
- Klavesnice zustava funkcne jako FIX175/FIX176/FIX177. Nehrabu znovu do CH kodu.
- GTIA 9/10/11 test tlacitka uz nepouzivaji DRAWTO. Vlozi BASIC program, ktery po GRAPHICS 9/10/11 naplni obrazovou RAM pres POKE na SAVMSC.
- Na konec test programu se automaticky posila RUN.
- BASIC TXT/paste je zrychleny opatrne: kratky znakovy puls, ale delsi pauza po RETURN, aby se radky nemichaly.
- Renderer ma lepsi GTIA diagnostiku: GTIA RENDER FIX178 ukaze mode, adresu, pocet nenulovych bajtu a sample prvnich bajtu radku.
- GRAPHICS 7 zustava referencni funkcni mod.
- Zadny herni hack.

Test plan:
1. BASIC: LET A=5, ? "AHOJ", PRINT 2>1.
2. GRAPHICS 7 jen kontrolne.
3. GTIA 9 TEST, pockat az se dopise a automaticky spusti RUN.
4. GTIA 10 TEST.
5. GTIA 11 TEST.
6. Poslat screenshoty + snapshot/log. Hledat radky GTIA MODE FIX178 a GTIA RENDER FIX178.

Commit summary:
FIX178 realos gtia poke test render core
