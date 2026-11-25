#!/usr/bin/env python3
# asm6502.py - B348: maly dvouprechodovy assembler 6502 pro testovaci program
# (test_b348_sio.asm -> test_b348_program.h). Jen to, co test potrebuje:
#   NAZEV = vyraz | .org vyraz | navesti: | .byte v, ... | .word v, ...
#   instrukce: imp/A, #imm, zp/abs, zp,X / abs,X, abs,Y, (zp),Y, (zp,X), (abs)
#   vyrazy: $hex, %bin, desitkove, 'z', symboly, + a -, <x (nizsi bajt), >x (vyssi)
# Pouziti: python3 asm6502.py vstup.asm vystup.h PREFIX SYMBOL1,SYMBOL2,...
import re, sys

OPS = {}
for radek in """
ADC 69 65 75 -- 6D 7D 79 61 71 -- -- --|AND 29 25 35 -- 2D 3D 39 21 31 -- -- --
ASL -- 06 16 -- 0E 1E -- -- -- -- 0A --|BIT -- 24 -- -- 2C -- -- -- -- -- -- --
CMP C9 C5 D5 -- CD DD D9 C1 D1 -- -- --|CPX E0 E4 -- -- EC -- -- -- -- -- -- --
CPY C0 C4 -- -- CC -- -- -- -- -- -- --|DEC -- C6 D6 -- CE DE -- -- -- -- -- --
EOR 49 45 55 -- 4D 5D 59 41 51 -- -- --|INC -- E6 F6 -- EE FE -- -- -- -- -- --
JMP -- -- -- -- 4C -- -- -- -- 6C -- --|JSR -- -- -- -- 20 -- -- -- -- -- -- --
LDA A9 A5 B5 -- AD BD B9 A1 B1 -- -- --|LDX A2 A6 -- B6 AE -- BE -- -- -- -- --
LDY A0 A4 B4 -- AC BC -- -- -- -- -- --|LSR -- 46 56 -- 4E 5E -- -- -- -- 4A --
ORA 09 05 15 -- 0D 1D 19 01 11 -- -- --|ROL -- 26 36 -- 2E 3E -- -- -- -- 2A --
ROR -- 66 76 -- 6E 7E -- -- -- -- 6A --|SBC E9 E5 F5 -- ED FD F9 E1 F1 -- -- --
STA -- 85 95 -- 8D 9D 99 81 91 -- -- --|STX -- 86 -- 96 8E -- -- -- -- -- -- --
STY -- 84 94 -- 8C -- -- -- -- -- -- --
""".replace("\n", "|").split("|"):
    c = radek.split()
    if not c: continue
    m = c[0]
    rezimy = ["imm", "zp", "zpx", "zpy", "abs", "abx", "aby", "izx", "izy", "ind", "acc", "imp"]
    OPS[m] = {r: int(k, 16) for r, k in zip(rezimy, c[1:]) if k != "--"}
for m, k in (("BRK", 0), ("CLC", 0x18), ("CLD", 0xD8), ("CLI", 0x58), ("CLV", 0xB8), ("DEX", 0xCA), ("DEY", 0x88),
             ("INX", 0xE8), ("INY", 0xC8), ("NOP", 0xEA), ("PHA", 0x48), ("PHP", 0x08), ("PLA", 0x68), ("PLP", 0x28),
             ("RTI", 0x40), ("RTS", 0x60), ("SEC", 0x38), ("SED", 0xF8), ("SEI", 0x78), ("TAX", 0xAA), ("TAY", 0xA8),
             ("TSX", 0xBA), ("TXA", 0x8A), ("TXS", 0x9A), ("TYA", 0x98)):
    OPS[m] = {"imp": k}
for m, k in (("BPL", 0x10), ("BMI", 0x30), ("BVC", 0x50), ("BVS", 0x70), ("BCC", 0x90), ("BCS", 0xB0), ("BNE", 0xD0), ("BEQ", 0xF0)):
    OPS[m] = {"rel": k}
DELKA = {"imp": 1, "acc": 1, "imm": 2, "zp": 2, "zpx": 2, "zpy": 2, "izx": 2, "izy": 2, "rel": 2, "abs": 3, "abx": 3, "aby": 3, "ind": 3}

def hodnota(t, sym, pruchod):
    t = t.strip()
    if t.startswith("<") or t.startswith(">"):   # B363: i dopredny odkaz (v 1. pruchodu jeste neznamy)
        v = hodnota(t[1:], sym, pruchod)
        if v is None: return None
        return v & 0xFF if t.startswith("<") else (v >> 8) & 0xFF
    casti = re.split(r"([+-])", t)
    v, zn = 0, "+"
    for c in casti:
        c = c.strip()
        if c in ("+", "-"): zn = c; continue
        if not c: continue
        if c.startswith("$"): x = int(c[1:], 16)
        elif c.startswith("%"): x = int(c[1:], 2)
        elif re.fullmatch(r"'.'", c): x = ord(c[1])
        elif re.fullmatch(r"\d+", c): x = int(c)
        elif c in sym: x = sym[c]
        elif pruchod == 1: return None          # dopredny odkaz - jeste nezname
        else: raise SystemExit("neznamy symbol: " + c)
        v = v + x if zn == "+" else v - x
    return v

