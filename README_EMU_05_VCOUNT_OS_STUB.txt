# AtariHelp.eu EMU-05 - VCOUNT / OS Stub

Reakce na test EMU-04:

START vstup byl v pořádku, ale PC zůstalo ve smyčce kolem $2390.

Disassembly ukazuje:

$2390: LDA $D40B
$2393: CMP #$3C
$2395: BCC $2390
$2397: JMP $22C7

To je čekání na VCOUNT, ne chyba START tlačítka.

## Co EMU-05 přidává

- VCOUNT $D40B už běží a nevrací pořád 0
- WSYNC $D40A posouvá VCOUNT
- JSR do Atari OS ROM $E000-$FFFF se bere jako stub/návrat
- hlavně SETVBV $EF9C už neposílá CPU do prázdné ROM
- tlačítko DRŽ START + RUN 60000
- vysvětlení smyčky přímo v appce

## Test

1. Nainstaluj EMU-05.
2. Hry -> EMU-05 VCOUNT.
3. Load XEX into RAM.
4. RUN 10000.
5. DRŽ START + RUN 60000.
6. Pošli PC, Steps, OS count a log.
7. Pokud narazí na unsupported opcode, pošli opcode/adresu + TRACE.
