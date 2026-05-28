# AtariHelp.eu EMU-06 - ADC Indexed

Reakce na log z EMU-05:

Unsupported opcode $7D at $25C4

$7D je ADC absolute,X.

## Co EMU-06 přidává

- ADC abs,X opcode $7D
- ADC zp,X / abs,Y / (zp,X) / (zp),Y
- SBC indexované režimy
- AND/ORA/EOR indexované režimy
- CMP indexované režimy
- CPX/CPY zero page a absolute
- lepší overflow flag u ADC

## Test

1. Nainstaluj EMU-06.
2. Hry -> EMU-06 ADC Indexed.
3. Load XEX into RAM.
4. RUN 10000.
5. DRŽ START + RUN 60000.
6. Pokud projde, dej RUN 100000.
7. Pošli PC, Steps, OS count a log.
8. Pokud narazí na unsupported opcode, pošli opcode/adresu + TRACE.