def preloz(text):
    radky = text.split("\n")
    sym, volby = {}, {}
    for pruchod in (1, 2):
        pc, vystup, org0 = 0, {}, None
        for i, r in enumerate(radky):
            r = r.split(";")[0].rstrip()
            if not r.strip(): continue
            m = re.match(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$", r)
            if m:
                v = hodnota(m.group(2), sym, pruchod)
                if v is not None: sym[m.group(1)] = v
                continue
            m = re.match(r"^\s*([A-Za-z_][A-Za-z0-9_]*):(.*)$", r)
            if m:
                if pruchod == 1 and m.group(1) in sym: raise SystemExit("navesti dvakrat: " + m.group(1))
                sym[m.group(1)] = pc
                r = m.group(2)
                if not r.strip(): continue
            r = r.strip()
            if r.lower().startswith(".org"):
                pc = hodnota(r[4:], sym, 2)
                if org0 is None: org0 = pc
                continue
            if r.lower().startswith(".byte") or r.lower().startswith(".word"):
                w = r.lower().startswith(".word")
                for p in r[5:].split(","):
                    v = hodnota(p, sym, pruchod) or 0
                    if w: vystup[pc] = v & 0xFF; vystup[pc + 1] = (v >> 8) & 0xFF; pc += 2
                    else: vystup[pc] = v & 0xFF; pc += 1
                continue
            m = re.match(r"^([A-Za-z]{3})\s*(.*)$", r)
            if not m: raise SystemExit("nerozumim: " + r)
            mn, op = m.group(1).upper(), m.group(2).strip()
            if mn not in OPS: raise SystemExit("neznama instrukce: " + mn)
            t = OPS[mn]
            if "rel" in t:
                rez, v = "rel", hodnota(op, sym, pruchod)
            elif not op:
                rez, v = ("imp" if "imp" in t else "acc"), 0
            elif op.upper() == "A":
                rez, v = "acc", 0
            elif op.startswith("#"):
                rez, v = "imm", hodnota(op[1:], sym, pruchod)
            elif re.fullmatch(r"\(.+\)\s*,\s*[Yy]", op):
                rez, v = "izy", hodnota(op[1:op.index(")")], sym, pruchod)
            elif re.fullmatch(r"\(.+,\s*[Xx]\s*\)", op):
                rez, v = "izx", hodnota(op[1:op.rindex(",")], sym, pruchod)
            elif op.startswith("("):
                rez, v = "ind", hodnota(op[1:-1], sym, pruchod)
            else:
                idx = None
                mm = re.fullmatch(r"(.+),\s*([XxYy])", op)
                if mm: op, idx = mm.group(1), mm.group(2).upper()
                v = hodnota(op, sym, pruchod)
                if pruchod == 1:
                    zp = v is not None and 0 <= v < 256
                    volby[i] = zp
                zp = volby[i]
                if idx is None: rez = "zp" if zp and "zp" in t else "abs"
                elif idx == "X": rez = "zpx" if zp and "zpx" in t else "abx"
                else: rez = "zpy" if zp and "zpy" in t else "aby"
            if rez not in t: raise SystemExit("rezim %s neni u %s (radek %d)" % (rez, mn, i + 1))
            n = DELKA[rez]
            if pruchod == 2:
                vystup[pc] = t[rez]
                if rez == "rel":
                    d = v - (pc + 2)
                    if not -128 <= d <= 127: raise SystemExit("skok moc daleko: radek %d" % (i + 1))
                    vystup[pc + 1] = d & 0xFF
                elif n == 2: vystup[pc + 1] = v & 0xFF
                elif n == 3: vystup[pc + 1] = v & 0xFF; vystup[pc + 2] = (v >> 8) & 0xFF
            pc += n
    lo, hi = min(vystup), max(vystup)
    return org0, bytes(vystup.get(a, 0) for a in range(lo, hi + 1)), sym

if __name__ == "__main__":
    vstup, vys, prefix, verejne = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4].split(",")
    org, data, sym = preloz(open(vstup, encoding="utf-8").read())
    with open(vys, "w", encoding="utf-8") as o:
        o.write("// %s - VYGENEROVANO asm6502.py z %s (needitovat rucne)\n#pragma once\n#include <cstdint>\n" % (vys.split("/")[-1], vstup.split("/")[-1]))
        o.write("static const int %s_ORG = 0x%04X;\n" % (prefix, org))
        for s in verejne: o.write("static const int %s_%s = 0x%04X;\n" % (prefix, s, sym[s]))
        o.write("static const uint8_t %s_KOD[%d] = {\n" % (prefix, len(data)))
        for a in range(0, len(data), 16):
            o.write("  " + ", ".join("0x%02X" % b for b in data[a:a + 16]) + ",\n")
        o.write("};\n")
    print("%s: %d bajtu od $%04X" % (vys, len(data), org))
