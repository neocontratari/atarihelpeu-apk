EMU-09 FIX71 ACTIONS RETRY + OSCOLOR + TEXT + COLLISION

Co je nove:

- Tlacitka vestavenych her jsou zamerne zkracena na PiTT-KiTT, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chranena reference.
- KiTT Garage je pridana jako druha chranena reference a uz nespadne do profilu PiTT.
- KiTT Garage ma vlastni pomalejsi takt, OS shadow barvy se obnovuji pred DLI a ANTIC text 2/3 bere normalni znaky z COLPF2.
- Super Cobra dostava obecnou opravu DLIST validace pro hry, ktere maji LMS skoro na kazdem radku.
- Obecne PMG hry maji odlozene mazani GTIA kolizi po HITCLR, aby si hra mohla precist zasahy po renderovanem frame.
- Donkey dostava obecne PMG overlap kolize P/M, aby bonusove objekty nebyly neviditelne pro herni logiku.

Testuj hlavne:

1. PiTT-KiTT Remaster - pismo a hra musi zustat v poradku.
2. KiTT Garage - barvy, textove radky a rychlost.
3. Donkey Kong - intro muze byt rychle; zkus druhy level a bonusy.
4. Super Cobra - menu/start, barvy a jestli FIRE zasahuje cile.
5. Pitstop II - nesmi se zhorsit split obraz.


