#!/usr/bin/env python3
# B357: TESTOVACI ROM PRO SEGU (Mega Drive) - rucne prelozeny program 68000 (zadny assembler tu neni).
# Kazdy snimek: pocka na zatemneni (VBLANK), precte OBA ovladace (protokol 3 tlacitek: TH=1 a TH=0), zamicha je
# do generatoru (xorshift32), ulozi do RAM (kruh 4 KB) a nastavi barvu pozadi (CRAM 0). Obraz i RAM tak zavisi
# na kazdem stisku obou hracu ve spravnem snimku - kdyby se telefony v online hre lisily o jediny snimek
# vstupu, otisk stavu by se lisil. Vypise ROM jako pole C++ (sega_test_rom.h).
import sys

def w(*xs):
    out = []
    for x in xs:
        out += [(x >> 8) & 0xFF, x & 0xFF]
    return out

code = []          # (popis, bajty) - adresy od 0x200
lab = {}
def at():
    return 0x200 + sum(len(b) for _, b in code)
def ins(popis, *slova):
    code.append((popis, w(*slova)))
def label(n):
    lab[n] = at()

ins("move.w #$2700,sr", 0x46FC, 0x2700)
ins("lea $C00004,a0", 0x41F9, 0x00C0, 0x0004)
ins("lea $C00000,a1", 0x43F9, 0x00C0, 0x0000)
ins("move.w #$8004,(a0)", 0x30BC, 0x8004)
ins("move.w #$8144,(a0)  ; obraz zapnuty, rezim 5", 0x30BC, 0x8144)
ins("move.w #$8F02,(a0)  ; autoinkrement 2", 0x30BC, 0x8F02)
ins("move.b #$40,$A10009 ; port 1: TH vystup", 0x13FC, 0x0040, 0x00A1, 0x0009)
ins("move.b #$40,$A1000B ; port 2: TH vystup", 0x13FC, 0x0040, 0x00A1, 0x000B)
ins("moveq #1,d7", 0x7E01)
ins("lea $FF0000,a2", 0x45F9, 0x00FF, 0x0000)
label("smycka")
ins("move.w (a0),d0      ; stav VDP", 0x3010)
ins("btst #3,d0          ; VBLANK?", 0x0800, 0x0003)
code.append(("beq.s smycka", [0, 0]))                   # doplni se nize
ins("move.b #$40,$A10003 ; port 1 TH=1", 0x13FC, 0x0040, 0x00A1, 0x0003)
ins("nop", 0x4E71); ins("nop", 0x4E71)
ins("move.b $A10003,d1   ; ?1CBRLDU", 0x1239, 0x00A1, 0x0003)
ins("move.b #$00,$A10003 ; port 1 TH=0", 0x13FC, 0x0000, 0x00A1, 0x0003)
ins("nop", 0x4E71); ins("nop", 0x4E71)
ins("move.b $A10003,d2   ; ?0SA00DU", 0x1439, 0x00A1, 0x0003)
ins("move.b #$40,$A10005 ; port 2 TH=1", 0x13FC, 0x0040, 0x00A1, 0x0005)
ins("nop", 0x4E71); ins("nop", 0x4E71)
ins("move.b $A10005,d3", 0x1639, 0x00A1, 0x0005)
ins("move.b #$00,$A10005 ; port 2 TH=0", 0x13FC, 0x0000, 0x00A1, 0x0005)
ins("nop", 0x4E71); ins("nop", 0x4E71)
ins("move.b $A10005,d4", 0x1839, 0x00A1, 0x0005)
ins("move.l d7,d5", 0x2A07); ins("moveq #13,d6", 0x7C0D); ins("lsl.l d6,d5", 0xEDAD); ins("eor.l d5,d7", 0xBB87)
ins("move.l d7,d5", 0x2A07); ins("moveq #17,d6", 0x7C11); ins("lsr.l d6,d5", 0xECAD); ins("eor.l d5,d7", 0xBB87)
ins("move.l d7,d5", 0x2A07); ins("lsl.l #5,d5", 0xEB8D); ins("eor.l d5,d7", 0xBB87)
ins("eor.b d1,d7", 0xB307); ins("rol.l #8,d7", 0xE19F)
ins("eor.b d2,d7", 0xB507); ins("rol.l #8,d7", 0xE19F)
ins("eor.b d3,d7", 0xB707); ins("rol.l #8,d7", 0xE19F)
ins("eor.b d4,d7", 0xB907); ins("rol.l #8,d7", 0xE19F)
ins("move.l d7,(a2)+     ; do RAM", 0x24C7)
ins("move.l a2,d0", 0x200A)
ins("andi.w #$0FFF,d0", 0x0240, 0x0FFF)
code.append(("bne.s dal", [0, 0]))
ins("lea $FF0000,a2      ; kruh 4 KB", 0x45F9, 0x00FF, 0x0000)
label("dal")
ins("move.l #$C0000000,(a0) ; zapis CRAM 0", 0x20BC, 0xC000, 0x0000)
ins("move.w d7,d0", 0x3007)
ins("andi.w #$0EEE,d0", 0x0240, 0x0EEE)
ins("move.w d0,(a1)      ; barva pozadi", 0x3280)
label("konecvb")
ins("move.w (a0),d0", 0x3010)
ins("btst #3,d0", 0x0800, 0x0003)
code.append(("bne.s konecvb", [0, 0]))
code.append(("bra.w smycka", [0, 0, 0, 0]))

