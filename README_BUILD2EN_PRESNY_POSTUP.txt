ATARIHELP.EU EMU-10 BUILD2EN_APK_BASE_ALTIRRA_WAV_PCM_CLOAD

KODY JSOU STEJNE

DULEZITE:
- Zaklad index.html je vytazeny z APK, kterou poslal Rene jako posledni funkcni stav:
  app-debug(1).apk
- Ten stav umel boot/BASIC/tlacitka/CSAVE/WAV a Altirra jeho WAV nacetla.
- EK/EM rozbily boot; proto se NEPOUZIVAJI jako zaklad.
- BUILD2EN meni pouze app/src/main/assets/emu/index.html.

CO SE MENI:
- CLOAD pro vlastni Altirra-OK WAV nepouzije hlavni cestu decode WAV -> hotove SERIN bajty.
- Misto toho jde WAV PCM tvar primo na SKSTAT bit4 pres uz existujici jadrovou cestu.
- WAV export se nemeni, protoze Altirra ho nacetla.
- CSAVE, XEX, Java, ROM, UI a design se nemeni.

PRESNY POSTUP V TOTAL COMMANDERU:
1) Vlevo otevri projekt:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) Vpravo otevri ZIP:
   AtariHelp_eu_EMU10_BUILD2EN_APK_BASE_PCM_CLOAD_CMD.zip

3) V ZIPu otevri slozku:
   build2en_cmd_installer

4) Zkopiruj vlevo do korene projektu tyto 3 veci:
   SPUSTIT_BUILD2EN_APK_BASE_PCM_CLOAD.cmd
   _EMU10_BUILD2EN_PAYLOAD
   README_BUILD2EN_PRESNY_POSTUP.txt

5) Vlevo v koreni projektu spust:
   SPUSTIT_BUILD2EN_APK_BASE_PCM_CLOAD.cmd

6) GitHub Desktop:
   Summary: BUILD2EN apk base pcm cload
   Commit to main
   Push origin

7) WWW / GitHub Actions udela APK jako obvykle.

TEST:
- Po startu log musi ukazat:
  AtariHelp.eu EMU-10 BUILD2EN_APK_BASE_ALTIRRA_WAV_PCM_CLOAD pripraven.

- Po CSAVE ma zustat stejny WAV export jako v funkcni APK.

- Pri CLOAD/PLAY musi log ukazat:
  WAV PCM CLOAD BUILD2EN: ALTIRRA_OK WAV posilam primo jako PCM hrany na SKSTAT bit4

- V logu nesmi byt hlavni cesta:
  CLOAD SERIAL BUILD2BR: SERIN predano...
  HOLD_SERIN...

KDYZ TO SELZE:
Posli LOG. Dulezite je, jestli selze boot/tlacitka, nebo CLOAD.
