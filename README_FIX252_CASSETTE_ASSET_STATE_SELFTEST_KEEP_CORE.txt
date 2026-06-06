FIX252_CASSETTE_ASSET_STATE_SELFTEST_KEEP_CORE

- Sdileny cassette state pro UI, snapshot a CLOAD.
- Vestaveny TurboBasicXXL.wav se nacita jako APK asset a snapshot musi ukazat inserted true po nacteni.
- WAV RIFF/PCM parser + pilot/raw analyza, bez fake LOAD a bez RAM injectu.
- CLOAD + PLAY jen posle CLOAD po real READY a zapne motor/play.
- SELF TEST cesta zachovana: ROM-bus DLIST + real RAM $3000, pridany diagnosticky DMA zoom z realnych RAM bitu.
- Zadny fake READY, zadny fake SELF TEST, zadne kreslene kostky.

KODY JSOU STEJNE.
