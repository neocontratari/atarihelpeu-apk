FIX216_ROM_AUDIT_SELFTEST_GATE_CORE

KODY JSOU STEJNE. SUBMARINE BASIC ani GTIA 9/10/11 BASIC kody nejsou zmenene.

Duvod: FIX215 ukazal, ze ROM se opravdu cetla, ale BASIC boot byl rozbity tim, ze emulator omylem pouzil PORTB bit7 jako self-test ROM select. To mapovalo $5000-$57FF v normalnim PURE BASIC bootu a CPU skocil do self-test kodu ($532F), proto vznikl snih/smet.

FIX216 oprava:
- $5000-$57FF self-test ROM window je aktivni jen pri explicitnim SELF TEST/BYE SELFTEST rezimu.
- PURE BASIC BOOT vidi RAM na $5000, ne self-test ROM.
- Vsechny ROM soubory od Reneho jsou zkopirovane do app/src/main/assets/rom_audit pro audit.
- Hlavni spousteci ROM zustava ATARIXL.ROM + ATARIBAS.ROM z posledniho uploadu.

ROM audit:
- REV01.ROM: 16384 bytes md5 9aea45e724d2588fbbeda658c7dc53ee tail 18c0b8c22cc0
- Atariosa.rom: 10240 bytes md5 eb1f32f5d9f382db1bbfb8d7f9cb343a tail b4e777e4f3e6
- REVAPAL.ROM: 10240 bytes md5 eb1f32f5d9f382db1bbfb8d7f9cb343a tail b4e777e4f3e6
- REVBNTSC.ROM: 10240 bytes md5 4177f386a3bac989a981d3fe3388cb6c tail 91e725f1f3e6
- ATARIBAS.ROM: 8192 bytes md5 0bac0c6a50104045d902df4503a4c30b tail 00a00005f0bf
- ATARIOSB.ROM: 10240 bytes md5 a3e8d617c95d08031fe1b20d541434b2 tail 91e725f1f3e6
- ATARIXL.ROM: 16384 bytes md5 06daac977823773a3eea3422fd26a703 tail 18c0aac22cc0
- REV02.ROM: 16384 bytes md5 06daac977823773a3eea3422fd26a703 tail 18c0aac22cc0
- REV03.ROM: 16384 bytes md5 54e704558a6aedfc45cebf8f8ac9c312 tail 18c0aac22cc0
- REV04.ROM: 16384 bytes md5 b7a2a04677d34f069eeb643d5238bf86 tail 18c0aac22cc0
- REV10.ROM: 16384 bytes md5 e3e8c74bfe1dcd6b56af50bd9a82dc15 tail 3cc065c350c0
- REVA.ROM: 8192 bytes md5 a4dc52536d526ecc51ea857b9fa2b90f tail 00a00005f9bf
- REVB.ROM: 8192 bytes md5 04ea6a4e386601445ca5bfc8e37fb620 tail 00a00005f0bf
- REVC.ROM: 8192 bytes md5 0bac0c6a50104045d902df4503a4c30b tail 00a00005f0bf

Test plan:
1. PURE ROM BASIC BOOT.
2. Jestli BASIC nenabehne, poslat snapshot s BUILD TAG FIX216 a FIX216 SELFTEST GATE.
3. Az potom PURE ROM SELF TEST.

