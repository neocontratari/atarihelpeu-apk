BUILD2RV_SEGA_NATIVE_CPP_ONLY_RR_RECOVERY_SAFE_AUDIT_STAGE138

Ucel:
- OPRAVNY recovery build po rozbitych RT/RU auditech.
- Zaklad je posledni hratelna RR vetev.
- RT/RU zmeny jsou zamerne vypustene: zadne vynucene JUE->PAL, zadny fixed-ring experiment, zadny modry fallback/audit monitor po ROM loadu.
- Cilem je vratit realny gameplay/frame/audio a pritom nechat jen bezpecny pasivni audit v logu.

Co je zmenene:
- Build label BUILD2RV.
- Audio mode marker: FM_PSG_ZEROED_RR_RECOVERY_SAFE_AUDIT_RV.
- JUE ROM zustava bezpecne NTSC jako v hratelne RR vetvi.
- E-only ROM zustava PAL podle puvodni RR detekce, ale bez RT/RU frame-clock/ring/visible-frame zmen.
- Zachovana C++ only cesta, Java wrapper zustava vypnuty.
- Zachovana RR audio cesta: zeroed FM/PSG buffery, no hard trim, no RP tiny buffer, no RT/RU fixed-ring experiment.

Co se nema v tomto buildu objevit:
- BUILD2RT nebo BUILD2RU markery.
- FM_PSG_REGION_MEMORY_AUDIT_RT.
- FM_PSG_REGION_FIX_VISIBLE_FRAME_RU.
- audioRingAudit=FIXED_RING_NO_DEQUE.
- JUE_APP_DEFAULT_EU.
- modry auditni monitor misto hry po ROM loadu.

Test plan:
1) Nox - Sonic JUE / USA-Europe:
   - CARTRIDGE -> vybrat ROM.
   - Ocekavani: realna hra, ne modry monitor.
   - Zvuk ma byt jako RR: hratelny/cisty, ale muze mit RR zpozdeni.

2) Nox - EU-only ROM:
   - CARTRIDGE -> vybrat Aladdin Europe nebo jinou E-only.
   - Ocekavani: nesmi zustat modry monitor.
   - Pokud EU-only nejede, poslat log; je to samostatny PAL/core problem, ne RT/RU audit guard.

3) S8:
   - Az po overeni Nox.
   - Testovat USA ROM a EU ROM zvlast.

Log markery:
- BUILD2RV_SEGA_NATIVE_CPP_ONLY_RR_RECOVERY_SAFE_AUDIT_STAGE138
- FM_PSG_ZEROED_RR_RECOVERY_SAFE_AUDIT_RV
- REAL_CORE_WORKER_ALIVE=YES
- frameReady=YES
- videoCaptured>0
- nativeActiveAudioTracks=1 po startu audia
- nativeRegionAuto=US_NTSC_OVERSEAS header=JUE pro JUE ROM
- nativeRegionAuto=EU_PAL_OVERSEAS header=E pro E-only ROM

Kriticke:
- Toto neni dalsi experiment s regionem. Je to recovery hotfix na posledni hratelny zaklad.
