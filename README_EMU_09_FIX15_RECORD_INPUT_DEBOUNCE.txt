EMU-09 FIX15 RECORD INPUT DEBOUNCE

Základ: FIX14 ORIGINAL XEX + PALETTE/DLIST.

Změna: původní PiTT-KiTT XEX zůstává byte-for-byte stejný. Opravuje se jen mobilní ovládání high-score podpisu.

Problém: při podpisu rekordu držel dotyk FIRE/RIGHT déle než reálné krátké klepnutí a hra mohla potvrdit víc písmen najednou, takže po prvním znaku skočila zpět do intra.

Oprava:
- FIRE v high-score obrazovce je jednorázově zalatchované: jedno klepnutí = jedno potvrzení.
- RIGHT je v high-score obrazovce ignorované jako potvrzení; zůstává jako joystick pro řízení ve hře.
- UP/DOWN dál mění písmeno.
- XEX, barvy, DLIST, PMG a hra samotná nejsou upravené.
