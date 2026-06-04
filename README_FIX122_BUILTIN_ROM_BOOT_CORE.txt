AtariHelp.eu EMU-09 FIX122 BUILTIN ROM BOOT CORE

Co je nové:
- FIX122 už ROMky nepoužívá jen jako poznámku/mapu. ATARIXL.ROM 16K a ATARIBAS.ROM 8K jsou vestavěně mapované v emulátoru.
- Tester nic nenahrává. Tlačítka pro ROM zůstávají jen informačně/override, běžná cesta je bez ruční ROM.
- OS ROM se mapuje na $C000-$CFFF a $D800-$FFFF, BASIC na $A000-$BFFF. $D000-$D7FF zůstává hardware.
- Kritické OS vstupy CIOV/SIOV/SETVBV/SYSVBV/XITVBV se stále obsluhují bezpečným shimem, aby se XEXy nezacyklily v reálném OS bez periferií.
- Moon Patrol/Pac-Man mají teď skutečný ROM obraz pro čtení/kód místo slepého internal-vector stubu.
- Donkey/Cobra/Arkanoid baseline je chráněný.

Commit Summary:
FIX122 builtin ROM boot core

Test:
1) Donkey / Cobra / Arkanoid jen ověřit, že se nerozbily.
2) Moon Patrol 5-10 s, screenshot + snapshot log.
3) Pac-Man 5-10 s, screenshot + snapshot log.
4) Montezuma PRELIM + ALT/ORIG: grafika, smrt/reload, 3. level.
