BUILD2SA5U / STAGE169 - ZIP content detect, Atari XEX fix

Co opravuje:
- Stage168 uz drzela Sega ZIP v Sega emu, ale mohla omylem poslat Atari ZIP do Sega trasy,
  pokud byl klik proveden z kontextu Sega/Web sbirky.
- Atari ZIP s XEX pak Sega extraktor odmitl, proto se XEX nespustilo.

Nova logika:
- Raw Sega ROM odkazy (.gen/.md/.smd/.sms/.68k/.sgd) jdou porad primo do Sega emu.
- ZIP odkazy se uz nerozhoduji podle obrazovky/kontextu predem.
- Kazdy ZIP se nejdriv stahne pres provider relay a potom se rozhodne podle obsahu:
  - pokud ZIP obsahuje Sega ROM, jde do Sega emu.
  - pokud ZIP obsahuje XEX/ATR/Atari data, zustava 130XE loader.
- Download limit pro obecny ZIP je 16 MB, aby Sega i Atari ZIPy mely rezervu.
- Chyba downloadu stale nesmi sama otevrit 130XE.

Zachovano:
- Provider bypass z BUILD2SA5S: prvni cesta https://proxy.cors.sh/<puvodni_url>.
- Stage168 fix: zadny fallback skok do 130XE pri Sega download chybe.
- PS1 Stage164 audio rollback zustava beze zmen.
- Zadny BIOS ZIP import neni pridan.

Log markery:
- BUILD2SA5U WEB_GAME_DOWNLOADED ... zipContentDetect=ON
- BUILD2SA5U ZIP_CONTAINS_SEGA
- BUILD2SA5U WEB_GAME_DOWNLOAD_FAIL
- BUILD2SA5T SEGA_ARCHIVE_* zustava jen nouzova/nepouzita kompatibilni cesta.

Test:
- Sonic ZIP pres relay obsahuje Sonic The Hedgehog (USA, Europe).gen.
- Donkey Kong ZIP pres relay obsahuje Donkey Kong.xex.
- Galactic Chase ZIP pres relay obsahuje Galactic Chase.xex.
