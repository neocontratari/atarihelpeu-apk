BUILD2SA13C7 - MP3/WAV PLAYER + TV WEB CAST AUDIO

Co je nove:
- MP3/WAV player pouziva Android picker pres AHPICK.pickMp3().
- Vybrane MP3/WAV se do playlistu posilaji jako content:// stream, ne jako cele base64 soubory v pameti.
- Player ma prijimaci JS funkce AHLOCAL_MP3_URI / AHLOCAL_MP3_PLAYLIST_BEGIN / AHLOCAL_MP3_PLAYLIST_END.
- WebAudio vystup playeru se posila do TV WEB CAST audio ring bufferu jako stereo PCM16 se zdrojem PLAYER.
- TV status/log ma ukazovat BUILD2SA13C TV_WEB_AUDIO_SOURCE PLAYER pri prehravani MP3/WAV.
- YouTube tlacitko uz neni HTML navigacni odkaz; vola Android externi otevreni pres AHPICK.openExternalUrl().

Poznamka k YouTube:
YouTube neni stejne jako nase TV WEB CAST zrcadleni. Oficiální YouTube cast pousti video primo v TV aplikaci YouTube.
Nase TV WEB CAST umi vlastni obraz apky + nase audio kanaly z emulatoru a MP3/WAV playeru. Externi YouTube app/browser se timhle vlastnim kanalem neprenasi.

Test:
1. Zapni TV WEB CAST a na TV otevri URL z telefonu.
2. V playeru otevri MY PLAYLISTS -> PRIDAT MP3 / WAV.
3. Vyber jednu MP3 nebo WAV.
4. Spust PLAY.
5. Na TV by mel jit obraz playeru a zvuk; /status nebo log ma mit audioSource=PLAYER.

Kdyz MP3 po vyberu nebude hrat:
- poslat log z apky,
- napsat, jestli je soubor z interní pameti, SD karty, Google Drive nebo jine cloudove aplikace,
- napsat, jestli playlist ukazal nazev skladby po vyberu.
