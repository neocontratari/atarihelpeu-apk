AtariHelp.eu EMU-10 BUILD2RH
SEGA_NATIVE_CPP_ONLY_AUDIO_ROLLBACK_SCENE_STRESS_STAGE124

CIL BUILD2RH:
- Vraci spatky zvukove chovani pred BUILD2RG, protoze RG zhorsil basy/chrceni.
- Nelepi grafiku ani ovladac: Sega UI a landscape layout zustava z RF/RE/RG stavu.
- Rusi RG audio-first catch-up, ktery mohl menit rytmus Sonica podle scena/zateze.
- Pridava scene-stress diagnostiku: coreLastMs/coreAvgMs/coreMaxMs/sceneStress/audioBacklog.
- LOW/HIGH ponechava jako UI/render/performance volbu, ale uz nemeni bass/mix ani audio FIFO charakter.
- LOG dedup zustava: jedno zmacknuti nema vytvorit 4 logy.

HLAVNI ZMENY:
1) Java AudioTrack:
   - RG zapisoval plny chunk i pri malem got -> chrcici basy.
   - RH vraci RF-style zapis: kdyz native vrati maly pocet vzorku, nezapisuje se nucene plny chunk.
   - Logy: NATIVE_AUDIO_STREAM_START_RH_AUDIO_ROLLBACK_QT, NATIVE_AUDIO_PULL_RH_AUDIO_ROLLBACK_QT.

2) Native C++ audio:
   - FIFO velikosti zpet na RF/QP: target 2048, max 4096, desync 768.
   - LOW/HIGH uz nemeni audio headroom, aby se nemenil zvuk mezi rezimy.

3) Core timing:
   - Rusi RG extra frame catch-up.
   - Core bezi jeden MD frame na jeden clock tick.
   - Pokud mobil nestiha, clock se resynchronizuje jako RF, ale nedela extra sprinty.

4) Diagnostika sceny:
   - realCoreStatus / native status pridava:
     coreLastMs, coreAvgMs, coreMaxMs, sceneStress=YES/NO, audioBacklog, perfLow.
   - Tim pujde overit, jestli Sonic zpomaluje v narocne scene/pozadi, nebo kvuli audio callbacku.

TEST PLAN:
1. Workflow: musi projit a vygenerovat jednu APK.
2. Nox: Sonic -> Atari 130XE -> zpet Sega -> Sonic/Aladdin.
3. S8 HIGH: SBIRKA -> HIGH -> CARTRIDGE -> Sonic 5 minut.
4. S8 LOW: SBIRKA -> LOW -> Sonic 10-15 minut.
5. Sleduj hlavne treti smycku intra / misto, kde se pravidelne zpomaluje.
6. Pri zpomaleni uloz log hned v ten moment.

V LOGU HLEDEJ:
BUILD2RH_SEGA_NATIVE_CPP_ONLY_AUDIO_ROLLBACK_SCENE_STRESS_STAGE124
NATIVE_AUDIO_STREAM_START_RH_AUDIO_ROLLBACK_QT
NATIVE_AUDIO_PULL_RH_AUDIO_ROLLBACK_QT
NATIVE_PERF_MODE_OK_RH mode=LOW/HIGH
REAL_CORE_RENDER_OK_WORKER_THREAD_RH ... coreLastMs= coreAvgMs= coreMaxMs= sceneStress= audioBacklog=
NATIVE_RENDER_PERF_RH
SAVE_LOG_DEDUP_RH / BUILD2RH LOG_SAVE_DEDUP jen pri rychlem opakovanem tapnuti

POZNAMKA:
Tohle neni final. Je to cilene odpojeni spatne RG audio vetve a diagnostika, jestli hlavni problem lezi ve scena/VDP zatezi core, nebo v audio callbacku.
