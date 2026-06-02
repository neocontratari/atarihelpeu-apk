AtariHelp.eu EMU-09 FIX81 STABLE CORE + VIRTUAL JOYSTICK

DULEZITE:
- FIX81 vychazi z uzivatelem oznacene funkcni verze FIX77.
- Neobsahuje riskantni jadrove zmeny z FIX78/FIX79, ktere rozbily Cobru/Donkey.
- Pridava jen izolovany kruhovy virtualni joystick v UI.
- Joystick nepouziva zadny interval/timer a nemeni CPU/ANTIC/GTIA/DLI/VBI jadro.
- Stary kriz zustal schovany jako nouzova volba.

TESTOVAT PRED DALSIMI OPRAVAMI:
1) Super Cobra: roluje krajina, je videt vrtulnik.
2) Donkey Kong: intro a prechod do hry aspon jako ve FIX77.
3) Ovládani: joystick nesmi zpomalovat hry.
4) Az potom Galaxian a dalsi XEX.

Další vývoj: novou podporu her pridavat po malych krocich, vzdy z teto stabilni vetve a bez rozbijeni referenci Cobra/Donkey/PiTT-KiTT.
