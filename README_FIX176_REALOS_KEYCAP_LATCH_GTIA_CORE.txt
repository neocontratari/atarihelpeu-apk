# AtariHelp.eu EMU-09 FIX176 REALOS KEYCAP LATCH GTIA CORE

Overlay pro GitHub Desktop. Navazuje na potvrzeny FIX175.

Commit summary:
FIX176 realos keycap latch gtia core

Co je ve FIX176:
- Build tag: `FIX176_REALOS_KEYCAP_LATCH_GTIA_CORE`.
- CH/kodove mapovani klaves z FIX175 zustava beze zmeny: Rene potvrdil, ze `=` a `?` uz funkcne jedou.
- Oprava je graficka: mobilni klavesnice prohazuje keycap popisky `=` a `>` bez zasahu do kodu.
- SHIFT je latch/toggle: klepnout = zustane zamacknuty, dalsi klepnuti = vypnout.
- CTRL je latch/toggle: doplneno samostatne tlacitko CTRL; CTRL sipky zustavaji.
- BASIC TXT / VLOZIT PROGRAM zustava zpomalene a v poradi z FIX175.
- LIST / NEW / teply RESET zustava z FIX173/FIX175.
- GTIA 9/10/11 kod zustava, ale GRAPHICS 9/10/11 jsou stale k dalsimu renderer testu, zatim potvrzeny je hlavne GRAPHICS 7.
- Bez hernich hacku; XEX baseline zustava chraneny.

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Zkontrolovat mobilni klavesnici: graficke popisky `=` a `>` jsou prohozene podle FIX175 testu.
3. Zkus `LET A=5`, `? "AHOJ"`, `PRINT 2>1`.
4. Zkus SHIFT: klepnout SHIFT, napsat symbol, SHIFT zustane ON; dalsim SHIFT vypnout.
5. Zkus CTRL jako samostatny latch a CTRL sipky.
6. BASIC TXT / VLOZIT PROGRAM kratky program.
7. Az klavesnice sedi, vratit se k GRAPHICS 9/10/11 rendereru.
