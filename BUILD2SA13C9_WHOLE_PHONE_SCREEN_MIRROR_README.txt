BUILD2SA13C9 - TV WEB CAST WHOLE-PHONE SCREEN MIRROR

Co je nove:
- TV WEB CAST po spusteni pozada Android o povoleni sdileni cele obrazovky pres MediaProjection.
- Po odsouhlaseni systemoveho dialogu se MJPEG obraz bere z celeho displeje telefonu, ne jen z okna AtariHelp.
- To znamena, ze TV WEB CAST muze videt systemovy file picker, externi browser i YouTube obraz.
- /status ukazuje mirror=SCREEN, kdyz bezi whole-phone rezim. Kdyz povoleni neni aktivni, ukazuje mirror=APP a jede stary app-only capture.
- Log markery:
  BUILD2SA13C9 SCREEN_MIRROR_PERMISSION_REQUEST
  BUILD2SA13C9 SCREEN_MIRROR_ON ...
  BUILD2SA13C9 SCREEN_MIRROR_OFF ...

Zvuk:
- Zvuk emulatoru, Atari WebAudio a MP3/WAV playeru jde porad vlastnim TV WEB audio kanalem.
- MP3/WAV player posila do TV zvuk az za EQ/bass/treble/balance/volume.
- YouTube/cizi aplikace na Samsungu S8 / Android 9 pravdepodobne nepujde odchytit do naseho web audio kanalu. Android audio capture pro cizi aplikace prisel az pozdeji a i pak zalezi na aplikaci.

Test:
1. Spust TV WEB CAST.
2. Telefon ukaze systemove potvrzeni sdileni obrazovky. Dej Spustit / Start now.
3. Na TV otevri URL z telefonu.
4. Otevri YouTube tlacitkem v appce nebo rucne.
5. /status ma ukazat mirror=SCREEN a TV ma videt i YouTube obraz.

Kdyz je obraz cerna plocha:
- Nektere DRM/secure obrazovky Android cernaji schvalne. Normalni YouTube UI a bezna videa by ale mela byt videt.
- Poslat log a /status.
