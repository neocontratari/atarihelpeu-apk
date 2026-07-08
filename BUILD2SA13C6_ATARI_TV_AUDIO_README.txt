BUILD2SA13C6_ATARI_TV_WEB_AUDIO

Co je nove:
- TV WEB CAST zustava: MJPEG/PixelCopy obraz + WebAudio zvuk.
- PS1 a Sega zvuk zustavaji pres Java/Native PCM cestu.
- Atari VBXE/130XE POKEY/GTIA zvuk je napojen z WebAudio callbacku:
  emu_vbxe/index.html posila mono PCM16 pres AHTVWEB.pushAtariPcm16().
- MainActivity prijme Atari mono PCM16, zdvoji ho do stereo ringu a TV browser
  ho prehrava pres /audio.raw stejne jako PS1/Sega.

Markery/log:
- BUILD2SA13C TV_WEB_AUDIO_SOURCE ATARI
- /status ukazuje audioSource=ATARI

Test:
1. V appce zapni TV WEB CAST.
2. Na TV/PC otevri URL a potvrdit AUDIO OK.
3. Spust Atari/VBXE hru se zvukem.
4. Na TV by mel hrat Atari POKEY/GTIA zvuk.

Poznamka:
- Tohle resi POKEY/GTIA emulacni zvuk z Atari WebAudio.
- Samostatny kazetovy WAV/MP3 monitor pres HTML audio element muze byt jina cesta;
  pokud by nesel, posli konkretni test a log.
- Kompilace APK tady nebyla spustena: repo nema gradlew a systemovy gradle neni dostupny.
