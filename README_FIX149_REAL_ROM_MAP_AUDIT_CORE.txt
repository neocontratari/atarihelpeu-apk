# AtariHelp.eu EMU-09 – FIX149_REAL_ROM_MAP_AUDIT_CORE

Overlay od rootu repozitáře.

## Směr FIX149
- Žádný nový vizuální hack.
- Návrat/ochrana rychlého baseline z FIX148/FIX145: bez canvas frameholdu.
- Přidány a ověřeny skutečné ROM obrazy od Reného:
  - ATARIXL.ROM 16 KB, MD5 06daac977823773a3eea3422fd26a703
  - ATARIOSB.ROM 10 KB, MD5 a3e8d617c95d08031fe1b20d541434b2
  - ATARIBAS.ROM 8 KB, MD5 0bac0c6a50104045d902df4503a4c30b
- Opravené mapování OS-B 10 KB: OS-B se mapuje jako D800-FFFF, ne falešně do C000.
- XL/XE OS 16 KB zůstává C000-CFFF + D800-FFFF.
- BASIC ROM zůstává A000-BFFF.
- ROM_PROBE profily (generic/Moon/Pac) používají OS-B jako první kompatibilitní mapu; chráněné hry Donkey/Cobra/Montezuma zůstávají na legacy cestě, aby se nerozbily.
- Přidána diagnostika `REAL ROM MAP FIX149` do snapshotu.

## Commit summary
FIX149 real ROM map audit core

## Test plan
1. Jen potvrdit build tag FIX149_REAL_ROM_MAP_AUDIT_CORE.
2. Donkey Kong original – rychlost a šum nesmí být horší než FIX145/FIX148.
3. Donkey Kong Arcade.xex – jestli se méně míchá intro/hra díky generic + OS-B mapě.
4. Donkey Kong Junior Title Version – jestli se nevrátila regrese FIX147.
5. Montezuma PRELIM + Super Cobra – jen potvrdit funkční baseline.
6. Moon Patrol / Arkanoid jen snapshot, pokud bude čas; hlavně řádek `REAL ROM MAP FIX149`.
