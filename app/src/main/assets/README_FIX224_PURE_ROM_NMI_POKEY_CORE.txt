AtariHelp.eu EMU-09 FIX224_PURE_ROM_NMI_POKEY_CORE

KODY JSOU STEJNE
- SUBMARINE BASIC kod beze zmeny.
- GTIA 9/10/11 VERIFY kody beze zmeny.
- Ostatni BASIC testy beze zmeny.

Cil FIX224:
- Zadna fake READY obrazovka.
- Zadna fake SELF TEST obrazovka.
- PURE ROM BASIC / SELF TEST ma jit pres ROM reset vektor a ROM cteni.
- POKEY zvuk uz nesmi byt v REALOS/BASIC umlceny.
- VBI/DLI pro PURE ROM se nepousti pres stary async JSR wrapper, ale pres pokus o realne NMI/RTI:
  CPU push PC/P -> NMI vector $FFFA/$FFFB -> ROM handler -> RTI.

Zmeny:
1) FIX224 TRUE NMI
- pridana realnejsi NMI cesta pro PURE ROM rezim.
- VBI nastavi NMIST $D40F bit $40 a jde pres ROM NMI vector.
- DLI nastavi NMIST $D40F bit $80 a jde pres ROM NMI vector jen kdyz DLIST obsahuje realne DLI bity.
- zadny save/restore CPU jako fake JSR wrapper; RTI se musi vratit normalne pres stack.

2) FIX224 POKEY REALOS
- POKEY zapisy $D200-$D208 uz aktualizuji audio i v activeXexProfile realos.
- updateAudioFromPokey uz REALOS/BASIC neumlcuje.

3) FIX224 INPUT HARD RESET
- PURE ROM BASIC BOOT a PURE ROM SELF TEST rusi fronty a casovace jako drive, ale stav je explicitne logovany jako NO FAKE.

Dulezite log radky:
- BUILD TAG FIX224_PURE_ROM_NMI_POKEY_CORE
- FIX224 TRUE NMI
- FIX224 POKEY REALOS
- FIX224 NO FAKE
- FIX224 ROM SET AUDIT
- FIX224 BRK IRQ VECTOR
- DLIST / SAVMSC / NMIEN / VBI / DLI / PC

Test plan:
1. Spust PURE ROM BASIC BOOT.
   - cekej READY bez resetu.
   - rucne napis PRINT FRE(0), LIST, NEW.
   - snapshot.

2. Spust jednoduchy zvukovy BASIC test:
   10 SOUND 0,100,10,8
   20 FOR I=1 TO 200:NEXT I
   30 SOUND 0,0,0,0
   RUN
   - poslouchej, jestli POKEY zvuk v BASICu konecne zazni.
   - snapshot.

3. Spust PURE ROM SELF TEST.
   - nic dalsiho nema byt predem vlozene.
   - screenshot + snapshot, i kdyz menu jeste nebude.

Poznamka:
FIX224 netvrdi, ze SELF TEST uz musi byt hotovy. Tvrdí jen to, co je v kodu opravdu zmenene: realos POKEY uz neni umlceny a PURE ROM interrupty jdou pres NMI vector/RTI, ne pres fake async JSR wrapper.
