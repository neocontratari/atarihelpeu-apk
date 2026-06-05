# FIX151_REAL_ATARI_READY_PROBE_CORE

Overlay pro AtariHelp.eu EMU-09.

## Hlavní změna
- FIX149 se nepoužívá.
- FIX151 vychází z bezpečného FIX150/FIX148/FIX145 baseline.
- Přidává dočasný test **TEST REAL ATARI READY**.
- Test zobrazí modrou Atari MEMO PAD/READY obrazovku a dovolí psát do ní přes mobilní Atari klávesnici nebo fyzickou klávesnici.
- ROM bajty z ATARIXL/ATARIOSB/ATARIBAS jsou stále vložené a test vypíše reset/IRQ/NMI vektory.
- XEX hry nejsou plošně přepnuté do ROM_PROBE.

## Commit summary
FIX151 real Atari READY probe core

## Krátký test plan
1. Ověřit build tag: FIX151_REAL_ATARI_READY_PROBE_CORE.
2. Kliknout TEST REAL ATARI READY.
3. Do modré Atari obrazovky napsat pár písmen a ENTER.
4. Krátce otestovat Donkey Kong, Super Cobra a Montezuma PRELIM, zda zůstaly funkční.

Poznámka: režim READY je záměrně oddělený test/audit. Je určený k ověření ROM, mapování a klávesnicové cesty bez rizika rozbití XEX jádra.
