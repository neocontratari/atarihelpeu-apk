AtariHelp.eu EMU-10 BUILD2LW_CORE_VBXE_SR_TRANSPARENT_ZERO_SAFE

ZAKLAD:
- BUILD2LR, ne LT/LU/LV.
- River Raid TOP + rychly loader z LR chranen.
- UI / klavesnice / joystick / kazeta / CLOAD / CSAVE beze zmen.
- KODY JSOU STEJNE.

PROC TENTO SMER:
- LT odstranil Night Driver pruh, ale rezal auto.
- LV vratil auto, ale vratil se pruh.
- Proto koncim s radkovymi SR filtry a jdu do core VBXE pravidla.

CORE ZMENA:
- VBXE SR pixel mode znovu respektuje VIDEO_CONTROL transparentnost.
- Index 0 je transparentni, pokud neni VC_NO_TRANS.
- Pri VC_TRANS15 je transparentni i low-nibble $F.
- To plati obecne pro SR/LR overlay, neni to patch podle nazvu hry.

TEST:
1) River Raid
- musi zustat TOP vcetne nahravani.

2) Night Driver
- pruh/sum: pryc / stejny / horsi
- auto: cele / ukrojene / horsi
- zvuk: OK / horsi

3) Popeye VBXE / W3D
- nahravani rychle / horsi
- grafika: lepsi / stejna / horsi

4) Decathlon
- cekam spis stejny stav; tenhle build neni Decathlon fix.
- pokud se zmeni, poslat screenshot + log.

V LOGU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2LW_CORE_VBXE_SR_TRANSPARENT_ZERO_SAFE pripraven
VBXE SR TRANSPARENT ZERO BUILD2LW
VBXE LR TRANSPARENT ZERO BUILD2LW

V LOGU NESMI BYT JAKO HLAVNI NOVY SMER:
BUILD2LT/LU/LV odrezavani SR radku jako reseni.
