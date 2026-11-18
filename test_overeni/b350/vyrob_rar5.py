#!/usr/bin/env python3
# vyrob_rar5.py CILOVA_SLOZKA - testovaci archivy RAR 5 pro B350 (Atari: hra v RAR5 z noveho WinRARu)
#
# Vlastni maly koder RAR 5 jen pro testy (stejny princip jako RAR 2.9 v ../b349/vyrob_archivy.py):
# tokeny -> bitovy proud presne podle rozbalovace unrar (Unpack5, ReadBlockHeader, ReadTables, ReadFilter):
#  - bloky: priznaky (bity posledniho bajtu, pocet bajtu velikosti, posledni blok, tabulky), kontrolni bajt
#    0x5A ^ ..., velikost; blok bez tabulek pouzije predchozi (i v dalsim souboru pevneho archivu),
#  - tabulky BC 20 + LD 306 / DD 64 / LDD 16 / RD 44 (bez delty), kody 16-19,
#  - literaly, 257 (opakovani), 258-261 (stare vzdalenosti + delka z RD), 262+ (delka, vzdalenost,
#    nizke 4 bity pres LDD, +1 / +2 / +3 nad 0x100 / 0x2000 / 0x40000), 256 = filtr (DELTA, E8, E8E9, ARM),
#  - kontejner RAR 5: signatura, hlavicky s CRC32 a cisly promenne delky (vint), hlavni / soubor / konec,
#    CRC32 souboru (nebo bez nej - jako s otiskem BLAKE2), rozsireni "odkaz" (symlink).
# Vsechno ze vzniklych archivu rozbali i bsdtar (libarchive) - over_bsdtar.sh (../b349).
# Vedle archivu zapise do CILOVA_SLOZKA/obsah/<archiv>/<soubor> puvodni data.
import os, random, re, struct, sys, zlib

sys.dont_write_bytecode = True                  # zadny __pycache__ v ../b349
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'b349'))
import vyrob_archivy as V                       # atr(), xex(), TEXT, Bity, delky_huff, kody, kopiruj, uloz

CIL = sys.argv[1] if len(sys.argv) > 1 else 'archivy5'
os.makedirs(CIL, exist_ok=True)
V.CIL = CIL
V.OBSAH = os.path.join(CIL, 'obsah')
DOS_CAS = V.DOS_CAS

NC, DC, LDC, RC, BC = 306, 64, 16, 44, 20
HTS = NC + DC + LDC + RC
TAB_ROZSAH = {'LD': (0, NC), 'DD': (NC, DC), 'LDD': (NC + DC, LDC), 'RD': (NC + DC + LDC, RC)}
F_DELTA, F_E8, F_E8E9, F_ARM = 0, 1, 2, 3


def slot_delky(L):
    v = L - 2
    assert v >= 0, L
    if v < 8:
        return v, 0, 0
    for s in range(43, 7, -1):
        lb = s // 4 - 1
        z = (4 | (s & 3)) << lb
        if z <= v < z + (1 << lb):
            return s, lb, v - z
    raise ValueError(L)


def slot_vzd(d):
    v = d - 1
    if v < 4:
        return v, 0, 0
    for s in range(63, 3, -1):
        db = s // 2 - 1
        z = (2 | (s & 1)) << db
        if z <= v < z + (1 << db):
            return s, db, v - z
    raise ValueError(d)


def bonus(d):
    return (d > 0x100) + (d > 0x2000) + (d > 0x40000)


class Stav:
    def __init__(self):
        self.stare = [0, 0, 0, 0]; self.posledni = 0


def aplikuj(t, out, st):
    """token na okne (bez filtru) a stavu - presne jako Unpack5"""
    if t[0] == 'lit':
        out.append(t[1])
    elif t[0] == 'rep':
        V.kopiruj(out, st.posledni, st.stare[0])
    elif t[0] == 'old':
        k, L = t[1], t[2]
        d = st.stare[k]
        st.stare = [d] + st.stare[:k] + st.stare[k + 1:]
        st.posledni = L
        V.kopiruj(out, L, d)
    elif t[0] == 'match':
        L, d = t[1], t[2]
        st.stare = [d] + st.stare[:3]
        st.posledni = L
        V.kopiruj(out, L, d)


