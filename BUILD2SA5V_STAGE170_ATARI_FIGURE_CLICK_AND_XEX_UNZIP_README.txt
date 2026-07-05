BUILD2SA5V / STAGE170 - Atari web cards clickable + XEX unzip before 130XE inject

Co opravuje:
- Na AtariHelp XEX strance je skutecny ZIP odkaz casto jen na obrazku.
- Popisek typu "Atari Donkey Kong XEX" neni odkaz, takze klepnuti na text mohlo neudelat nic.
- Bridge ted chyta klik na cely WordPress figure/wp-block-image blok a pouzije ZIP odkaz z obrazku.
- Java uz pred injektem do 130XE rozbali Atari ZIP a posle do AHRECV primo vnitrni .xex/.com/.exe nebo .atr.

Chovani:
- Raw Sega ROM (.gen/.md/.smd/.sms/.68k/.sgd) zustava Sega.
- ZIP se stale rozhoduje podle obsahu.
- ZIP se Sega ROM jde do Sega emu.
- ZIP s Atari XEX/COM/EXE/ATR jde do 130XE jako rozbaleny vnitrni soubor.
- Chyba downloadu stale nesmi sama otevrit 130XE.

Log markery:
- BUILD2SA5V ZIP_CONTAINS_ATARI
- BUILD2SA5U WEB_GAME_DOWNLOADED ... zipContentDetect=ON
- BUILD2SA5U ZIP_CONTAINS_SEGA
- BUILD2SA5S PROVIDER_RELAY_OK mode=0

Testovane vstupy:
- donkey_kong.zip -> Donkey Kong.xex
- galactic_chase.zip -> Galactic Chase.xex
- Sonic-The-Hedgehog-USA-Europe.zip -> Sonic The Hedgehog (USA, Europe).gen
