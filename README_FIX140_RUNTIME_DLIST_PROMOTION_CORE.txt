AtariHelp.eu EMU-09
FIX140_RUNTIME_DLIST_PROMOTION_CORE

Cíl:
- Přestat honit mobilní výkon naslepo a vrátit se k viditelnému posunu v kompatibilitě.
- Zachovat mobilní log/DOM/WebView ochranu z FIX139.
- Opatrně zlepšit obecné rozpoznání runtime display listu pro hry, které při přechodu drží v SDLST/HW pointer na přechodový nebo no-end DLIST, zatímco v RAM už existuje stabilnější runtime DLIST se self-JVB smyčkou.

Co se změnilo:
1) Runtime DLIST promotion
- Přidané pravidlo: pokud živý DLIST nemá JVB/JMP konec a scanner najde stabilní self-JVB runtime DLIST s rozumnou výškou obrazu, emulátor ho smí povýšit.
- To není kreslicí hack pro jednu hru; je to obecná strukturální ochrana proti tomu, aby renderer četl přepsaná data jako DLIST.
- Očekávaný dopad hlavně na Montezuma/DeathRace-class přechodové obrazovky.

2) Chráněné reference
- Donkey Kong a Super Cobra zůstávají na chráněné cestě.
- Cobra zůstává hratelná reference, Donkey se nesmí znovu rozbít změnou DLIST strategie.

3) Mobil
- Neřeším teď další agresivní mobilní škrcení, protože uživatel nechce znovu trávit čas mobilním testem.
- FIX139 log/DOM guard zůstává.
- Snapshot stále ukazuje MOBILE PERF, ale hlavní test FIX140 je obrazová kompatibilita.

Build tag:
FIX140_RUNTIME_DLIST_PROMOTION_CORE

Commit summary:
FIX140 runtime dlist promotion core

Krátký test plan:
1. Ověřit build tag FIX140.
2. Montezuma PRELIM: jestli se ve hře zlepší rozjetá grafika nebo alespoň snapshot ukáže promoted self-JVB runtime DLIST.
3. Donkey Kong: menu + dupající intro + hra, nesmí být horší než FIX137/FIX139.
4. Super Cobra: jen potvrdit hratelnost.
5. Arkanoid III: krátce menu + první hra.
6. Moon Patrol: jen jednou, neočekává se velká změna; pořád je to OS/loader/VBI problém, ne obyčejný DLIST.
