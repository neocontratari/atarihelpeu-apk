#!/usr/bin/env python3
"""B321: vlozi osobni klic N&P do APK presne jako server v10 (NapApk.NovyBlok / NovyEocd v NapServer.cs):
dvojice ID 0x4E615056 v APK Signing Block, zaznamy ZIP a centralni adresar beze zmeny, vypln na 4096 B
jako apksigner. Slouzi ke kontrole skutecneho APK z CI (apksigner verify + NapApkKlic z appky).
Pouziti: python3 vloz_klic.py vstup.apk vystup.apk [text hodnoty]"""
import struct, sys

ID_NAP, ID_VYPLN = 0x4E615056, 0x42726577
ID_PODPIS = (0x7109871A, 0xF05368C0, 0x1B93AD61)


def rozbor(a):
    n = len(a)
    for i in range(n - 22, max(-1, n - 22 - 65536), -1):
        if a[i:i + 4] == b'PK\x05\x06' and i + 22 + struct.unpack_from('<H', a, i + 20)[0] == n:
            eocd = i
            break
    else:
        raise SystemExit('neni ZIP')
    cd_delka, cd = struct.unpack_from('<II', a, eocd + 12)
    if cd + cd_delka != eocd or a[cd - 16:cd] != b'APK Sig Block 42':
        raise SystemExit('APK nema podpisovy blok v2/v3')
    vel = struct.unpack_from('<Q', a, cd - 24)[0]
    blok = cd - vel - 8
    if struct.unpack_from('<Q', a, blok)[0] != vel:
        raise SystemExit('poskozeny podpisovy blok')
    p, konec, pary, vypln = blok + 8, cd - 24, [], False
    while p < konec:
        d, i = struct.unpack_from('<QI', a, p)
        if i == ID_VYPLN:
            vypln = True
        else:
            pary.append((i, a[p + 12:p + 8 + d]))
        p += 8 + d
    if p != konec or not any(i in ID_PODPIS for i, _ in pary):
        raise SystemExit('APK nema podpis v2/v3')
    return blok, cd, cd_delka, eocd, pary, vypln


def vloz(a, hodnota):
    blok, cd, cd_delka, eocd, pary, vypln = rozbor(a)
    telo = b''.join(struct.pack('<QI', len(v) + 4, i) + v for i, v in pary if i != ID_NAP)
    telo += struct.pack('<QI', len(hodnota) + 4, ID_NAP) + hodnota
    if vypln:
        zbytek = (8 + len(telo) + 8 + 16) % 4096
        if zbytek:
            v = 4096 - zbytek
            if v < 12:
                v += 4096
            telo += struct.pack('<QI', v - 8, ID_VYPLN) + bytes(v - 12)
    vel = len(telo) + 8 + 16
    novy = struct.pack('<Q', vel) + telo + struct.pack('<Q', vel) + b'APK Sig Block 42'
    konec = bytearray(a[eocd:])
    struct.pack_into('<I', konec, 16, blok + len(novy))
    return a[:blok] + novy + a[cd:eocd] + bytes(konec)


if __name__ == '__main__':
    hodnota = (sys.argv[3] if len(sys.argv) > 3 else 'nap=1\ntajemstvi=' + '5a' * 16 + '\napk_sha256=kontrola-ci\n').encode('utf-8')
    a = open(sys.argv[1], 'rb').read()
    b = vloz(a, hodnota)
    open(sys.argv[2], 'wb').write(b)
    print('klic vlozen: %d -> %d B' % (len(a), len(b)))
