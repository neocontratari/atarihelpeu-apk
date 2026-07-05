AtariHelp EMU10 BUILD2SA5AI / Stage183

Proc:
- log ze Stage182 ukazal, ze Atari ZIP z webu se stahne spravne
- ZIP se spravne rozbali na Donkey Kong.xex
- chyba byla po radku EMU130_QUEUE: uz neprisel prechod do 130XE injekce

Oprava:
- po netovem Atari XEX/ZIP se uz nespolehame jen na onPageFinished
- fronta po stazeni:
  1. ulozi XEX payload
  2. zastavi bezici webovy load
  3. otevre file:///android_asset/emu/index.html
  4. okamzite naplanuje inject retry
- pokud WebView porad neni na 130XE URL, retry ho v rozumnych intervalech znovu otevre
- az je 130XE pripraveny, posle AHRECV_BEGIN/PART/END jako lokalni XEX picker

Log znacky:
- BUILD2SA5AI EMU130_FORCE_OPEN
- BUILD2SA5AI EMU130_INJECT_REOPEN
- BUILD2SA5AG EMU130_INJECT_SCHEDULE
- BUILD2SA5AH EMU130_INJECT_COMMIT

Nezmeneno:
- Sega cesta zustava oddelena
- lokalni XEX z mobilu zustava beze zmen
- 130XE jadro/assets nejsou menene
- zadny BIOS/bin/rom payload
