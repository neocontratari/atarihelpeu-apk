# vyrob_b352.py - B352: testovaci programy pro Atari (6502, vlastni kod) -> test_b352_program.h
# Test pameti jako ve hre Tony VBXE ($1A66): pro kazdou hodnotu PORTB z tabulky zapise na $4000 cislo X+1,
# pak spocita, kolik hodnot si svoje cislo udrzelo (= kolik ruznych bank pocitac opravdu ma) -> $0600,
# hotovo $A5 -> $0601. Tabulky: Tony (bity 2-3 a 5-7 = 32 hodnot, chce 1088K), 130XE (4 banky),
# 320K Compy Shop (16 bank - jako B323 samotest). Varianty: XEX (kod na $2000) a bootovaci ATR (kod na $0706).
#   python3 vyrob_b352.py > test_b352_program.h
import sys

TONY = [0x83, 0x87, 0x8B, 0x8F, 0xA3, 0xA7, 0xAB, 0xAF, 0xC3, 0xC7, 0xCB, 0xCF, 0xE3, 0xE7, 0xEB, 0xEF,
        0x03, 0x07, 0x0B, 0x0F, 0x23, 0x27, 0x2B, 0x2F, 0x43, 0x47, 0x4B, 0x4F, 0x63, 0x67, 0x6B, 0x6F]
XE130 = [0xE3, 0xE7, 0xEB, 0xEF]
COMPY = [0x23 | ((b & 3) << 2) | ((~b & 0x0C) << 4) for b in range(16)]

def kod(org, tab):
    n = len(tab)
    o = []
    def b(*x): o.extend(x)
    adr_tab = None
    # pozice tabulky se doplni pozdeji (2 pruchody)
    for pruchod in range(2):
        o.clear()
        t = adr_tab or 0
        b(0xA2, 0x00)                          # LDX #0
        l1 = org + len(o)
        b(0xBD, t & 0xFF, t >> 8)              # LDA tab,X
        b(0x8D, 0x01, 0xD3)                    # STA PORTB
        b(0x8A, 0x18, 0x69, 0x01)              # TXA / CLC / ADC #1
        b(0x8D, 0x00, 0x40)                    # STA $4000
        b(0xE8, 0xE0, n)                       # INX / CPX #n
        b(0xD0, (l1 - (org + len(o) + 2)) & 0xFF)   # BNE l1
        b(0xA9, 0xFF, 0x8D, 0x01, 0xD3)        # LDA #$FF / STA PORTB
        b(0x8D, 0x00, 0x40)                    # STA $4000 (zakladni pamet = $FF)
        b(0xA2, 0x00, 0xA0, 0x00)              # LDX #0 / LDY #0
        l2 = org + len(o)
        b(0xBD, t & 0xFF, t >> 8)              # LDA tab,X
        b(0x8D, 0x01, 0xD3)                    # STA PORTB
        b(0x8A, 0x18, 0x69, 0x01)              # TXA / CLC / ADC #1
        b(0xCD, 0x00, 0x40)                    # CMP $4000
        b(0xD0, 0x01, 0xC8)                    # BNE +1 / INY
        b(0xE8, 0xE0, n)                       # INX / CPX #n
        b(0xD0, (l2 - (org + len(o) + 2)) & 0xFF)   # BNE l2
        b(0xA9, 0xFF, 0x8D, 0x01, 0xD3)        # LDA #$FF / STA PORTB
        b(0x8C, 0x00, 0x06)                    # STY $0600
        b(0xA9, 0xA5, 0x8D, 0x01, 0x06)        # LDA #$A5 / STA $0601
        konec = org + len(o)
        b(0x4C, konec & 0xFF, konec >> 8)      # JMP *
        adr_tab = org + len(o)
    o.extend(tab)
    return o

def xex(tab):
    k = kod(0x2000, tab)
    d = [0xFF, 0xFF, 0x00, 0x20, (0x2000 + len(k) - 1) & 0xFF, (0x2000 + len(k) - 1) >> 8] + k
    d += [0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20]
    return d

def atr(tab):
    # bootovaci sektor: 0 priznaky, 1 sektoru, nahrat na $0700, DOSINI = RTS na konci, kod od $0706 (OS ho zavola JSR)
    k = kod(0x0706, tab)
    rts = 0x0706 + len(k)
    sektor = [0x00, 0x01, 0x00, 0x07, rts & 0xFF, rts >> 8] + k + [0x60]
    assert len(sektor) <= 128, len(sektor)
    sektor += [0] * (128 - len(sektor))
    sektoru = 720
    par = sektoru * 128 // 16
    d = [0x96, 0x02, par & 0xFF, par >> 8, 128, 0] + [0] * 10 + sektor + [0] * ((sektoru - 1) * 128)
    return d

def pole(jm, d):
    s = 'static const uint8_t %s[%d] = {\n' % (jm, len(d))
    for i in range(0, len(d), 24):
        s += '  ' + ', '.join('0x%02X' % x for x in d[i:i + 24]) + ',\n'
    return s + '};\n'

out = ['// test_b352_program.h - vyrobeno vyrob_b352.py (vlastni kod 6502: test pameti jako ve hre Tony VBXE)\n#pragma once\n#include <cstdint>\n']
out.append(pole('B352_XEX_TONY', xex(TONY)))
out.append(pole('B352_XEX_130XE', xex(XE130)))
out.append(pole('B352_XEX_COMPY', xex(COMPY)))
atr_t = atr(TONY)
# ATR je velky (720 sektoru) - do hlavicky jen hlavicka + bootovaci sektor, zbytek doplni test nulami
out.append(pole('B352_ATR_TONY_ZACATEK', atr_t[:16 + 128]))
out.append('static const int B352_ATR_SEKTORU = 720;\n')
sys.stdout.write('\n'.join(out))
