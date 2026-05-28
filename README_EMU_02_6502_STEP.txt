# AtariHelp.eu EMU-02 - 6502 Step Lab

Reakce na log z EMU-01:

Unsupported opcode $85 at $221C

$85 je STA zero page. EMU-02 ho doplňuje a přidává větší balík běžných 6502 instrukcí.

## Co testovat

1. Nainstaluj EMU-02.
2. Appka musí pořád fungovat jako v25.
3. Hry -> EMU-02 6502 Step Lab.
4. Load XEX into RAM.
5. RUN 1000 instrukcí.
6. Když se zastaví, pošli mi opcode/adresu.
7. Když doběhne, zkus RUN 10000 instrukcí a pošli další stop/log.

## Důležité

Ještě to není hotový emulátor.
Cíl je iterovat přes skutečné stop body tvého XEX.
