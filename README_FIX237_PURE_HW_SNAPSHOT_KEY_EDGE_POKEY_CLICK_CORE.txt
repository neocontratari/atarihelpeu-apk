AtariHelp.eu EMU-09 FIX237_PURE_HW_SNAPSHOT_KEY_EDGE_POKEY_CLICK_CORE

Cil:
- Opravit realny problem z FIX236: jeden SNAPSHOT stahoval vice TXT souboru kvuli vrstvenym snapshot wrapperum FIX228..FIX236.
- Opravit REALOS/BASIC klavesnici: jedna klavesa nesmi drzet CH/KBCODE pres mnoho snimku a tisknout 10 znaku.
- RETURN musi byt jeden ATASCII EOL $9B, ne nekolik radku.
- Pridat kratky POKEY-style keyclick pouze pri skutecne manualni klavese; zadne fake obrazovky.

Zmeny:
1) Snapshot single-save gate:
   - vnitrni autosave volani ve starych PURE snapshot vrstvach se pri jednom SNAPSHOT kliknuti potlaci,
   - ulozi se az finalni FIX237 log,
   - soubor ma nove prefix atarihelp-FIX237.

2) REALOS keyboard edge:
   - manualni BASIC klavesa je jednorazova hrana,
   - po OS keyboard vectoru $E420/$E424 nebo raw CH read $02FC se CH/KBCODE ihned vycisti,
   - syncInputShadows uz nesmi znovu re-populovat spotrebovanou klavesu kazdy frame.

3) RETURN single EOL:
   - stejny mechanismus opravuje RETURN; po jednom precteni se $02FC/$D209 cisti.

4) POKEY-style keyclick:
   - kratky klik se spusti jen pri skutecnem manualnim REALOS input edge,
   - zadny fake READY/SELF TEST, zadny PC shortcut.

Kontrola ve snapshotu:
FIX237 INPUT/SNAPSHOT CHECK
SNAPSHOT ...
KEYEDGE ...
KEYCLICK ...
RULE SNAPSHOT_SINGLE_TXT / REALOS_KEY_ONE_SHOT_EDGE / RETURN_SINGLE_EOL / NO_DRAWN_SCREEN_FAKE

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