def filtruj(okno, zac, konec, filtry):
    """vystup souboru = okno[zac:konec] s pouzitymi filtry (jako unrar ApplyFilter; vstup = okno)"""
    out = bytearray(okno[zac:konec])
    for (start, n, typ, kan) in filtry:
        d = bytearray(okno[start:start + n])
        ofs = start - zac
        if typ in (F_E8, F_E8E9):
            pos = 0
            while pos + 4 < n:
                c = d[pos]; pos += 1
                if c == 0xE8 or (typ == F_E8E9 and c == 0xE9):
                    posun = (pos + ofs) % 0x1000000
                    a = struct.unpack_from('<I', d, pos)[0]
                    if a & 0x80000000:
                        if ((a + posun) & 0x80000000) == 0:
                            struct.pack_into('<I', d, pos, (a + 0x1000000) & 0xFFFFFFFF)
                    elif ((a - 0x1000000) & 0x80000000) != 0:
                        struct.pack_into('<I', d, pos, (a - posun) & 0xFFFFFFFF)
                    pos += 4
        elif typ == F_ARM:
            for pos in range(0, n - 3, 4):
                if d[pos + 3] == 0xEB:
                    o = d[pos] | (d[pos + 1] << 8) | (d[pos + 2] << 16)
                    o = (o - (ofs + pos) // 4) & 0xFFFFFF
                    d[pos], d[pos + 1], d[pos + 2] = o & 0xFF, (o >> 8) & 0xFF, (o >> 16) & 0xFF
        elif typ == F_DELTA:
            o = bytearray(n); src = 0
            for ch in range(kan):
                prev = 0
                for dst in range(ch, n, kan):
                    prev = (prev - okno[start + src]) & 0xFF
                    o[dst] = prev; src += 1
            d = o
        out[start - zac:start - zac + n] = d
    return bytes(out)


# ---------------- generator tokenu se vsemi kody a filtry ----------------
def skript(rng, kolik, out, st, blok_po=20000, filtry_ano=True):
    """vraci (tokeny, filtry); filtry = (absolutni start, delka, typ, kanalu)"""
    toks, filtry = [], []
    zac = len(out)
    dalsi_blok = zac + blok_po
    konec_filtru = zac
    while len(out) - zac < kolik:
        zbyva = kolik - (len(out) - zac)
        n = len(out)
        if n >= dalsi_blok and zbyva > 500:
            toks.append(('blok', rng.random() < 0.6))      # novy blok, s tabulkami / bez
            dalsi_blok = n + blok_po
            continue
        r = rng.random()
        if filtry_ano and r < 0.01 and zbyva > 3000 and n >= konec_filtru and n > 300:
            typ = rng.choice([F_DELTA, F_E8, F_E8E9, F_ARM])
            odsud = rng.choice([0, 0, rng.randint(1, 200)])
            delka = rng.randint(4, min(2500, zbyva - odsud - 10))
            kan = rng.randint(1, 32) if typ == F_DELTA else 0
            toks.append(('filtr', odsud, delka, typ, kan))
            filtry.append((n + odsud, delka, typ, kan))
            konec_filtru = n + odsud + delka
            # obsah pro filtr: instrukce E8 / E9 / ARM BL s adresami, aby filtr opravdu menil data
            for _ in range(rng.randint(5, 40)):
                if len(out) - zac >= kolik - 8:
                    break
                if typ in (F_E8, F_E8E9):
                    blok = [rng.choice([0xE8, 0xE9, 0x90])] + list(struct.pack('<i', rng.randint(-50000, 50000)))
                elif typ == F_ARM:
                    blok = list(struct.pack('<I', rng.getrandbits(24)))[:3] + [0xEB]
                else:
                    blok = [rng.getrandbits(8) for _ in range(4)]
                for b in blok:
                    t = ('lit', b); aplikuj(t, out, st); toks.append(t)
            continue
        if n < 16 or r < 0.22:
            for _ in range(min(zbyva, rng.randint(1, 6))):
                t = ('lit', min(255, int(rng.expovariate(0.06))))
                aplikuj(t, out, st); toks.append(t)
            continue
        if r < 0.29 and st.posledni and 0 < st.stare[0] <= n and st.posledni <= zbyva:
            t = ('rep',); aplikuj(t, out, st); toks.append(t); continue
        if r < 0.42:
            k = rng.randrange(4)
            if 0 < st.stare[k] <= n and zbyva >= 2:
                L = rng.randint(2, min(4097, zbyva)) if rng.random() < 0.1 else rng.randint(2, min(300, zbyva))
                t = ('old', k, L); aplikuj(t, out, st); toks.append(t); continue
        c = rng.random()
        if n > 0x40000 + 2000 and c < 0.05:
            d = rng.randint(0x40001, n)                      # vzdalenost nad 256 kB (+3 k delce)
        elif c < 0.3:
            d = rng.randint(1, min(n, 256))
        elif c < 0.6:
            d = rng.randint(1, min(n, 0x2000))
        elif c < 0.85:
            d = rng.randint(1, min(n, 0x40000))
        else:
            d = rng.randint(1, n)
        b = bonus(d)
        if zbyva < 2 + b:
            continue
        L = rng.randint(2 + b, min(4097 + b, zbyva)) if rng.random() < 0.05 else rng.randint(2 + b, min(260 + b, zbyva))
        t = ('match', L, d); aplikuj(t, out, st); toks.append(t)
    return toks, filtry


def lz_tokeny(data, out, st, blok_po=65536):
    """jednoduchy kompresor (skutecny obsah - ATR, XEX)"""
    toks = []
    zac = len(out)
    buf = bytes(out) + data
    hlavy = {}

    def pridej(p):
        if p + 3 <= len(buf):
            hlavy.setdefault(buf[p:p + 3], []).append(p)
    for p in range(max(0, zac - 65536), zac):
        pridej(p)
    p = zac
    dalsi_blok = zac + blok_po
    while p < len(buf):
        if p >= dalsi_blok and len(buf) - p > 1000:
            toks.append(('blok', False)); dalsi_blok = p + blok_po
        nejL, nejD = 0, 0
        maxL = min(4097, len(buf) - p)
        for d in st.stare:
            if 0 < d <= p:
                L = 0
                while L < maxL and buf[p + L] == buf[p + L - d]:
                    L += 1
                if L > nejL:
                    nejL, nejD = L, d
        for q in reversed(hlavy.get(buf[p:p + 3], [])[-24:]):
            if nejL >= 4097:
                break
            d = p - q
            L = 0
            m = min(4097, len(buf) - p)
            while L < m and buf[p + L] == buf[q + L]:
                L += 1
            if L > nejL + 1 or (L > nejL and nejD not in st.stare):
                nejL, nejD = L, d
        t = None
        if nejL >= 2 and nejD in st.stare:
            t = ('rep',) if (nejD == st.stare[0] and nejL == st.posledni) else ('old', st.stare.index(nejD), nejL)
        elif nejL >= 2 + bonus(nejD) and nejL - bonus(nejD) <= 4097:
            t = ('match', nejL, nejD)
        if t is None:
            t = ('lit', buf[p])
        pred = len(out)
        aplikuj(t, out, st); toks.append(t)
        for q in range(pred, len(out)):
            pridej(q)
        p = len(out)
    assert bytes(out[zac:]) == data
    return toks


# ---------------- bitovy proud ----------------
def fdata(v):
    n = 1 if v < 0x100 else 2 if v < 0x10000 else 3 if v < 0x1000000 else 4
    return [(2, n - 1)] + [(8, (v >> (8 * i)) & 0xFF) for i in range(n)]


def symboly(t):
    if t[0] == 'lit':
        return [('LD', t[1], [])]
    if t[0] == 'rep':
        return [('LD', 257, [])]
    if t[0] == 'old':
        s, lb, x = slot_delky(t[2])
        return [('LD', 258 + t[1], []), ('RD', s, [(lb, x)])]
    if t[0] == 'match':
        L, d = t[1], t[2]
        s, lb, x = slot_delky(L - bonus(d))
        ds, db, dx = slot_vzd(d)
        sy = [('LD', 262 + s, [(lb, x)])]
        if db >= 4:
            sy.append(('DD', ds, [(db - 4, dx >> 4)] if db > 4 else []))
            sy.append(('LDD', dx & 15, []))
        else:
            sy.append(('DD', ds, [(db, dx)] if db else []))
        return sy
    if t[0] == 'filtr':
        _, odsud, delka, typ, kan = t
        return [('LD', 256, fdata(odsud) + fdata(delka) + [(3, typ)] + ([(5, kan - 1)] if typ == F_DELTA else []))]
    return []


def zapis_tabulky(bw, L):
    sy = []
    i = 0
    while i < HTS:
        v = L[i]
        if v == 0:
            j = i
            while j < HTS and L[j] == 0 and j - i < 138:
                j += 1
            if j - i >= 11:
                sy.append((19, 7, j - i - 11)); i = j; continue
            if j - i >= 3:
                sy.append((18, 3, j - i - 3)); i = j; continue
        if i > 0 and v == L[i - 1]:
            j = i
            while j < HTS and L[j] == v and j - i < 138:
                j += 1
            if j - i >= 11:
                sy.append((17, 7, j - i - 11)); i = j; continue
            if j - i >= 3:
                sy.append((16, 3, j - i - 3)); i = j; continue
        sy.append((v, 0, 0)); i += 1
    cet = [0] * BC
    for s, _, _ in sy:
        cet[s] += 1
    bl = V.delky_huff(cet, 15)
    i = 0
    while i < BC:
        if bl[i] == 0:
            j = i
            while j < BC and bl[j] == 0 and j - i < 17:
                j += 1
            if j - i >= 3:
                bw.put(15, 4); bw.put(j - i - 2, 4); i = j; continue
            bw.put(0, 4); i += 1; continue
        if bl[i] == 15:
            bw.put(15, 4); bw.put(0, 4)
        else:
            bw.put(bl[i], 4)
        i += 1
    kb = V.kody(bl)
    for s, nb, x in sy:
        bw.put(kb[s], bl[s])
        if nb:
            bw.put(x, nb)


def koduj(soubory_toks):
    """tokeny souboru (pevny archiv = vic souboru, slovnik i tabulky navazuji) -> zabalena data souboru.
    ('blok', s_tabulkami) = zacatek noveho bloku; na zacatku souboru urci, jestli 1. blok ma tabulky
    (jinak: 1. soubor s tabulkami, dalsi soubor pevneho archivu bez nich = pouzije predchozi)."""
    bloky = []                                           # [soubor, s_tabulkami, tokeny]
    for fi, toks in enumerate(soubory_toks):
        toks = list(toks)
        s_tab = fi == 0
        if toks and toks[0][0] == 'blok':
            s_tab = toks[0][1] or fi == 0
            toks = toks[1:]
        cur = []
        for t in toks:
            if t[0] == 'blok':
                bloky.append([fi, s_tab, cur]); cur = []; s_tab = t[1]
            else:
                cur.append(t)
        bloky.append([fi, s_tab, cur])
    useky = []                                           # blok s tabulkami zacina novy usek (jedna tabulka)
    for bi, b in enumerate(bloky):
        assert b[2], 'prazdny blok'
        if b[1] or not useky:
            useky.append([])
        useky[-1].append(bi)
    vystup = [bytearray() for _ in soubory_toks]
    for u in useky:
        cet = {k: [0] * n for k, (_, n) in TAB_ROZSAH.items()}
        for bi in u:
            for t in bloky[bi][2]:
                for tab, s, _ in symboly(t):
                    cet[tab][s] += 1
        L = [0] * HTS
        for k, (o, n) in TAB_ROZSAH.items():
            L[o:o + n] = V.delky_huff(cet[k], 15)
        kk = {k: V.kody(L[o:o + n]) for k, (o, n) in TAB_ROZSAH.items()}
        ll = {k: L[o:o + n] for k, (o, n) in TAB_ROZSAH.items()}
        for bi in u:
            fi, s_tab, toks = bloky[bi]
            bw = V.Bity()
            if s_tab:
                zapis_tabulky(bw, L)
            for t in toks:
                for tab, s, ext in symboly(t):
                    assert ll[tab][s] > 0, (tab, s)
                    bw.put(kk[tab][s], ll[tab][s])
                    for nb, x in ext:
                        if nb:
                            bw.put(x, nb)
            bitu = len(bw.out) * 8 + bw.n
            data = bw.data()
            vel = len(data)
            posl_bitu = bitu - (vel - 1) * 8
            nb = 1 if vel < 0x100 else 2 if vel < 0x10000 else 3
            posledni = bi == len(bloky) - 1 or bloky[bi + 1][0] != fi
            prizn = (posl_bitu - 1) | ((nb - 1) << 3) | (0x40 if posledni else 0) | (0x80 if s_tab else 0)
            kontrola = (0x5A ^ prizn ^ vel ^ (vel >> 8) ^ (vel >> 16)) & 0xFF
            vystup[fi] += bytes([prizn, kontrola]) + vel.to_bytes(nb, 'little') + data
    return [bytes(v) for v in vystup]


# ---------------- kontejner RAR 5 ----------------
def vint(v):
    o = bytearray()
    while True:
        b = v & 0x7F; v >>= 7
        if v:
            o.append(b | 0x80)
        else:
            o.append(b); return bytes(o)


def hlavicka(typ, prizn, telo, extra=b'', data=None):
    f = prizn | (1 if extra else 0) | (2 if data is not None else 0)
    h = vint(typ) + vint(f)
    if extra:
        h += vint(len(extra))
    if data is not None:
        h += vint(data)
    h += telo + extra
    hs = vint(len(h))
    return struct.pack('<I', zlib.crc32(hs + h)) + hs + h


def rar5(clenove, pevny=False):
    """clenove: dict(jmeno, data, zabaleno=None (ulozeno) | bajty, crc=True, odkaz=None, slozka=False)"""
    out = bytearray(b'Rar!\x1a\x07\x01\x00')
    out += hlavicka(1, 0, vint(0x0004 if pevny else 0))
    for i, c in enumerate(clenove):
        jm = c['jmeno'].encode('utf-8')
        d = c.get('data', b'')
        z = c.get('zabaleno')
        slozka = c.get('slozka', False)
        bal = b'' if slozka or c.get('odkaz') else (d if z is None else z)
        metoda = 0 if z is None else 3
        ci = 0 | (0x40 if pevny and i > 0 and z is not None else 0) | (metoda << 7) | (3 << 10)   # slovnik 1 MB
        ff = (1 if slozka else 0) | 0x0002 | (0x0004 if c.get('crc', True) and not slozka else 0)
        telo = vint(ff) + vint(0 if slozka else len(d)) + vint(0x10 if slozka else 0x20) + struct.pack('<I', 0x5D726000)
        if ff & 4:
            telo += struct.pack('<I', zlib.crc32(d))
        telo += vint(ci) + vint(0) + vint(len(jm)) + jm
        extra = b''
        if c.get('odkaz'):
            cil = c['odkaz'].encode('utf-8')
            zaznam = vint(5) + vint(1) + vint(0) + vint(len(cil)) + cil      # typ 5 = presmerovani: unixovy symlink
            extra = vint(len(zaznam)) + zaznam
        out += hlavicka(2, 0, telo, extra, len(bal)) + bal
    out += hlavicka(5, 0, vint(0))
    return bytes(out)


def b348_program():
    """bootovaci program z ../b348/test_b348_program.h (stejny jako test_b349_jni.cpp)"""
    h = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'b348', 'test_b348_program.h'), encoding='utf-8').read()
    telo = h[h.index('B348_KOD['):]
    telo = telo[telo.index('{') + 1:telo.index('}')]
    return bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', telo))


