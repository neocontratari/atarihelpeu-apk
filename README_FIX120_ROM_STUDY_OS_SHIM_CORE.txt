# FIX120_ROM_STUDY_OS_SHIM_CORE

Tento overlay nepřibaluje žádné ROM soubory. ROMky od Reného byly použité jen jako technická reference pro mapu Atari OS/BASIC prostoru.

## Zjištění z dodaných souborů

- `ATARIBAS.ROM`: 8192 bytes, md5 `0bac0c6a50104045d902df4503a4c30b`
- `ATARIOSB.ROM`: 10240 bytes, md5 `a3e8d617c95d08031fe1b20d541434b2`
- `ATARIXL(1).ROM`: 16384 bytes, md5 `06daac977823773a3eea3422fd26a703`
- `REV02.ROM`: 16384 bytes, md5 `06daac977823773a3eea3422fd26a703`
- `REV03.ROM`: 16384 bytes, md5 `54e704558a6aedfc45cebf8f8ac9c312`
- `REV04.ROM`: 16384 bytes, md5 `b7a2a04677d34f069eeb643d5238bf86`
- `REV10.ROM`: 16384 bytes, md5 `e3e8c74bfe1dcd6b56af50bd9a82dc15`
- `REVA.ROM`: 8192 bytes, md5 `a4dc52536d526ecc51ea857b9fa2b90f`
- `REVB.ROM`: 8192 bytes, md5 `04ea6a4e386601445ca5bfc8e37fb620`
- `REVC.ROM`: 8192 bytes, md5 `0bac0c6a50104045d902df4503a4c30b`

Duplicitní / důležité:
- `ATARIXL(1).ROM`, `ATARIXL.ROM` a `REV02.ROM` mají shodný 16KB obraz.
- `ATARIBAS.ROM` a `REVC.ROM` mají shodný 8KB BASIC obraz.
- 16KB OS obrazy pomáhají určit, že interní mini-OS nesmí řešit jen `$E000+`, ale i `$C000-$CFFF`, `$D800-$FFFF` a opatrně také skoky do `$D000-$D7FF` jako HW/OS trap.
- 8KB BASIC obrazy potvrzují, že hry jako De Re Pac-Man mohou chtít víc než jednoduchý DLIST fix; volají BASIC/OS prostor masivně.

## Změny v kódu

- Externí ROM není požadovaná od testera.
- ROM loader není testovací povinnost.
- Interní mini OS/HW shim nově bezpečně zachytí i OS-B/XL/XE ROM oblasti `$C000-$CFFF`, `$D800-$FFFF`.
- Skok do `$D000-$D7FF` se u ROM-less profilu bere jako HW/OS trampoline místo okamžitého BRK.
- OS skeleton vyplní ROM prostor bezpečnými NOP/RTS bajty bez kopírování skutečné ROM.
- Donkey / Cobra / Arkanoid baseline zůstává chráněný.

## Test

1. Donkey / Cobra / Arkanoid jen rychlá kontrola.
2. Moon Patrol a Pac-Man bez nahrávání ROM.
3. Montezuma PRELIM/ALT/ORIG jen jestli není horší než FIX119/FIX116.