# doplnit skoky (beq.s / bne.s 2 B, bra.w 4 B - delky uz jsou v kodu jako nuly)
for i, (p, b) in enumerate(code):
    adr = 0x200 + sum(len(x) for _, x in code[:i])
    if p.startswith("beq.s") or p.startswith("bne.s"):
        cil = lab[p.split()[1]]
        d = cil - (adr + 2)
        assert -128 <= d < 128 and d != 0
        code[i] = (p, [0x67 if p.startswith("beq") else 0x66, d & 0xFF])
    elif p.startswith("bra.w"):
        cil = lab[p.split()[1]]
        d = cil - (adr + 2)
        code[i] = (p, [0x60, 0x00, (d >> 8) & 0xFF, d & 0xFF])

rom = bytearray(0x2000)
rom[0:4] = (0x00FFFE00).to_bytes(4, "big")       # SSP
rom[4:8] = (0x00000200).to_bytes(4, "big")       # PC
for v in range(2, 64): rom[4 * v:4 * v + 4] = (0x00000300).to_bytes(4, "big")   # vyjimky -> smycka
rom[0x100:0x110] = b"SEGA MEGA DRIVE "
rom[0x150:0x150 + 48] = b"NAP B357 ONLINE TEST".ljust(48)
rom[0x1A0:0x1A8] = (0).to_bytes(4, "big") + (0x1FFF).to_bytes(4, "big")
rom[0x1F0:0x1F3] = b"JUE"
p = 0x200
for _, b in code:
    rom[p:p + len(b)] = bytes(b); p += len(b)
rom[0x300:0x302] = bytes([0x60, 0xFE])           # bra.s *
assert p <= 0x300

if len(sys.argv) > 1:
    with open(sys.argv[1], "w") as f:
        f.write("// vygenerovano sega_test_rom.py (B357) - rucne prelozeny program 68000, viz tam\n#pragma once\n")
        f.write("static const unsigned char SEGA_TEST_ROM[%d] = {\n" % len(rom))
        for i in range(0, len(rom), 32):
            f.write("  " + ",".join("0x%02X" % x for x in rom[i:i + 32]) + ",\n")
        f.write("};\n")
print("ROM %d B, program 0x200-0x%X, smycka 0x%X" % (len(rom), p, lab["smycka"]))
