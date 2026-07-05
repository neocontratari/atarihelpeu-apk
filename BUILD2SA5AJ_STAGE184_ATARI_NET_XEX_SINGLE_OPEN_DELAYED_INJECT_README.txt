AtariHelp EMU10 BUILD2SA5AJ / Stage184

Proc:
- Stage183 uz dostal WebView z webu do 130XE, ale uzivatel videl opakovane cerne probliknuti
- to znamena, ze opakovane force reloady prilis rusily nabeh 130XE stranky

Oprava:
- po webovem Atari XEX/ZIP se 130XE otevre jen jednou
- zadne opakovane stopLoading/loadUrl behem startu
- po otevreni 130XE se ceka pevny settle delay jako u Sega delayed inject
- potom se teprve hleda AHRECV bridge a posila XEX
- pozdni direct fallback zustava, ale bez dalsiho blikani/reloadu

Log znacky:
- BUILD2SA5AJ EMU130_OPEN_ONCE
- BUILD2SA5AJ EMU130_INJECT_DELAYED_AFTER_OPEN
- BUILD2SA5AJ EMU130_WAIT_URL
- BUILD2SA5AH EMU130_INJECT_COMMIT
- BUILD2SA5AG EMU130_INJECT_SENT

Nezmeneno:
- Sega cesta zustava oddelena
- lokalni XEX z mobilu zustava beze zmen
- 130XE jadro/assets nejsou menene
- zadny BIOS/bin/rom payload
