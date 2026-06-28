AtariHelp.eu EMU-10 BUILD2MT_SEGA_LRUSSO_LIVE_IFRAME_FALLBACK_STAGE7

CIL:
Dostat prvni realny graficky Mega Drive vystup bez fake videa. Kdyz prime embedGenesis API stale neni dostupne v Android WebView, BUILD2MT otevre v monitoru realny live lrusso Genesis emulator jako iframe fallback.

TEST:
1. Rozbal ZIP pres koren projektu.
2. Buildni/spust APK.
3. Otevri SEGA modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic nebo Aladdin jako doted.
5. Ocekavani A:
   - prime embedGenesis se chyti, v logu bude embedGenesis CALLBACK STARTED a hra / grafika bezi primo v nasi obrazovce.
6. Ocekavani B, pravdepodobnejsi podle BUILD2MS:
   - uvidis kratce graficky loading/error screen.
   - pote se v SEGA monitoru otevira LIVE real-core fallback iframe.
   - klikni cerveny PLAY uvnitr monitoru.
   - ma se nacist realne demo z lrusso Genesis emulatoru.
7. Dulezite: tohle neni jeste finalni integrace Sonic ROM. Lokalni ROM z appky se do externiho cross-origin iframe nepreda automaticky.
8. Po testu klikni ULOZENE a posli log.

CO POSLAT ZPET:
- screenshot monitoru po kliknuti PLAY
- LOG z ULOZENE
- jestli iframe vubec naskocil
- jestli po PLAY vidis realnou grafiku / demo / cernou obrazovku / chybu

OCEKAVANE LOG MARKERY:
- LIVE IFRAME FALLBACK READY
- LIVE IFRAME FALLBACK LOAD FIRED
- loadRom FALLBACK MODE LIVE_IFRAME_READY
- pripadne predtim ONLINE_SCRIPT_LOADED_BUT_EMBEDGENESIS_MISSING

KODY JSOU ZMENENE.
ZADNY FAKE SONIC. ZADNY ROM V ZIPU. ATARI/VBXE SE NEMENI.
