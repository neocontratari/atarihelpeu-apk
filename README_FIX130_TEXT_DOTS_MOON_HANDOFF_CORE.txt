AtariHelp.eu EMU-09 — FIX130_TEXT_DOTS_MOON_HANDOFF_CORE
Datum: 2026-06-04

Commit summary:
FIX130 text dots + Moon handoff core

Co je změněno:
1) ANTIC text/charset fallback
   - Pokud CHBASE míří do OS ROM oblasti $E000+, renderer už nečte běžný OS/BASIC ROM obraz jako font.
   - Místo toho použije bezpečný syntetický Atari font pro textové/znakové režimy.
   - Cíl: odstranit Reného pozorované „tečky všude kolem písma“ v menu a textových řádcích.
   - Změna je obecná renderer/charset oprava, ne per-game vizuální hack.

2) ANTIC mode 4/5 fallback
   - Když CHBASE není platný nebo je prázdný, mode 4/5 už nepoužije samotný screen-code bajt jako bitmapový řádek.
   - Použije syntetický glyph řádek, takže mezery nebudou vyrábět tečkovaný bordel.
   - Cíl: Arkanoid III menu, Donkey Jr textové části a další hry s písmy.

3) Moon Patrol runtime handoff
   - FIX129 pořád skákal na Moon Patrol runtime window kolem $9D17 a z logu padal zpět přes BRK $0001.
   - FIX130 u Moon Patrol preferuje velký runtime segment v horní části XEXu před malým code-looking oknem.
   - Cíl: zkusit posunout Moon Patrol dál než špatná horní řádka + šedá plocha.
   - Je to boot/runtime handoff ochrana pouze pro Moon Patrol profil, ne vizuální hack.

4) Diagnostika
   - Build tag: FIX130_TEXT_DOTS_MOON_HANDOFF_CORE
   - Snapshot log přidává charset=... a moonSeg=... do řádku FIX130 CORE.
   - Důležité řádky v logu: DLIST RESOLVE FIX130, FRAMEBUFFER SCAN FIX130, FIX130 CORE, GTIA COLLISION FIX130.

Co záměrně NENÍ změněno:
- Donkey Kong reference není cíleně měněná.
- Super Cobra DLIST $2000 a collision core nejsou agresivně přepsané.
- Montezuma RAM/cache se nečistí agresivně, aby PRELIM zůstal hratelný.

Test plan pro Reného:
1) Ověř nahoře nebo ve snapshot logu build tag:
   FIX130_TEXT_DOTS_MOON_HANDOFF_CORE

2) Arkanoid III:
   - Udělej screenshot menu.
   - Sleduj hlavně, jestli zmizely nebo ubyly tečky kolem písma.
   - Pak START/FIRE do hry a screenshot první obrazovky.

3) Moon Patrol:
   - Načti Moon Patrol a udělej screenshot.
   - Ulož snapshot log.
   - Hlavně poslat řádky: FIX130 CORE, moonSeg=..., DLIST RESOLVE FIX130, FRAMEBUFFER SCAN FIX130.

4) Donkey Kong:
   - Jen rychlá reference: jestli hra pořád běží a je hratelná.
   - Šum mezi intry/menu je známý problém, ale nesmí se rozbít samotná hra.

5) Super Cobra:
   - Ověřit, že menu/hra pořád běží.
   - Zkus střelbu, bomby, zásahy a smrt.
   - Poslat poznámku, jestli se proti FIX129 něco zlepšilo/zhoršilo.

6) Montezuma PRELIM:
   - Nechat intro doběhnout do normální grafiky, potom AUTO START/FIRE jako u FIX129.
   - Ověřit, jestli hra pořád funguje a jestli menu/tečky na začátku vypadají lépe.

Jak ZIP použít:
- ZIP rozbal do rootu lokálního repozitáře přes GitHub Desktop / Průzkumník.
- Neuploaduj ZIP na GitHub jako soubor. Rozbal jeho obsah tak, aby přepsal app/src/main/assets/emu09_pmg_gtia_overlay.html a další soubory.
- Commit/push v GitHub Desktopu.
- GitHub Actions postaví APK.
