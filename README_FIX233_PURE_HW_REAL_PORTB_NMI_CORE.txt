AtariHelp.eu EMU-09 FIX233 PURE HW REAL PORTB NMI CORE

KODY JSOU STEJNE. SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy beze zmen.

ROM audit novych souboru:
- co24947a.rom 8192 md5 0bac0c6a50104045d902df4503a4c30b = stejny obsah jako hlavni ATARIBAS/BASIC C
- co60302a.rom 8192 md5 04ea6a4e386601445ca5bfc8e37fb620 = BASIC B varianta, ulozena do assets/rom_audit
- co61598b.rom 16384 md5 06daac977823773a3eea3422fd26a703 = stejny obsah jako hlavni ATARIXL.ROM

Duvod FIX233:
FIX232 BASIC log ukazal skok ROM na $5003, ale mode-gate blokoval self-test ROM okno. To byla spatna oprava. FIX233 nechava $5000-$57FF ridit realnym PIA effective PORTB bit7.
NMI se ted v PURE HW zveda jen pokud NMIEN obsahuje prislusny bit.

Test:
1. PURE HW AUDIT + SNAPSHOT
2. POWER XL/XE BASIC + SNAPSHOT
3. POWER OPTION SELF TEST + SNAPSHOT

Hledej v logu:
PURE HARDWARE SNAPSHOT FIX233
WINDOWS BASIC ... SELFTEST ... REAL_PORTB_RULE
NMI frame request ... NMIEN ... raised ...
