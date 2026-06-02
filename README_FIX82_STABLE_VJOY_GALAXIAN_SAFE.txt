AtariHelp.eu EMU-09 FIX82 STABLE VJOY + GALAXIAN SAFE

DULEZITE:
- FIX82 navazuje na uzivatelem potvrzeny dobry stav FIX81: "vse super, vsechno jede".
- Nevraci se k rozbitemu smeru FIX78/FIX79.
- Cobra/Donkey core zustava ze stabilni vetve.
- Kruhovy joystick je jen citlivejsi UI: mensi mrtva zona, bez timeru, bez extra CPU loopu.
- Galaxian (Title Version 2) je pridan opatrne:
  * profil podle nazvu / ENTRY $9B86 / delky 18033 B,
  * INIT-only BRK idle jen pro Galaxian,
  * DLI/VBI RTI/RTS ochrana jen pro Galaxian / INIT-only,
  * zadne globalni zmeny do Cobry/Donkey.

TESTOVAT:
1) Super Cobra: roluje krajina, je videt vrtulnik.
2) Donkey Kong: dojde do hry jako ve FIX81.
3) Joystick: citlivejsi, ale nezpomaluje.
4) Galaxian: test pres vestavene tlacitko nebo pres vlastni XEX.
5) Pokud Cobra/Donkey regrese: fix se musi vratit a Galaxian izolovat jeste vic.
