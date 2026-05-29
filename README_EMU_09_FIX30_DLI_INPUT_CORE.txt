EMU-09 FIX30 DLI/INPUT CORE

Základ: FIX29 Core Test Suite, ne FIX26.

Cíl: nelepíme River/Pitstop/Arkanoid po jednom obrázku. Opravy jsou v jádru:
- PiTT-KiTT zůstává referenční hra.
- TRIG1-TRIG3 se čtou před collision fallbackem $D000-$D00F.
- CH keyboard shadow $02FC umí krátké START/FIRE klávesové pulsy pro menu hry jako Arkanoid.
- Generic XEX už nepouští DLI dvakrát naslepo v main loopu. DLI se pro obecný renderer snímá do snapshotů přes WSYNC a používá se pro barvy po řádcích.
- Snapshot vypisuje DLI CAP počty.

Test:
1) PiTT-KiTT kontrolně.
2) Arkanoid III: zkus TAP FIRE na menu.
3) Pitstop II: pošli snapshot, hlavně DLI CAP a ANTIC řádky.
4) River Raid kontrolně, jestli se nerozbil.
