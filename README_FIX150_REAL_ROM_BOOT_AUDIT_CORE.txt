# AtariHelp.eu EMU-09 - FIX150_REAL_ROM_BOOT_AUDIT_CORE

Overlay od rootu repozitáře.

## Co je v FIX150

- Vrací se bezpečný FIX148/FIX145 baseline. FIX149 nepoužívat jako testovací základ.
- Hry nejsou plošně přepnuté do ROM_PROBE.
- Přidán samostatný viditelný test `TEST REAL ROM BOOT` přímo v EMU-09.
- Do buildu jsou vestavěné ROM obrazy dodané Reném:
  - ATARIXL.ROM: 16384 bytes, md5 06daac977823773a3eea3422fd26a703
  - ATARIOSB.ROM: 10240 bytes, md5 a3e8d617c95d08031fe1b20d541434b2
  - ATARIBAS.ROM: 8192 bytes, md5 0bac0c6a50104045d902df4503a4c30b
- ROM boot obrazovka kreslí do canvasu `ATARI COMPUTER - MEMO PAD`, `READY`, velikosti, MD5 a mapovací bajty.
- Donkey rychlá cesta bez canvas frameholdu zůstává.
- Montezuma PRELIM z FIX140 a Super Cobra jsou chráněné.

## Commit summary

FIX150 real ROM boot audit core

## Test plan

1. Spusť EMU-09 a ověř build tag `FIX150_REAL_ROM_BOOT_AUDIT_CORE`.
2. Zmáčkni `TEST REAL ROM BOOT`.
3. Na canvasu musí být modrá Atari obrazovka s `ATARI COMPUTER - MEMO PAD`, `READY` a řádky ROM.
4. Až potom krátce ověř Donkey original, Montezuma PRELIM a Super Cobra, že nejsou rozbité jako ve FIX149.
