AtariHelp.eu EMU-10 BUILD2RJ
SEGA_NATIVE_CPP_ONLY_AUDIO_MASTER_FRONTEND_STAGE126

CO TESTOVAT:
1) Workflow / GitHub Actions:
   - musi projit build a vytvorit APK.

2) Nox:
   - otevrit Sega Mega Drive,
   - CARTRIDGE / CART -> Sonic The Hedgehog (USA, Europe).gen,
   - pustit Sonic intro aspon 4 smycky,
   - poslouchat basy, hudbu, skok/cinknuti,
   - zkontrolovat levy kraj obrazu.

3) Mobil S8 HIGH:
   - SBIRKA -> HIGH QUALITY,
   - CARTRIDGE -> Sonic,
   - nechat intro 4 smycky,
   - hodnotit hlavne hudbu a basy.

4) Mobil S8 LOW:
   - SBIRKA -> LOW PERFORMANCE,
   - Sonic 4 smycky intra,
   - hudba musi mit stejnou rychlost jako HIGH.
   - LOW muze mit mene plynule video, ale nesmi zpomalovat hudbu.

CEKANY VYSLEDEK:
- basy maji byt mnohem tissi/cistsi nez BUILD2RI,
- Nox by nemel zpomalit hudbu,
- LOW/HIGH nesmi menit rychlost hudby,
- video LOW/HIGH se muze lisit jen prezentaci obrazu,
- cinknuti a skok musi zustat ciste,
- levý prouzek by mel byt lepe maskovany.

LOG MARKERY:
- BUILD2RJ_SEGA_NATIVE_CPP_ONLY_AUDIO_MASTER_FRONTEND_STAGE126
- NATIVE_AUDIO_STREAM_START_RJ_AUDIO_MASTER_FRONTEND_QT
- NATIVE_AUDIO_PULL_RJ_AUDIO_MASTER_FRONTEND_QT
- FM_PSG_MASTER_FRONTEND_RJ
- fixedAudioClock=YES
- fmGain=16 psgGain=12 masterGain=85
- desyncDrops=0
- NATIVE_PERF_MODE_OK_RJ mode=LOW/HIGH target=4096 max=16384 lowWater=1536
- REAL_CORE_RENDER_OK_WORKER_THREAD_RJ

KDY POSLAT LOG:
- hned jak se hudba zpomali,
- hned pokud jsou basy porad moc,
- hned pokud LOW hraje pomaleji nez HIGH.
