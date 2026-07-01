BUILD2RO_SEGA_NATIVE_CPP_ONLY_ZEROED_REFERENCE_AUDIO_STAGE131

Ucel:
- Opravuje realnou pricinu bass/gong/two-musics artefaktu ve ClownMDEmu audio frontendu.
- ClownMDEmu FM_OutputSamples() a PSG_Update() zapisují do sample_buffer pres +=.
- Predchozi BUILD2RL/RM/RN pouzival thread_local scratch buffer bez vycisteni, takze se stare vzorky michaly do novych.
- BUILD2RO proto vzdy nuluje FM/PSG/PCM/CDDA scratch buffer pred volanim core generatoru.

KODY JSOU ZMENENE.

Dulezite zmeny:
- zeroInputBuffers=YES
- coreLPF=ON
- noRumbleFilter=YES
- no fake bass clamp / no rumble blocker
- fmGain=100 psgGain=100 masterGain=90
- tvrdsi pre-start hard stop AudioTrack, aby nebezely dva audio streamy najednou

Test:
1) Workflow musi projit a vytvorit APK.
2) Testuj pouze Nox + Sonic intro.
3) Poslouchej zacatek: nesmi tam byt gong/thump.
4) Poslouchej basy: nemaji byt dvojite ani prebuzeny bordel.
5) Pokud je zvuk stale spatny, poslat Sega log + C++ in-place log.

Log markers:
- BUILD2RO_SEGA_NATIVE_CPP_ONLY_ZEROED_REFERENCE_AUDIO_STAGE131
- FM_PSG_ZEROED_FRONTEND_RO
- zeroInputBuffers=YES
- coreLPF=ON
- noRumbleFilter=YES
- NATIVE_AUDIO_STREAM_PRESTART_HARD_STOP_RO_ZEROED_REFERENCE_AUDIO_QT
- NATIVE_AUDIO_STREAM_START_RO_ZEROED_REFERENCE_AUDIO_QT
- NATIVE_AUDIO_PULL_RO_ZEROED_REFERENCE_AUDIO_QT
