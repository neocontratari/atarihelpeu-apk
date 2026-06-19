BUILD2ET - OS BUFFER CHECKSUM FIX

KODY JSOU STEJNE.

Zmenen je jen app\src\main\assets\emu\index.html.
CSAVE WAV export se nemeni, protoze funguje v Altirre i na realnem Atari 130XE.

Oprava proti BUILD2ES:
- BUILD2ES zapisoval do OS cassette bufferu control+128 dat, ale checksum pocital bez uvodnich $55 $55.
- Realny Atari checksum je pres cely 132B record vcetne $55 $55.
- BUILD2ET proto preseeduje checksum hodnotou $AA, coz je checksum po $55+$55.

Postup:
1. Zkopiruj SPUSTIT_BUILD2ET_OS_BUFFER_CHECKSUM_FIX.cmd a slozku _EMU10_BUILD2ET_PAYLOAD do korene projektu.
2. Spust CMD z korene projektu.
3. GitHub Desktop: Commit to main + Push origin.
4. WWW/Actions vytvori APK.
