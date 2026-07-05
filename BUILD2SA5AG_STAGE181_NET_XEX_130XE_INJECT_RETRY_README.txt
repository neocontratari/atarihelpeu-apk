AtariHelp EMU10 BUILD2SA5AG / Stage181

Ucel:
- navazuje na Stage180 protected web relay restore
- ponechava nasi ochranu/provider relay pro AtariHelp web
- opravuje Atari XEX/ZIP z netu: po stazeni se payload nehrne do 130XE okamzite,
  ale ceka, dokud je 130XE WebView AHRECV bridge pripraveny

Co je zmeneno:
- net Atari download uklada payload do pending fronty
- po nacteni file:///android_asset/emu/index.html se opakovane overuje:
  AHRECV_BEGIN / AHRECV_PART / AHRECV_END
- teprve potom se XEX/ATR payload posle do 130XE
- kdyby WebView callback selhal, zustava fallback injekce po kratkem cekani
- log znacky:
  BUILD2SA5AG EMU130_QUEUE
  BUILD2SA5AG EMU130_INJECT_SCHEDULE
  BUILD2SA5AG EMU130_INJECT_WAIT / READY / FALLBACK
  BUILD2SA5AG EMU130_INJECT_SENT

Co zustava:
- lokalni XEX z telefonu jde stejnou cestou jako predtim
- Sega ZIP/GEN cesta zustava oddelena a nedotcena
- 130XE jadro/assets nejsou menene
- zadny BIOS neni pridany

Kontrola pred balenim:
- Java zavorky sedi
- ZIP neobsahuje BIOS/bin/rom payload
- overlay je uzky: MainActivity + predchozi PS1 overlay soubory ze Stage180
