AtariHelp.eu EMU-09 FIX87 STABLE SCOPED OSROM + DEATH RACE CORE

Cíl: neopakovat chybu FIX86. Death Race a OS ROM/I/O ochrana jsou zapnuté jen pro obecné XEX profily, ne pro stabilní reference Cobra/Donkey/PiTT/KiTT/Pitstop/River.

Základ: stabilní FIX84 větev.

Co je nové:
- Death Race.xex je přidán jako vestavěný test.
- Async OS ROM/I/O exit guard je scoped: activeXexProfile generic/arkanoid/galaxian.
- Cobra a Donkey běží původní FIX84 async cestou; Death Race nesmí rozbíjet reference.
- Přidaný log: ASYNC OS EXIT FIX87 scoped=...
- Joystick zůstává z FIX84, protože v testu seděl nejlépe.

Test pořadí:
1. Cobra: scroll, vrtulník, zásahy.
2. Donkey: intro, sud v horním patře, skoky, vstup do hry.
3. Galaxian: bez regresu.
4. Death Race: zda už nedojde k VBI trapu do $D006 / OS ROM prázdné oblasti.

Poznámka:
Toto není jednoherní hack. Je to návrat k funkčnímu stabilnímu jádru a obecná, ale profilově bezpečná oprava OS ROM/I/O exitů.
