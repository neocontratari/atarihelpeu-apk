EMU-09 FIX69 DLIST CORE + REFERENCES

Co je nove:

- Tlacitka vestavenych her jsou zamerne zkracena na PiTT-KiTT, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chranena reference.
- KiTT Garage je pridana jako druha chranena reference a uz nespadne do profilu PiTT.
- Super Cobra dostava obecnou opravu DLIST validace pro hry, ktere maji LMS skoro na kazdem radku.
- Donkey dostava obecne PMG overlap kolize P/M, aby bonusove objekty nebyly neviditelne pro herni logiku.

Testuj hlavne:

1. PiTT-KiTT Remaster - pismo a hra musi zustat v poradku.
2. KiTT Garage - nacteni a obraz pres obecny DLIST.
3. Donkey Kong - intro muze byt rychle; zkus druhy level a bonusy.
4. Super Cobra - po rucnim START/FIRE ma byt DLIST $2000, ne fallback/cerna plocha.
5. Pitstop II - nesmi se zhorsit split obraz.
