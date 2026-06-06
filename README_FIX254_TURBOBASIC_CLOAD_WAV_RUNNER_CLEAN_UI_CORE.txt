AtariHelp.eu EMU-09 FIX254 TURBOBASIC CLOAD WAV RUNNER CLEAN UI CORE

Commit summary:
FIX254 TurboBasic CLOAD WAV runner clean UI core

CIL:
- Cisty panel bez starych zalozek a bez PiTT-KiTT zbytku.
- Kazeťak musi byt pro cloveka hned videt a slyset.
- PLAY pouziva realny TurboBasicXXL.wav pres HTML audio/media tag, ne jen tichy log.
- RESET/STOP tvrde vypne kazetovy zvuk.
- SNAPSHOT ma fallback textarea, kdyz Android/Nox nestahne TXT.
- CLOAD + PLAY posle realny prikaz CLOAD do BASICu a pusti realny WAV zvuk; stale bez fake RAM load.
- SELF TEST cesta z FIX248/FIX249 zustava zachovana, bez kresleneho fake self-testu.

DULEZITE:
Tohle porad neni tvrzeni, ze TurboBasic je dekodovany do RAM. Je to poctivy runner: slyset realny WAV, videt kotouce, ROM/BASIC dostane CLOAD prikaz. Napojeni audio hran na Atari cassette vstup bude dalsi HW krok.

KODY JSOU STEJNE
