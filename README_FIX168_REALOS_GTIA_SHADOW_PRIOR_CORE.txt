AtariHelp.eu EMU-09
FIX168_REALOS_GTIA_SHADOW_PRIOR_CORE

Commit summary:
FIX168 realos gtia shadow prior core

Co je nove:
- FIX167 REAL OS/BASIC obraz je potvrzeny stabilni bod.
- Klavesnice, uvozovky a GRAPHICS 3 zustavaji beze zmeny.
- GTIA shadow registry COLOR0-4 / PCOLR0-3 / PRIOR se synchronizuji do HW COLPF/COLBK / PCOLR / PRIOR pred DLI.
- DLI barevne snimky doplnuji shadow barvy jen kdyz HW registr neni realne zapsany. Zapsana HW nula zustava platna.
- River a PiTT specialni renderer zustavaji chranene.

Test plan:
1. REAL OS BASIC AUTO BOOT - READY obraz musi zustat jako ve FIX167.
2. PRINT 2+2, uvozovky, RUN - kontrola ze klavesnice zustala hotova.
3. GRAPHICS 3 - kontrola ze cerna graficka plocha a modry textovy pruh zustaly spravne.
4. Snapshot/log - hledej radek GTIA SYNC FIX168 a REAL OS GTIA FIX168.
5. Volitelne rychly smoke Donkey/Cobra/Montezuma - jen overit ze se baseline nerozbil.
