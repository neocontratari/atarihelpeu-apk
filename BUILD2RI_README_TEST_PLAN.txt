AtariHelp.eu EMU-10 BUILD2RI
BUILD2RI_SEGA_NATIVE_CPP_ONLY_FM_MUSIC_TIMING_BASS_CLAMP_STAGE125

CIL BUILD2RI:
- Nesahat na Sega skin, layout ani ovladac.
- Opravit smer podle testu BUILD2RH: video se drzi, ale hudba/FM se zpomaluje a basy chrasti.
- Zastavit agresivni FM/PSG desync dropy, ktere mohly zahazovat realne hudebni vzorky.
- Snizit FM/bass gain a pridat mekci bass clamp bez zmeny rychlosti hudby.
- Udrzet AudioTrack clock stabilnim plnym chunkem, ale bez RG core catch-up experimentu.
- Pri sceneStress setrit CPU pres vynechani jen frontend video copy, ne emulaci/audio.
- Opravit drobny spatny levy okraj herniho framebufferu pres C++ left-edge guard.

CO SE ZMENILO:
1) app/src/main/cpp/nap_sega_native_proof.cpp
   - BUILD2RI audio profil: FM_PSG_QT_BASS_CLAMP_MUSIC_TIMING_RI.
   - FM gain 44% -> 34%, PSG 16% -> 14%.
   - soft knee 24500 -> 21000, jemnejsi clamp /12.
   - vetsi real audio FIFO: HIGH target 3072/max 12288, LOW target 4096/max 16384.
   - odstraneny destruktivni FM-vs-PSG desync drop; desyncDrops by mel zustat 0.
   - sceneStress video guard: skipuje jen kopirovani framebufferu, emulace/audio bezi dal.
   - log prida videoCaptured/videoSkipped.
   - left-edge guard v scanline rendereru.

2) app/src/main/java/eu/atarihelp/emu10/MainActivity.java
   - BUILD2RI logy a nazvy souboru.
   - AudioTrack ma vetsi buffer/prefill.
   - AudioTrack dostava plny chunk kvuli stabilnimu clocku; native chunk obsahuje real data + kontrolovany decay jen pri real underrunu.
   - Dedup logu zustava.

3) app/src/main/java/eu/atarihelp/emu10/NativeSegaCoreBridge.java
   - BUILD2RI popis bridge.

4) app/src/main/assets/emu_sega/index.html
   - BUILD2RI labely, SBIRKA LOW/HIGH zustava, CARTRIDGE zustava ROM picker.

TEST PLAN:
1. Workflow / GitHub Actions:
   - Ocekavani: build projde a vznikne jedna APK.

2. Nox:
   - Sonic intro aspon 4 smycky.
   - Sleduj: basy, treti smycka, levý kraj obrazu.
   - Ocekavani: basy min chrasti, video zustava plynule, zadny C++ CORE flek.

3. Mobil S8 HIGH:
   - SBIRKA -> HIGH QUALITY -> CARTRIDGE -> Sonic.
   - Test aspon 4 smycky intra.
   - Sleduj: hudba, cinknuti/skok, video, treti smycka.

4. Mobil S8 LOW:
   - SBIRKA -> LOW PERFORMANCE -> Sonic.
   - Test aspon 4 smycky intra.
   - Sleduj: jestli hudba uz nezpomaluje hned od zacatku a jestli video jen pripadne vynecha frame misto zpomaleni hudby.

LOG MARKERY:
BUILD2RI_SEGA_NATIVE_CPP_ONLY_FM_MUSIC_TIMING_BASS_CLAMP_STAGE125
NATIVE_AUDIO_STREAM_START_RI_AUDIO_MUSIC_TIMING_QT
NATIVE_AUDIO_PULL_RI_AUDIO_MUSIC_TIMING_QT
NATIVE_PERF_MODE_OK_RI mode=LOW/HIGH
FM_PSG_QT_BASS_CLAMP_MUSIC_TIMING_RI
REAL_CORE_RENDER_OK_WORKER_THREAD_RI ... audioBacklog= coreAvgMs= sceneStress= videoCaptured= videoSkipped=
desyncDrops=0
SAVE_LOG_DEDUP_RI
