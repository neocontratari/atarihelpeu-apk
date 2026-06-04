AtariHelp.eu EMU-09 - FIX136_RUNTIME_ISOLATION_VIDEO_STRATEGY_CORE

Commit summary:
FIX136 runtime isolation video strategy core

Proč tento build:
Po FIX135 už Super Cobra vypadá jako použitelná reference, ale ostatní hry se pořád plácají v šumu/rozjeté grafice. Logy ukázaly zásadní systémový problém: diagnostika a někdy i cache strategie z jedné hry zůstávala po načtení další hry. V Arkanoidu se objevoval Moon framebuffer scan, v Moon Patrol se objevoval DLI režim z jiné hry. To je špatně pro univerzální emulátor.

Hlavní změny:
- Runtime izolace mezi XEX: resetuje se AUTO COMPAT, DLI cache, framebuffer scan, video fallback, viewport a diagnostické stavové proměnné při každém cold loadu i po detekci profilu.
- DLI cache guard podle profilu: generic DLI snapshot se smí znovu použít jen pro stejný profil a stejný display list. Tím se nemá přenášet barevná strategie mezi Moon/Arkanoid/Montezuma/Death Race.
- Moon/Pac/generic framebuffer fallback: hledá i nevyrovnané LMS adresy přímo z display listu, nejen bloky po $80. To je důležité pro Moon Patrol, jehož DLIST ukazuje na adresy jako $42FF/$A8FF.
- Donkey Kong: delší potlačení známého přechodového DLISTu $1200 pomocí posledního stabilního snímku, aby se omezil šum mezi intry/menu/levelem.
- FIX136 build tag přepsaný v HTML, indexu, snapshotu a log filename.

Co NEřeším v tomto buildu:
- Super Cobra kulomet dál nehoním, protože tester potvrdil, že hra je hratelná.
- Nepřidávám falešné nápisy ani kreslené overlaye přes hru.
- Nezamykám změnu na jednu hru; jde o izolaci a výběr video strategie pro více XEX.

Test plan:
1) Ověřit build tag FIX136_RUNTIME_ISOLATION_VIDEO_STRATEGY_CORE v aplikaci i snapshotu.
2) Moon Patrol: screenshot + snapshot, jestli se změnil šum/fallback a jestli FRAMEBUFFER SCAN píše LMS adresy.
3) Arkanoid III: menu + první hra; ověřit, jestli už v diagnostice nezůstává Moon framebuffer scan a jestli se hra nechová jinak.
4) Montezuma PRELIM: ověřit grafiku ve hře, hlavně jestli se po čistém loadu nezhoršuje kvůli starému fallbacku.
5) Donkey Kong: běh hry + přechody mezi intry/menu/levelem; ověřit, jestli ubyl šum.
6) Super Cobra: jen rychlá reference, že zůstává hratelná a bez trvalého PLAYER 1 overlaye.
