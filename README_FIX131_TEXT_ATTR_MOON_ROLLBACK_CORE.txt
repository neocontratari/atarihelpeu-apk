AtariHelp.eu EMU-09
FIX131_TEXT_ATTR_MOON_ROLLBACK_CORE

Commit summary:
- Zachován FIX130 přínos: písmo/tečky přes OS-ROM CHBASE fallback zůstávají opravené.
- Moon Patrol: rollback přímého velkého runtime segment handoffu z FIX130. FIX131 znovu preferuje FIX129 code-window handoff a sdílí stejný resolved kandidát v should/apply cestě, aby se neopakoval pád do BRK $543A.
- Textové režimy: doplněné Atari screen-code znaky < = > a základní symboly, aby menu kurzory/šipky nezmizely v syntetickém fallback fontu.
- ANTIC text color fallback: když jsou PF2/PF3 kvůli neúplnému DLI zachycení černé, 4barevný text dostane bezpečný kontrast pro Arkanoid/Donkey Jr diagnostiku.
- Build tag: FIX131_TEXT_ATTR_MOON_ROLLBACK_CORE.

Co čekám:
- Arkanoid III menu má zůstat bez teček; možná se objeví/zesílí kurzor/šipka a část barev textu.
- Moon Patrol by se neměl kousnout hned na začátku jako FIX130. Cíl pro tento krok je vrátit stav minimálně na FIX129: běží, i když grafika ještě není správná.
- Donkey Kong / Super Cobra / Montezuma PRELIM nesmí zregresovat.

Test plan pro Reného:
1) Ověřit nahoře/snapshot log, že běží build FIX131_TEXT_ATTR_MOON_ROLLBACK_CORE.
2) Arkanoid III:
   - screenshot menu,
   - zkusit START/FIRE do hry,
   - pokud je hra rozsypaná, udělat snapshot až v té rozsypané hře, ne jen v menu.
3) Moon Patrol:
   - načíst, nic extra nemačkat 5–10 sekund,
   - screenshot + snapshot,
   - sledovat řádky: FIX131 CORE, handoff=, moonWin=, moonSeg=, GENERIC TRAP RECOVER FIX131.
4) Donkey Kong:
   - jen rychle ověřit, že hra běží a že se nezhoršil šum/intro.
5) Super Cobra:
   - otestovat kulomet na cílích a po smrti,
   - udělat snapshot až ve hře při střelbě, pokud zásahy vypadají špatně.
6) Montezuma PRELIM:
   - stejně jako předtím: nechat intro doběhnout do normální grafiky, pak AUTO START/FIRE.

Instalace:
- ZIP rozbalit přesně do rootu lokálního GitHub repozitáře.
- GitHub Desktop: zkontrolovat změny, commit např. "FIX131 text attrs moon rollback core", push.
- GitHub Actions postaví APK.
