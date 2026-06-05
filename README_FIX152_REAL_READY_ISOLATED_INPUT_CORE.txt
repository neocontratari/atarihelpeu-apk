# AtariHelp.eu EMU-09 FIX152 REAL READY ISOLATED INPUT CORE

Opravný build po FIX151. FIX151 ukázal ROM audit, ale nebyl jasně oddělený od poslední XEX hry a chybělo přímé tlačítko pro psací READY sandbox.

## Co je nové
- nové tlačítko: TEST REAL ATARI READY
- samostatný izolovaný READY/MEMO PAD sandbox, do kterého jde psát přes mobilní Atari klávesnici nebo HW klávesnici
- TEST ROM AUDIT zůstává jen kontrola ROM bajtů, ne psací režim
- při obou ROM testech se vyčistí poslední XEX stav, takže status/log nesmí ukazovat Montezumu/Donkey jako aktivní hru
- XEX hry zůstávají na bezpečném FIX150/FIX148/FIX145 baseline; ROM se stále nepouští plošně do her

## Commit summary
FIX152 real ready isolated input core

## Test plan
1. Spusť build a zkontroluj tag FIX152_REAL_READY_ISOLATED_INPUT_CORE.
2. Klikni TEST REAL ATARI READY.
3. Napiš HELLO RENE přes mobilní Atari klávesnici dole nebo HW klávesnici.
4. ENTER musí přidat další READY.
5. Pak krátce ověř Donkey / Super Cobra / Montezuma.
