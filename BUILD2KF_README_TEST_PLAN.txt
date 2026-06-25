AtariHelp.eu EMU-10 BUILD2KF_DOC_GUIDED_NIGHTDRIVER_AUDIO_STRIPE_PASS
================================================

KODY JSOU STEJNE
BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

ZAKLAD
- Navazuje na BUILD2KE/KB/JZ/JX smer.
- NEvychazi z KC/KD mikrovetvi.
- KD zasah do Decathlon PMBASE=$00 zustava pryc.
- Chranene body: Commando Arnold ANO + hra ANO, Mission nabehne, Tetris VBXE, HL/Heartlight.

PROC TENHLE BUILD
Rene poslal dokumentaci Atari130XE_GTIA_VBXE_Java_Dokumentace.zip.
Z dokumentace je dulezite hlavne:
- VBXE je overlay nad GTIA/ANTIC obrazem, ne nahrada GTIA.
- SR overlay: 1 byte = 1 pixel, 256 barev.
- Transparentnost ridi VIDEO_CONTROL: index 0 transparent, volitelne xF pri trans15.
- XDL recordy se maji kreslit podle obsahu VRAM a priority, ne podle nazvu hry.

ZMENA 1 - NIGHT DRIVER VODOROVNY CAJ U AUTA
BUILD2KE pevne preskocil pouze zacatek nizke SR oblasti. To zlepsilo horizont, ale pruh u auta zustal.
BUILD2KF prida obsahovy SR stripe filtr:
- pouziva jen podezrele nizke SR/XDL recordy ve spodnim obraze,
- meri radku podle SR transparentnosti,
- vynecha jen izolovanou vodorovnou artefakt-radku,
- nerusi cele auto a neni to screen-paint ani hack obrazku.

Log marker:
VBXE SR STRIPE FILTER BUILD2KF

ZMENA 2 - NIGHT DRIVER ZVUK
Rene dodatecne upozornil, ze Night Driver nema celou dobu zvuk.
BUILD2KF pridava robustni AudioContext resume:
- pokud Android WebView/Nox po pickeru nebo pauze drzi audio context suspended, startAudio ho znovu resume,
- je to obecny POKEY/GTIA audio fix, ne hack hry.

TEST MINIMUM
1) Night Driver:
   - auto ano/ne
   - caj u auta pryc/mensi/stejny
   - zvuk ano/ne

2) Commando:
   - jen rychla kontrola: Arnold ANO + hra ANO

3) Mission:
   - jen rychla kontrola: nabehne ano/ne, grafika stejna/lepsi/horsi

4) Decathlon:
   - pouze kontrola regrese. Tento build ho naslepo neopravuje.
   - pokud je stejny jako KE/KB, je to ocekavane.

CO POSLAT ZPET
- Slovne vysledek Night Driveru.
- LOG / CHYBA hlavne pokud:
  - zmizel Arnold,
  - Mission nenabehne,
  - Night Driver je horsi,
  - Decathlon se zhorsil.