def atr_b348(sektoru, s_programem):
    """disketa jako v test_b349_jni.cpp: sektor s, bajt i = s*7 + i*13 + (s>>8)*5 + 1"""
    par = sektoru * 128 // 16
    d = bytearray(16 + sektoru * 128)
    d[0:5] = bytes([0x96, 0x02, par & 0xFF, (par >> 8) & 0xFF, 128])
    for s in range(1, sektoru + 1):
        for i in range(128):
            d[16 + (s - 1) * 128 + i] = (s * 7 + i * 13 + (s >> 8) * 5 + 1) & 0xFF
    if s_programem:
        k = b348_program()
        d[16:16 + len(k)] = k
    return bytes(d)


def main():
    rng = random.Random(350)
    uloz = V.uloz

    # ---------- A) rozbaleni ----------
    uloz('rar5_ulozeny.rar', rar5([{'jmeno': 'hra.xex', 'data': V.xex(60)}, {'jmeno': 'navod.txt', 'data': V.TEXT},
                                    {'jmeno': 'Hry', 'slozka': True}, {'jmeno': 'Hry/druha.xex', 'data': V.xex(61)}]),
         [('hra.xex', V.xex(60)), ('navod.txt', V.TEXT), ('Hry/druha.xex', V.xex(61))])

    out, st = bytearray(), Stav()
    toks, filtry = skript(rng, 300000, out, st)
    okno = bytes(out)
    data = filtruj(okno, 0, len(okno), filtry)
    zab = koduj([toks])[0]
    uloz('rar5_kody.rar', rar5([{'jmeno': 'kody.bin', 'data': data, 'zabaleno': zab}]), [('kody.bin', data)])
    druhy = {}
    for t in toks:
        druhy[t[0]] = druhy.get(t[0], 0) + 1
    print('rar5_kody.rar: %d B, tokeny %s, filtry %s, shod nad 256 kB: %d, delka > 1000: %d, zabaleno %d B'
          % (len(data), ' '.join('%s=%d' % kv for kv in sorted(druhy.items())),
             ' '.join('%s=%d' % (n, sum(1 for f in filtry if f[2] == k)) for k, n in ((0, 'DELTA'), (1, 'E8'), (2, 'E8E9'), (3, 'ARM'))),
             sum(1 for t in toks if t[0] == 'match' and t[2] > 0x40000),
             sum(1 for t in toks if t[0] in ('match', 'old') and t[-1 if t[0] == 'old' else 1] > 1000), len(zab)))

    # pevny archiv: 4 soubory jednim proudem; 2. zacina blokem BEZ tabulek (pouzije tabulky 1.), 3. je prazdny
    out, st = bytearray(), Stav()
    t1, f1 = skript(rng, 40000, out, st); k1 = len(out)
    t2, f2 = skript(rng, 30000, out, st); k2 = len(out)
    t4, f4 = skript(rng, 20000, out, st); k4 = len(out)
    okno = bytes(out)
    d1, d2, d4 = filtruj(okno, 0, k1, f1), filtruj(okno, k1, k2, f2), filtruj(okno, k2, k4, f4)
    zab = koduj([t1, t2, [('blok', True)] + t4])
    # 3. soubor (prazdny) nema data vubec (jako WinRAR) - do proudu nepatri
    clen = [{'jmeno': 'pevny1.bin', 'data': d1, 'zabaleno': zab[0]}, {'jmeno': 'pevny2.bin', 'data': d2, 'zabaleno': zab[1]},
            {'jmeno': 'pevny3_prazdny.bin', 'data': b''}, {'jmeno': 'pevny4.bin', 'data': d4, 'zabaleno': zab[2]}]
    uloz('rar5_pevny.rar', rar5(clen, True), [('pevny1.bin', d1), ('pevny2.bin', d2), ('pevny3_prazdny.bin', b''), ('pevny4.bin', d4)])

    # bez CRC32 (jako archiv s otiskem BLAKE2) + odkaz (symlink) + slozka
    hra = V.atr(720, True, 70)
    out, st = bytearray(), Stav()
    zab = koduj([lz_tokeny(hra, out, st)])[0]
    uloz('rar5_bez_crc.rar', rar5([{'jmeno': 'hra.atr', 'data': hra, 'zabaleno': zab, 'crc': False},
                                    {'jmeno': 'odkaz.atr', 'data': b'hra.atr', 'odkaz': 'hra.atr'}]), [('hra.atr', hra)])

    # ---------- B) vyber pro Atari z RAR5 ----------
    pop = [('PoP_DD_final.atr', V.atr(1040, True, 10)), ('PoP_SDA_final.atr', V.atr(720, True, 11)),
           ('PoP_SDB_final.atr', V.atr(720, True, 12)), ('readme.txt', V.TEXT)]
    clen = []
    for jm, d in pop:
        o, s = bytearray(), Stav()
        clen.append({'jmeno': jm, 'data': d, 'zabaleno': koduj([lz_tokeny(d, o, s)])[0]})
    uloz('vyber_pop5.rar', rar5(clen), pop)
    # pevny RAR5: strana B je v archivu prvni (jako vyber_sd_pevny.rar v B349)
    sd = [('PoP_SDB_final.atr', V.atr(720, True, 12)), ('readme.txt', V.TEXT), ('PoP_SDA_final.atr', V.atr(720, True, 11))]
    o, s = bytearray(), Stav()
    tt = [lz_tokeny(d, o, s) for _, d in sd]
    zab = koduj(tt)
    uloz('vyber_sd_pevny5.rar', rar5([{'jmeno': jm, 'data': d, 'zabaleno': z} for (jm, d), z in zip(sd, zab)], True), sd)
    # strany A / B s programem z B348 (JNI test: boot strany A, vymena za B, SIO cte sektor 4 strany B)
    strany = [('Hra (Side B).atr', atr_b348(720, False)), ('navod.txt', b'Vloz stranu A, pri vyzve stranu B.\r\n'), ('Hra (Side A).atr', atr_b348(720, True))]
    clen = []
    for jm, d in strany:
        o, s = bytearray(), Stav()
        clen.append({'jmeno': jm, 'data': d, 'zabaleno': koduj([lz_tokeny(d, o, s)])[0]})
    uloz('vyber_strany5.rar', rar5(clen), strany)
    print('hotovo: %d archivu RAR5 v %s' % (len([f for f in os.listdir(CIL) if f.endswith('.rar')]), CIL))


if __name__ == '__main__':
    main()
