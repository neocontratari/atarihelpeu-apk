AtariHelp.eu EMU-10 BUILD2QK_SEGA_NATIVE_CPP_REGION_AUDIO_FIRST_STAGE101

KODY JSOU ZMENENE: ANO
Navazuje na BUILD2QJ / BUILD2PV+ vendor ClownMDEmu-core offline stav.

CIL:
1) Opravit EU/PAL hry v nativnim C++ core: Aladdin Europe, Sega Sports Europe, Ayrton Senna Japan/Europe.
2) Pridat prvni skutecnou native audio cestu z ClownMDEmu FM callbacku do Android AudioTrack.
3) Zachovat normalni Sega UI, monitor, joystick, tlacitka, cartridge picker.

ZMENY:
- C++ core automaticky nastavi hardware region podle ROM headeru:
  * E / JE bez U => OVERSEAS PAL
  * J only => DOMESTIC NTSC
  * U / JUE / default => OVERSEAS NTSC
- To ma odstranit hlasku u EU her typu PAL/French hardware only.
- Pridan C++ FM audio FIFO a Java AudioTrack stream 48000 Hz mono.
- Audio je oznacene jako FIRST FM AUDIO: PSG/mix jeste neni finalni.
- Zadne behajici kostky, zadne dalsi C++ okno.

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Nech projit GitHub Actions build.
3. Otevri SEGA -> C++ CORE.
4. Testuj postupne:
   - Sonic The Hedgehog (USA, Europe).gen
   - Aladdin (Europe).gen
   - Sega Sports 1 (Europe).gen
   - Ayrton Senna's Super Monaco GP II (Japan, Europe).gen
5. U kazde hry sleduj:
   - obraz nabehne / region hlaska / black screen
   - zvuk slyset ano/ne, chrci/nechrci
   - joystick reaguje ano/ne
6. Dej ULOZENE a posli log.

LOG MARKERY:
BUILD2QK_SEGA_NATIVE_CPP_REGION_AUDIO_FIRST_STAGE101
nativeRegionAuto=EU_PAL_OVERSEAS / US_NTSC_OVERSEAS / JP_NTSC_DOMESTIC
NATIVE_AUDIO_STREAM_START
NATIVE_AUDIO_PULL got=...
audio_fifo=... pushed=... pulls=...
REAL_CORE_RENDER_OK_WORKER_THREAD

POZOR:
- Pokud build spadne, poslat prvni cerveny compiler error.
- Pokud appka spadne po vyberu ROM, napsat u jake ROM a v jakem okamziku.
