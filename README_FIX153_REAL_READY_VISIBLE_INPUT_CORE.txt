# AtariHelp.eu EMU-09 — FIX153_REAL_READY_VISIBLE_INPUT_CORE

Commit summary:

`FIX153 real READY visible input core`

## Co je opraveno

- FIX152 READY režim přijímal klávesy, ale kreslil do neexistujícího canvasu `screen` místo skutečného `video`, takže obraz zůstal černý.
- FIX153 kreslí REAL ATARI READY/MEMO PAD přímo do `canvas#video`.
- Snapshot v READY režimu už nespadne na `xexInfo.warnings.length`, protože READY/no-XEX stav má bezpečné prázdné `warnings`.
- Po kliknutí na `TEST REAL ATARI READY` se nastaví viditelný no-XEX READY režim, psaní přes mobilní klávesnici/HW klávesnici se má okamžitě objevit v modré obrazovce.
- XEX hry zůstávají na bezpečném baseline; ROM/READY probe nepřepíná Donkey/Cobra/Montezumu do ROM_PROBE.

## Test plan

1. Ověř build tag `FIX153_REAL_READY_VISIBLE_INPUT_CORE`.
2. Klikni `TEST REAL ATARI READY`.
3. Napiš `HELLO RENE` nebo pár písmen přes mobilní Atari klávesnici dole.
4. Obraz Atari musí být modrý, s `ATARI COMPUTER - MEMO PAD`, `READY` a napsaným textem.
5. ENTER musí přidat další `READY`.
6. Potom jen krátce ověř Donkey Kong, Super Cobra a Montezuma PRELIM, že zůstaly jako před FIX152.

## Poznámka

Tohle pořád není kompletní Altirra-style OS boot přes PIA/POKEY/ANTIC reset inicializaci. Je to izolovaný viditelný READY input probe, aby bylo ověřené, že ROM audit + klávesnice + canvas cesta fungují bez zásahu do XEX her.
