FIX84 GALAXIAN PF/PMG SAFE + VJOY TUNE

Zaklad: FIX83 / stabilni FIX77-FIX81 smer.
Cil: nevracet rollback, ale opatrne posunout Galaxian a trochu zklidnit kruhovy joystick.

Zmeny:
- joystick je malinko mene citlivy: vetsi mrtva zona, mensi maximalni tah; bez timeru.
- Galaxian umi lokalne preferovat runtime naskenovany DLIST, pokud aktualni DLIST kresli moc malou cast obrazu.
- Galaxian ma lokalni PF barevny fallback, kdyz DLI wrapper vrati PF1-PF3 nulove a objekty by zmizely v cerne.
- Galaxian povoluje PMBASE $0000 jako low-memory PMG fallback jen pro tento profil.
- Cobra/Donkey/PiTT/KiTT/Pitstop nejsou zamyslene jako dotcene zmeny.

Test poradi:
1) Super Cobra: scroll + vrtulnik.
2) Donkey Kong: najede a hraje.
3) Joystick: citlivost trochu klidnejsi.
4) Galaxian: jestli se objevi lod/raketka a jestli se obraz po START/FIRE zlepsi.
