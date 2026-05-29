EMU-09 FIX14 ORIGINAL XEX + PALETTE/DLIST

- Vestavěný PiTT-KiTT XEX zůstává byte-for-byte z posledního dodaného souboru.
- Opraveno mapování 2BPP barev: 00=COLBK, 01=COLPF0, 10=COLPF1, 11=COLPF2.
- Opravená hrubá GTIA/PAL paleta: $C4 je zelená louka, $76 modrá obloha, $04 šedá silnice, $0F bílá.
- Display-listy intra/prohry/výhry/rekordu se už nedrží na starých adresách; po načtení XEX se detekují ze self-loop DLISTů v paměti.
- Super Cobra zůstává generic XEX test; není ještě plně podporovaná Atari emulace.
