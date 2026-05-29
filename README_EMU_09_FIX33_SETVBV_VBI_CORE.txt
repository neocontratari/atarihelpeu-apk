EMU-09 FIX33 SETVBV / VBI CORE

Základ: FIX32.
PiTT-KiTT zůstává referenční XEX.

Oprava jádra:
- Implementován Atari OS SETVBV $E45C pro A=6/A=7.
- Hry jako Pitstop II si přes SETVBV instalují VBI rutinu; konkrétně Pitstop II dává VBI $3354.
- Bez této rutiny čekací timer $E7 nikdy neklesl a hra po titulním obrázku nepostoupila.
- Snapshot vypisuje VBI/VVBLKD a timer $E7.

Testy:
1. PiTT-KiTT kontrola.
2. Pitstop II: po titulním logu počkat nebo TAP START/FIRE, snapshot má ukázat VBI=$3354 a měnící se timer $E7.
3. Arkanoid III: FIRE/START už jde do hry; menu volby v tomto konkrétním XEXu nemusí fungovat ani v Altirře.
