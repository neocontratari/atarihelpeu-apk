# AtariHelp.eu EMU-03 - Shift/Rotate

Reakce na log z EMU-02:

RUN 1000 prošlo.
RUN 10000 skončilo:
Unsupported opcode $4A at $2312

$4A je LSR A.

## Co EMU-03 přidává

- LSR A opcode $4A
- ASL A / zp / zp,X / abs / abs,X
- LSR A / zp / zp,X / abs / abs,X
- ROL A / zp / zp,X / abs / abs,X
- ROR A / zp / zp,X / abs / abs,X
- trace posledních instrukcí při stopu

## Test

1. Nainstaluj EMU-03.
2. Hry -> EMU-03 Shift/Rotate.
3. Load XEX into RAM.
4. RUN 10000 instrukcí.
5. Když se zastaví, pošli opcode/adresu + TRACE.
6. Pokud projde, dej RUN 10000 znovu a pošli další stop.
