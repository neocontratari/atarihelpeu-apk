AtariHelp.eu EMU-09 FIX98 PROTECTED REFERENCES + VVBLKD PMG CORE

FIX98 je opatrný strukturální balík po FIX97. Donkey Kong a Super Cobra zůstávají chráněné reference bez agresivního DLIST promotion z FIX95. Arkanoid menu zůstává čisté po opravě ANTIC 5/7 text scanline. Montezuma dostává důležitější Atari OS opravu: pokud hra nastaví deferred VBI přes SETVBV/VVBLKD a hardware VBI vektor je prázdný, mini core teď volá VVBLKD jako Atari OS. To může vrátit PMG animaci/hráče, ne jen posouvat lebku.

Změny:
- Donkey: stabilní frame capture už nesmí uložit přechodový DLIST $1200 jako „poslední dobrý obraz“.
- Cobra: ořez PMG missile kreslení/kolizí mimo herní okno a přísnější missile-hit guard pro falešné zásahy do HUD/textu.
- Montezuma: VVBLKD deferred VBI fallback + jemnější PMG Y offset 6 px, aby lebka/hráč seděli na cihlu.
- Arkanoid: zachováno menu bez auto START/FIRE; gameplay se zatím bere jako další fáze.
- Snapshot přidává FIX98 PROTECTED CORE řádek: deferredVBI / pmgGuard / donkeyStable.

Commit Summary:
FIX98 protected references VVBLKD PMG core
