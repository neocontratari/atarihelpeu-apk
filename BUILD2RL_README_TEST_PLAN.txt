AtariHelp.eu EMU-10 BUILD2RL
SEGA_NATIVE_CPP_ONLY_CLEAN_AUDIO_FRONTEND_STAGE128

CO TESTOVAT:
1) Workflow / GitHub Actions:
   - musi projit build a vytvorit APK.

2) Nox:
   - otevrit Sega Mega Drive,
   - CARTRIDGE / CART -> Sonic The Hedgehog (USA, Europe).gen,
   - pustit Sonic intro aspon 4 smycky,
   - hodnotit hlavne originalni cistotu zvuku: basy nesmi chrcet/prepalovat,
   - skok/cinknuti maji zustat ciste,
   - zkontrolovat levy kraj obrazu.

3) Mobil az po Nox:
   - mobil netestovat, dokud Nox nebude zvukove cisty.

CEKANY VYSLEDEK:
- nejde jen o stiseni: odstranena je bass-heavy low-pass smycka a underrun decay,
- FM stereo se uz nekolapsuje pred mixem do mono basoveho rumblu,
- PSG pouziva official CLOWNMDEMU_PSG_VOLUME_DIVISOR,
- vystup do Androidu je stereo AudioTrack 48 kHz,
- HIGH/LOW nemeni audio clock/gain/FIFO,
- levy prouzek framebufferu je maskovan tvrdsi ochranou.

LOG MARKERY:
- BUILD2RL_SEGA_NATIVE_CPP_ONLY_CLEAN_AUDIO_FRONTEND_STAGE128
- NATIVE_AUDIO_STREAM_START_RL_AUDIO_MASTER_FRONTEND_QT
- NATIVE_AUDIO_PULL_RL_AUDIO_MASTER_FRONTEND_QT
- FM_PSG_CLEAN_FRONTEND_RL
- stereo=YES noLowPass=YES dcBlock=YES
- fmGain=10 psgDivisor=core psgGain=100 masterGain=92
- desyncDrops=0
- NATIVE_PERF_MODE_OK_RL mode=LOW/HIGH target=4096 max=16384 lowWater=1536
- REAL_CORE_RENDER_OK_WORKER_THREAD_RL

KDY POSLAT LOG:
- hned pokud basy porad chrci,
- hned pokud se hudba zase zpomali,
- hned pokud workflow spadne.
