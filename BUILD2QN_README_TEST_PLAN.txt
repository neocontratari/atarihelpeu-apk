BUILD2QN_SEGA_NATIVE_CPP_PSG_SYNC_LIGHT_STAGE104

TEST:
1. Nasad overlay.
2. Nox: SEGA -> C++ CORE -> Sonic.
3. Testuj jen Sonic:
   - hudba
   - zvuk skoku
   - cinknuti prstynku
   - prodleva
   - chrceni
4. Dej ULOZENE a posli log.

CEKAM V LOGU:
- BUILD2QN_SEGA_NATIVE_CPP_PSG_SYNC_LIGHT_STAGE104
- NATIVE_AUDIO_STREAM_START_PSG_LIGHT
- audio_mode=FM_PSG_LIGHT_SYNC_QN
- psg_mixed=LIGHT
- pcm_seen_not_mixed=YES

STAV:
- Toto neni finalni zvuk.
- Je to rizene vraceni PSG efektu bez QL bordelu/PCM mixu.
