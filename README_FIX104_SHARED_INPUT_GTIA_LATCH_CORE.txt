AtariHelp.eu EMU-09 FIX104_SHARED_INPUT_GTIA_LATCH_CORE

REPACK: opraveny viditelny build label. Predchozi FIX104 balik mel v nekterych UI/log hlaskach stale text FIX103, i kdyz EMU_BUILD_TAG uz byl FIX104.

Zmeny jadra:
- sdilena TRIG0/STRIG0 opakovaci cesta pro hry typu Cobra/River
- GTIA collision latch timing s odlozenym HITCLR pro render-time kolize
- bounded playfield pixel fallback pro kolize v DLI-shifted terenu
- Cobra title/menu fallback zachovan
- Donkey safe baseline zachovan
- Arkanoid clean menu zachovano
- Montezuma VVBLKD/PMG viditelnost zachovana

Kontrola: v emulatoru, snapshotu i ulozenem logu musi byt FIX104, ne FIX103.
