BUILD2RP_SEGA_NATIVE_CPP_ONLY_LOW_LATENCY_SINGLE_AUDIO_STAGE132

Ucel buildu:
- Zachovat prvni cisty Sonic zvuk z BUILD2RO/clean baseline: nulovani FM/PSG scratch bufferu pred ClownMDEmu += mixem.
- Opravit zpozdeni cinknuti/skoku z RO: zmenseny native FIFO target/max, mensi AudioTrack chunk a mensi prefill.
- Prokazat jedinou audio cestu: singleAudioPath=YES, activeAudioTracks=1/0, oldAudioKilled=YES.
- S8: best-effort vyssi priorita native workeru/audio threadu, bez zmeny zvukove barvy.
- Levy prouzek: real source crop 4px ve TextureView, bez barevneho prebarvovani prouzkem.

Test plan:
1) GitHub workflow musi projit a vzniknout APK.
2) Nox: Sonic intro 4 smycky. Kontrola: cisty zvuk zustal, cink/skok uz nema cca 1s zpozdeni, levy okraj nema odskoceny prouzek.
3) S8 HIGH: pouze po Noxu. Sonic intro 3-4 smycky, poslat log pri prvnim zpomaleni/praskani.

Log markery:
- BUILD2RP_SEGA_NATIVE_CPP_ONLY_LOW_LATENCY_SINGLE_AUDIO_STAGE132
- FM_PSG_ZEROED_LOW_LATENCY_RP
- zeroInputBuffers=YES
- lowLatency=YES
- singleAudioPath=YES
- latencyFrames= / latencyMs=
- activeAudioTracks=
- NATIVE_AUDIO_STREAM_START_RP_LOW_LATENCY_SINGLE_AUDIO_QT
- NATIVE_AUDIO_STREAM_STOP_REQUEST_RP_LOW_LATENCY_SINGLE_AUDIO_QT
- srcCrop=4px
