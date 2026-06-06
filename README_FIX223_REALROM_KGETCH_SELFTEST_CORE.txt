AtariHelp.eu EMU-09 FIX223_REALROM_KGETCH_SELFTEST_CORE

KODY JSOU STEJNE. SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nebyly zmeneny.

Zmeny:
- FAST BASIC ma epoch/generation guard: stare cekajici setTimeout fronty se po PURE BOOT/RESET uz nesmi pozdeji spustit.
- BASIC TXT FAST / VLOZIT FAST / SUBMARINE FAST posilaji zdroj pres KGETCH/E420 jako ATASCII, ne pres smiseny CH/KBCODE preklad.
- BYE SELFTEST tlacitko uz nevklada text BYE jako paste frontu, ale spousti primo PURE ROM SELF TEST cestu bez fake obrazovky.
- PURE ROM SELF TEST drzi OPTION + kratky START pulse, protoze ROM kod u $5059 ceka na START low.
- PURE SELFTEST uz nevola VBI/DLI jako asynchronni JSR wrapper, ktery rozbijel low RAM/BRK.

Test plan:
1) PURE ROM BASIC BOOT: over READY, PRINT FRE(0), LIST, NEW.
2) SUBMARINE FAST: sleduj, jestli prvni radek a znaky $ # ; " ( ) jsou ciste.
3) BYE SELFTEST a PURE ROM SELF TEST: screenshot + snapshot.
