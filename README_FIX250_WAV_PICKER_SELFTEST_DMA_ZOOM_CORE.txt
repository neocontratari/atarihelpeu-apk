AtariHelp.eu EMU-09 FIX250_WAV_PICKER_SELFTEST_DMA_ZOOM_CORE

CIL:
- Opravit Android/Nox file picker pro WAV/CAS: accept all files + WAV/CAS/audio/octet-stream.
- Po vybrani souboru precist hlavicku pres FileReader a zalogovat prvnich 16 bytu.
- Zatim bez fake LOAD: zadne plneni RAM z WAV/CAS.
- Dalsi krok self-test grafiky: DMA zoom/strip z real RAM $3000 pri ROM-bus DLIST $51D1.

KODY JSOU STEJNE.
