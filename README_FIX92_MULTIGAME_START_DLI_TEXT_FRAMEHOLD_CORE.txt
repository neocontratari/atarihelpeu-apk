FIX92 MULTIGAME START + DLI TEXT + FRAME HOLD CORE

Cíl:
- Nezáplatovat jednu hru po druhé, ale zlepšit společné části emulátoru:
  1) hry stojící na PAUSE/INTRO/START,
  2) rozhozené přechodové snímky při změně DLISTu,
  3) černý/nečitelný DLI text v horních lištách,
  4) 240px viewport bez falešných horních stínů.

Základ:
- Navazuje na FIX91/FIX89 směr, kde Donkey znovu funguje a obraz je lépe vycentrovaný.
- Donkey, Cobra, PiTT, KiTT, Pitstop, River, Galaxian zůstávají chráněné reference.
- Generic XEX zůstává pro další hry, např. Death Race.

Nové v FIX92:
- AUTO START/FIRE tlačítko pro hry, které visí na pause/menu/intro.
  Posílá sekvenci START, FIRE, START+FIRE, OPTION, SELECT, START.
  Není to automatický hack; uživatel ho spustí ručně, když hra stojí.

- DLIST transition frame hold:
  Když se během přechodu hry změní DLIST nebo je DLIST krátce podezřelý,
  renderer na 2–4 frame podrží starší obraz místo zobrazení šumu mezi intry.
  Log: FRAME HOLD FIX92.

- DLI text fallback:
  Když DLI nechá textovou barvu stejnou jako pozadí, renderer zvolí čitelnější barvu.
  Pomáhá hlavně horním textům/menu typu Cobra a obecným XEX textovým obrazovkám.

- Snapshot/log:
  Build tag je FIX92_MULTIGAME_START_DLI_TEXT_FRAMEHOLD_CORE.
  Snapshot obsahuje FRAME HOLD FIX92, VIEWPORT FIX92 a build tag.

Testovací pořadí:
1. Donkey Kong: hra, centrování, horní patro, sud/skok, intro přechody.
2. Super Cobra: horní text, scroll, vrtulník, restart, střelba.
3. Death Race: intro/menu, AUTO START/FIRE, obraz ve hře, snapshot.
4. Galaxian: jestli se nezhoršil proti FIX91.
5. Jedna náhodná nová hra: první obraz, jestli stojí na PAUSE, AUTO START/FIRE, snapshot.
