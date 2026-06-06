AtariHelp.eu EMU-09 FIX222_REALROM_INPUT_SELFTEST_CORE

KODY JSOU STEJNE.
- SUBMARINE BASIC kod beze zmeny.
- GTIA 9/10/11 VERIFY kody beze zmeny.
- Ostatni BASIC testy beze zmeny.

Co se meni:
- FAST BASIC / SUBMARINE / VLOZIT FAST uz nepousti CH/KBCODE frontu naslepo.
- Fronta ceka na skutecny BASIC READY stav: DLIST $9C20 + SAVMSC $9C40 + VBI.
- PURE ROM BASIC BOOT a PURE ROM SELF TEST pri startu cisti vsechny stare autopaste/keyboard fronty.
- PURE ROM SELF TEST uz nedostane zbytky SUBMARINE/BASIC TXT fronty z predchoziho testu.
- PURE SELFTEST DLI se docasne nespousti jako genericky async JSR; DLIST se stale vykresluje z ROM/RAM, ale DLI rutina se nebude volat neprirozenym JSR wrapperem, ktery v FIX221 delal low-RAM BRK.
- Pridana diagnostika:
  FIX222 READY QUEUE
  FIX222 INPUT QUARANTINE
  FIX222 SELFTEST DLI

Test plan:
1) PURE ROM BASIC BOOT: overit, ze READY nabehne bez nutnosti dalsiho RESET.
2) Zadat rucne PRINT FRE(0) nebo LIST. Nesmí to spadnout do kraviny/sekani.
3) SUBMARINE FAST: musi se spustit az po READY, nesmi ztratit prvni znaky radku 0.
4) PURE ROM SELF TEST: testovat samostatne po predchozim BASIC testu; fronty musi byt vycistene. Screenshot + snapshot.

Commit summary:
FIX222 realrom input selftest core
