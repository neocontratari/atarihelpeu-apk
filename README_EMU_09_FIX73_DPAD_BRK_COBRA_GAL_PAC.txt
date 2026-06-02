EMU-09 FIX73 DPAD + BRK + COBRA + GAL/PAC

Co je nove proti FIX72:

- Mobilni ovladani je prestavene na levy palcovy kriz: nahoru, dolu, vlevo, vpravo.
- FIRE je velke tlacitko vpravo pro pravou ruku.
- START zustava uprostred krize a kratke TAP START+FIRE je vpravo nahore.
- Super Cobra renderuje PMG/kolize kazdy frame a male missile kolize maji sirsi playfield vzorkovani.
- Obecny XEX core umi pri BRK tesne pred nactenym segmentem pokracovat na nejblizsim nactenem kodu. Cili hlavne loader/trampoline chyby typu Galactic Chase po intru.
- Obecny renderer ma posledni zachranu: kdyz SDLST/HW DLIST pointer neni pouzitelny, opatrne skenuje nactene segmenty na solidni display list.
- Status radek ukazuje core recover BRK a scanDL, aby slo z mobilu poznat, jestli tahle zachrana opravdu zabrala.

Chranene reference:

- PiTT-KiTT Remaster zustava nedotknutelny test.
- KiTT Garage zustava druha chranena reference.

Testuj hlavne:

1. PiTT-KiTT Remaster - pismo a hra se nesmi zhorsit.
2. KiTT Garage - barvy, texty a rychlost.
3. Donkey Kong - intro a druhy level.
4. Super Cobra - intro, barvy a jestli FIRE lip zasahuje.
5. Galactic Chase - jestli po intru uz nespadne na BRK $1888.
6. De Re Pac-Man - jestli je aspon posun proti sumu; tenhle titul muze chtit jeste hlubsi OS/CIO emulaci.
