BUILD2PO_SEGA_MOBILE_VIEWPORT_PERF_GOVERNOR_NO_INPUT_CHANGE_STAGE79

CIL TESTU:
Overit, jestli problem neni jen zvuk, ale celkova WebView/GPU/CPU zatez v landscape. Build zachova PN audio prime diag a zmensuje landscape iframe overscale.

TEST:
1) Nasad ZIP overlay pres GitHub Desktop.
2) Build APK.
3) NOX: Sonic intro 20 s, pak skok/prstynky 30 s, ulozit LOG.
4) Mobil S8: stejne, ulozit LOG.
5) Mobil A12: stejne, ulozit LOG.

HLASENI ZPATKY:
- Nox: cisty / chrci / opozdeni / sync lepsi-stejny-horsi
- S8: ozvena / praskani / rychlost hry / sync
- A12: rychlost hry / zvuk / sync
- Landscape: jsou videt zivoty/HUD lepe? ano/ne
- Joystick: stejny / zmeneny

V LOGU HLEDAT:
- BUILD2PO_SEGA_MOBILE_VIEWPORT_PERF_GOVERNOR_NO_INPUT_CHANGE_STAGE79
- viewportPerfGovernor=YES
- noInputChange=YES
- AUDIO_SP_DIAG requested=2048 actual=2048 noPatch=YES
- DEVICE_INFO ua=... hc=... dpr=... screen=...
- PARENT_PERF_MONITOR ... avgGapMs/maxGapMs/slow45ms
- WRAPPER_PERF_MONITOR ... avgGapMs/maxGapMs/slow45ms

NESMI TAM BYT:
- 4096 samples vs expected 2048
- latencyHint=balanced
- mobileBalancedNative=YES
