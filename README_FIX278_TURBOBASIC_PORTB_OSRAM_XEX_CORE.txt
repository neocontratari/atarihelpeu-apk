AtariHelp.eu EMU-09 FIX278_TURBOBASIC_PORTB_OSRAM_XEX_CORE

Commit summary:
FIX278 TurboBasic PORTB OSRAM XEX core

Co je cilem:
- Ne turbo nahravani ani CLOAD.
- TurboBasic XL dostat do emulatoru jako skutecny XEX program.
- XEX segment loader ma zkopirovat segmenty a spustit INIT/RUN.
- Emulator musi respektovat realne PORTB mapovani: kdyz program vypne OS ROM, CPU cte/zapisuje RAM pod $C000/$D800.

Zmeny:
- zachovan turbo-basic-xl-1.5-copy.xex jako asset; prekopirovan z dodaneho souboru.
- pridan panel TURBO BASIC XL do EMU-09 FIX278.
- pridana funkce fix278StartTurboBasic().
- doplnen PORTB MMU patch: OS ROM active jen pri PORTB bit0=1, BASIC ROM jen pri bit1=0.
- TurboBasic XEX se spousti pres realny XEX segment loader, ne pres CLOAD/WAV a ne pres fake READY.
- snapshot FIX278 vypise segmenty, entry, INIT/RUN, PC pred/po startu, kroky CPU, PORTB pred/po.

Zadne fake:
- zadny fake READY
- zadny fake LOAD
- zadny RAM BASIC program inject
- zadny screen RAM write jako dukaz
- zadny fake kazetovy zvuk

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou menene.
