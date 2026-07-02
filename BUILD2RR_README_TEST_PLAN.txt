BUILD2RR_SEGA_NATIVE_CPP_ONLY_AUDIO_MASTER_NOSTARVE_STAGE134

KODY JSOU ZMENENE.

CIL:
- Navazat na RQ/RO cisty zvuk v Noxu.
- Nevracet basove/gong chyby.
- Opravit S8 hladove audio: prazdne FIFO, chrasteni, periodicke zpomalovani.
- Opravit levy barevne odskoceny pruh jako skutecny boundary/stride problem, ne crop/masku.

HLAVNI ZMENY:
1) Audio-master no-starve profil
   - Native runtime dostava Android SDK/model/cores z Javy.
   - Android 9 / S8 aktivuje s8NoStarve=YES.
   - S8 ma vetsi native reservoir: target=6144, max=32768, lowWater=3072.
   - Nox/A12 drzi cisty RQ profil: target=3072, max=16384, lowWater=1536.

2) Zadny hard trim jako RP
   - drops/desyncDrops nesmi rust jako v RP.
   - RQ/RR nepouziva masivni zahazovani vzorku.

3) No-starve catchup
   - Kdyz S8 audio FIFO hladovi, worker neceka na dalsi video tick.
   - Pri hladoveni se snizi native video capture, aby core mene kopiroval obraz a vice vyrabel YM/PSG audio.

4) Java AudioTrack S8 reservoir
   - S8/Android9 dostane vetsi AudioTrack buffer a delsi prefill.
   - Nox zustava mensi/stabilni.
   - Pri low got se na S8 kratce ceka/retry, aby se hned nepsalo ticho/praskani.

5) Levy pruh
   - Java crop 4px je zrusen.
   - C++ scanline uz neduplikuje prvni aktivni pixely do leveho borderu.
   - Aktivni scanline se posune k x=0, zbytek se cerni.

TEST PLAN:
A) Nox Sonic HIGH
- Ocekavani: zvuk zustane cisty jako RQ/RO, bez RP rozsekani.
- Levy pruh by mel zmizet nebo se zmenit na korektni cerny okraj bez odskoceni.

B) S8 Sonic HIGH
- Ocekavani: mene chrasteni, mene stop-loss v bonusu/Spring/Marble.
- Druha smycka intra by se nemela periodicky zhorsovat tak rychle.

LOG MARKERY:
- BUILD2RR_SEGA_NATIVE_CPP_ONLY_AUDIO_MASTER_NOSTARVE_STAGE134
- FM_PSG_ZEROED_AUDIO_MASTER_NOSTARVE_RR
- audioMasterClock=YES
- noStarve=YES
- s8NoStarve=YES/NO
- target=6144 max=32768 lowWater=3072 na S8
- target=3072 max=16384 lowWater=1536 na Nox
- starveEvents=
- catchupIterations=
- NATIVE_RUNTIME_CONFIG_RR
- NATIVE_AUDIO_STREAM_START_RR_AUDIO_MASTER_NOSTARVE_QT
- srcCrop=0px leftBoundaryShift=YES
