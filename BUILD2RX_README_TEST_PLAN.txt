BUILD2RX_SEGA_NATIVE_CPP_ONLY_PCM_SKIP_THERMAL_AUDIT_RECT_FIX_STAGE140

=========================================================
CO DOKAZALY TVOJE RW LOGY (fakta, ne dojmy):
=========================================================
1) PAMET JE CISTA. Java heap 8.9 -> 12.8 MB za 4 minuty (pomaly, normalni rust logu),
   GC count porad 3, GC time porad 110 ms, nativni heap plochy ~26 MB.
   => Zadne bobtnani, zadny leak, zadny GC problem. Tahle hypoteza je MRTVA.

2) JAVA AUDIO TRUBKA JE CISTA. audioTrackUnderruns=0 po celou dobu.
   => Chrceni nevznika mezi Javou a reproduktorem.

3) CHRCENI = NATIVNI FIFO HLADOVI. Ayrton (header JE => bezel PAL 50Hz):
   - prvni minuta: latence 60-80 ms, underruns=0 -> zvuk OK (sedi s tvym pozorovanim)
   - tick 7 (~70 s): prvni underruny 2056 -> zacatek sekani (sedi!)
   - tick 13-23: latence spadne na 12-31 ms, underruny 6.5k -> 691k
   => C++ jadro prestava stihat vyrabet sample. Ne pamet, ne render (ten je porad 2-3 ms).

4) ALADDIN "USA Final Cut": coreAvgMs=23.7 ms, ale NTSC frame budget je 16.67 ms.
   sceneStress=YES, hladovi od prvni vteriny (~950k underruns/10 s).
   POZOR: tahle ROM je 2.4 MB REMIX/hack (checksum NESEDI, "patched copy").
   Je vyrazne tezsi nez originalni Aladdin.

5) "Po prepnuti hry zpomaleny zvuk porad": generace jsou ciste (audioGen spravne rostl,
   activeAudioTracks=1, zadna stara instance). Neni to zaseknuty stav appky.
   Hlavni podezreni: TELEFON JE UZ ROZEHRATY = thermal throttling Exynosu
   (klasika S8: po ~minute plne zataze CPU podtaktuje; catchup smycka jede naplno
   a ohrivani jeste urychluje). RX to zmeri natvrdo.

6) AYRTON NENI USA ROM. Header je JE (Japan/Europe) => bezi jako PAL 50 Hz.
   Tzn. tvoje drivejsi "EU chrci / USA jede" muze byt ve skutecnosti
   "tezka hra chrci / lehka jede" nebo "studeny telefon jede / horky chrci".
   RX audit to rozsekne.

7) LEVY ODSKOK V NOXU: C++ audit dokazal, ze framebuffer je OK
   (leftBoundary=0, levy sloupec je herni obsah, zadny barevny pruh ve frame).
   Chyba byla v umisteni videa: #monitorBlank je display:none => meril se jako 0x0
   => VZDY se pouzil nouzovy prepocet z okna, ktery stage spocital jinak, nez je
   realne vykresleny (Nox: fallback left=35 vs realny stage left=29 => ~6 px odskok).

=========================================================
CO BUILD2RX MENI (3 cilene zmeny, kazda podlozena logem):
=========================================================
A) C++ CPU USPORA: PCM a CDDA generatory se uz NEVOLAJI.
   CD je vypnute a jejich vystup se stejne zahazoval (jen se pocital citac).
   RW log: pcm_seen ~766k samplu za 10 s = platili jsme CPU za ticho.
   FM/PSG/region/FIFO/clock/gain NEDOTCENO.
   Marker: pcmCddaGen=SKIPPED_NO_CD.

B) HTML LEVY ODSKOK FIX: portrait rect se meri z REALNEHO #stage elementu
   a aplikuji se na nej stejna procenta jako v CSS monitoru (6.42/17.16/87.28/29.60).
   Stary fallback zustava jen kdyz stage nejde zmerit.
   Marker: NATIVE_RECT_STAGE_PERCENT ... leftStripFix=YES.

C) THERMAL AUDIT: PASSIVE_AUDIT_RX radek nove obsahuje:
   batteryTempC, cpu0KHz, cpu4KHz (velke jadro Exynosu),
   a explicitne coreAvgMs/coreMaxMs/sceneStress/audioBacklog
   (v RW je orizlo 1400 znaku limitu - proto je ted tahame zvlast na zacatek radku).

ZMENENE SOUBORY:
- app/src/main/cpp/nap_sega_native_proof.cpp
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/assets/emu_sega/index.html
(NativeSegaCoreBridge.java beze zmeny, prilozen pro jistotu)

=========================================================
TEST PLAN (po lopate):
=========================================================
1) NOX - LEVY ODSKOK (nejrychlejsi overeni):
   - Spustit Sonic JUE.
   - V logu MUSI byt NATIVE_RECT_STAGE_PERCENT (ne FALLBACK_MONITOR).
   - Podivat se, jestli odskok vlevo zmizel. Screenshot + SAVE LOG.

2) NOX - ZVUK/HRA: Sonic musi hrat minimalne stejne jako RW. Kdyby PCM skip
   cokoli rozbil (chybejici zvukovy efekt, jina hudba), OKAMZITE hlas - vratime to.
   (Necekam to: Sonic DAC je ve FM, CD neni pripojene.)

3) S8 - STUDENY TELEFON TEST (dulezite kvuli thermal teorii):
   - Telefon nechat 10 minut vypnuty/odlozeny, at je studeny.
   - Spustit Ayrton Senna, hrat/nechat bezet a sledovat:
     v kolikate minute zacne sekani (RW: ~1:10).
   - 4-5 minut staci, pak SAVE LOG.
   - V logu budou batteryTempC a cpu4KHz: kdyz teplota poroste a cpu4KHz
     v okamziku sekani SPADNE, mame thermal throttling cerne na bilem.
4) S8 - ORIGINAL ALADDIN: pokud mas, zkus original Aladdin (USA) misto
   "Final Cut" remixu (ten ma rozbity checksum a je o dost tezsi).
   Zajima me coreAvgMs originalu.

LOG MARKERY:
- BUILD2RX_SEGA_NATIVE_CPP_ONLY_PCM_SKIP_THERMAL_AUDIT_RECT_FIX_STAGE140
- audio_mode=FM_PSG_ZEROED_RW_AUDIT_PCM_CDDA_SKIP_RX pcmCddaGen=SKIPPED_NO_CD
- PASSIVE_AUDIT_RX tick=N ... batteryTempC=... cpu4KHz=... coreAvgMs=...
- NATIVE_RECT_STAGE_PERCENT ... leftStripFix=YES
- frameReady=YES, nativeActiveAudioTracks=1 (stejne jako RV/RW)

CO SE NESMI OBJEVIT:
- NATIVE_RECT_FALLBACK_MONITOR v Noxu (jen kdyby stage nesel zmerit)
- RT/RU markery, modry monitor po ROM loadu

OCEKAVANI (bez lakovani):
- Levy odskok: mel by zmizet uplne.
- PCM skip: da S8 trochu CPU vzduchu navic - muze oddalit zacatek sekani,
  ale SAM O SOBE tezke hry na horkem S8 nezachrani.
- Kdyz RX potvrdi thermal throttling, dalsi krok RY bude cileny:
  omezit spal CPU v catchup smycce + S8 profil, ktery obetuje latenci/video
  za stabilni zvuk. To je zmena governoru, tu delame az s dukazem v ruce.
