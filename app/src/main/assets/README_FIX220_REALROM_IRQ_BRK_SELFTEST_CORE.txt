AtariHelp.eu EMU-09 FIX220_REALROM_IRQ_BRK_SELFTEST_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nejsou zmenene.

Navazuje na FIX219.

Proc:
- FIX219 uz dovedl PURE ROM BASIC BOOT na realnou READY obrazovku.
- PURE ROM SELF TEST ale zastavil na BRK v RAM okolo $68BC, protoze emulator odmital realny IRQ vektor OS ROM $C02C.
- Na skutecnem 6502 BRK vzdy pushne PC/P a skoci pres $FFFE/$FFFF. U XL ROM to je realny IRQ handler $C02C.

Zmeny:
- v PURE REAL ROM modu se BRK vektory do OS ROM $C000-$FFFF provadi jako realny 6502 BRK,
- konkretne self-test uz nema byt zastaven safe-stopem kvuli IRQV=$C02C,
- pridany snapshot radek: FIX220 BRK IRQ VECTOR,
- READY se nefakuje,
- SELF TEST obraz se nefakuje,
- ROM soubory zustavaji puvodni Reneho ROMy.

Test plan:
1. PURE ROM BASIC BOOT - jen overit, ze READY zustava.
2. PURE ROM SELF TEST - hlavni test; snapshot po chvili.
3. Pokud neuvidis SELF TEST menu, poslat screenshot + snapshot s radky:
   BUILD TAG FIX220_REALROM_IRQ_BRK_SELFTEST_CORE
   FIX220 BRK IRQ VECTOR
   FIX220 SELFTEST GATE LIVE
   FIX220 REAL ROM VBI
   FIX220 SIO TIMEOUT
   DLIST / SAVMSC / PC
