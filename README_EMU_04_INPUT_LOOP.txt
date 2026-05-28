# AtariHelp.eu EMU-04 - Input / Loop Lab

Reakce na test EMU-03:

EMU-03 už se nezastavuje na opcode.
PC se opakuje kolem $2390 / $2393 / $2395.
To vypadá jako čekací smyčka na vstup nebo časování.

## Co EMU-04 přidává

- START button přes CONSOL $D01F
- FIRE button přes TRIG0 $D010
- LEFT/RIGHT přes PORTA $D300
- RUN FRAME
- TAP START + RUN FRAME
- TAP FIRE + RUN FRAME
- jednoduchá detekce PC smyčky

## Test

1. Nainstaluj EMU-04.
2. Hry -> EMU-04 Input/Loop.
3. Load XEX into RAM.
4. RUN 10000.
5. TAP START + RUN FRAME.
6. TAP FIRE + RUN FRAME.
7. Drž FIRE a dej RUN FRAME.
8. Pošli PC/log po těchto pokusech.

Cíl: zjistit, jestli se XEX pohne z čekací smyčky dál.
