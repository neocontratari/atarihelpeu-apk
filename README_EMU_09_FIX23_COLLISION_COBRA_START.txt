EMU-09 FIX23 COLLISION + COBRA START

- PiTT-KiTT zůstává referenční hra, XEX beze změny.
- River Raid: opraveno čtení GTIA collision registrů $D000-$D00F, aby LEFT/RIGHT nezpůsobovaly falešnou okamžitou smrt.
- Přidány OS shadow vstupy STICK0 $0278 a STRIG0 $0284 pro cizí XEX hry.
- Super Cobra: opravená detekce profilu, aby nepadala do River profilu jen kvůli podobné velikosti XEX.
- Super Cobra: lehký frame latch stub $A1 proti smyčce kolem $A0E2.
- TAP START / TAP FIRE prodlouženy na 180 ms pro starší startovací smyčky.
