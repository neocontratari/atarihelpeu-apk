#!/usr/bin/env python3
# vyrob_archivy.py CILOVA_SLOZKA - testovaci archivy pro B349 (hry pro Atari v ZIP / RAR)
#
# ZIP: modul zipfile / zlib (cizi implementace deflate = nezavisla kontrola nasi Inflate)
#      + par rucne slozenych ZIPu (ZIP64, heslo, jmena s '\', UTF-8, poskozene).
# RAR4 (verze 29, metoda 0x33): vlastni maly koder RAR 2.9 jen pro testy. Bitovy proud
# zapisuje presne podle rozbalovace unrar (Unpack29 / ReadTables30): Huffmanovy tabulky
# (BC, LD 299, DD 60, LDD 17, RD 28, delta proti stare tabulce, kody 16-19), literaly,
# shody 271+, opakovani 258, stare vzdalenosti 259-262, kratke vzdalenosti 263-270,
# nizke bity vzdalenosti pres LDD vcetne kodu 16 (opakovat 15x), konec bloku 256
# (nova tabulka / konec souboru / konec souboru + nova tabulka), pevny archiv (solid).
# Kazdy RAR odsud rozbali i bsdtar (libarchive) - JAK_SPUSTIT.txt bod 1.
#
# Vedle archivu zapise do CILOVA_SLOZKA/obsah/<archiv>/<soubor> puvodni data (kontrola).
import os, random, struct, sys, zipfile, zlib

CIL = sys.argv[1] if len(sys.argv) > 1 else 'archivy'
os.makedirs(CIL, exist_ok=True)
OBSAH = os.path.join(CIL, 'obsah')
CAS = (2026, 11, 18, 12, 0, 0)
DOS_CAS = (((2026 - 1980) << 9 | 11 << 5 | 18) << 16) | (12 << 11)


def uloz(jmeno, data, clenove):
    with open(os.path.join(CIL, jmeno), 'wb') as f:
        f.write(data)
    for jm, d in clenove:
        cesta = os.path.join(OBSAH, jmeno, jm.replace('\\', '/'))
        os.makedirs(os.path.dirname(cesta), exist_ok=True)
        with open(cesta, 'wb') as f:
            f.write(d)


# ---------------- soubory pro Atari ----------------
def atr(sektoru, boot=True, znacka=0):
    par = sektoru * 128 // 16
    h = bytes([0x96, 0x02, par & 0xFF, (par >> 8) & 0xFF, 128, 0, (par >> 16) & 0xFF]) + bytes(9)
    d = bytearray()
    for s in range(1, sektoru + 1):
        d += bytes(((s * 7 + i * 13 + znacka * 31 + (s >> 8) * 5) & 0xFF) for i in range(128))
    # bootovaci sektor: priznaky, pocet sektoru, adresa nahrani $0700, init $1540
    d[0:6] = bytes([0, 3, 0x00, 0x07, 0x40, 0x15]) if boot else bytes(6)
    return h + bytes(d)


def xex(znacka=0, delka=256):
    telo = bytes(((i * 5 + znacka * 17) & 0xFF) for i in range(delka))
    return b'\xff\xff' + struct.pack('<HH', 0x2000, 0x2000 + delka - 1) + telo + struct.pack('<HHH', 0x02E0, 0x02E1, 0x2000)


def car(znacka=0):
    return b'CART' + struct.pack('>III', 1, 0, 0) + bytes(((i * 3 + znacka) & 0xFF) for i in range(8192))


def xfd(sektoru=720):
    return atr(sektoru, True, 9)[16:]


TEXT = (b'PRINCE OF PERSIA - jak hrat\r\nSipky = pohyb, FIRE = skok.\r\n' * 40)


# ---------------- ZIP ----------------
def zip_py(jmeno, clenove, metoda=zipfile.ZIP_DEFLATED, uroven=6):
    import io
    b = io.BytesIO()
    with zipfile.ZipFile(b, 'w') as z:
        for c in clenove:
            jm, d = c[0], c[1]
            m = c[2] if len(c) > 2 else metoda
            u = c[3] if len(c) > 3 else uroven
            zi = zipfile.ZipInfo(jm, CAS)
            zi.compress_type = m
            z.writestr(zi, d, compress_type=m, compresslevel=u if m == zipfile.ZIP_DEFLATED else None)
    data = b.getvalue()
    uloz(jmeno, data, [(c[0], c[1]) for c in clenove if not c[0].endswith('/')])
    return data


def deflate_raw(d, uroven=6):
    c = zlib.compressobj(uroven, zlib.DEFLATED, -15)
    return c.compress(d) + c.flush()


def zip_rucne(polozky):
    """polozky: (jmeno_bajty, data, metoda 0/8, priznaky, zip64)"""
    out, cd = bytearray(), bytearray()
    for jm, d, metoda, prizn, z64 in polozky:
        komp = d if metoda == 0 else deflate_raw(d)
        crc = zlib.crc32(d)
        ofs = len(out)
        out += struct.pack('<IHHHHHIIIHH', 0x04034B50, 20, prizn, metoda, 0x6000, 0x5D72, crc, len(komp), len(d), len(jm), 0) + jm + komp
        if z64:
            ext = struct.pack('<HHQQ', 1, 16, len(d), len(komp))
            ks, us = 0xFFFFFFFF, 0xFFFFFFFF
        else:
            ext, ks, us = b'', len(komp), len(d)
        cd += struct.pack('<IHHHHHHIIIHHHHHII', 0x02014B50, 20, 45 if z64 else 20, prizn, metoda, 0x6000, 0x5D72, crc, ks, us, len(jm), len(ext), 0, 0, 0, 0x20, ofs) + jm + ext
    n = len(polozky)
    return bytes(out + cd + struct.pack('<IHHHHIIH', 0x06054B50, 0, 0, n, n, len(cd), len(out), 0))


# ---------------- RAR 2.9: konstanty jako v unrar ----------------
NC, DC, LDC, RC, BC = 299, 60, 17, 28, 20
HTS = NC + DC + LDC + RC
LDEC = [0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224]
LBIT = [0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5]
SDDEC = [0, 4, 8, 16, 32, 64, 128, 192]
SDBIT = [2, 2, 3, 4, 5, 6, 6, 6]
DDEC, DBIT = [], []
_v = 0
for _b, _c in enumerate([4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 14, 0, 12]):
    for _ in range(_c):
        DDEC.append(_v); DBIT.append(_b); _v += 1 << _b


def slot_delky(v):
    for k in range(27, -1, -1):
        if LDEC[k] <= v:
            assert v - LDEC[k] < (1 << LBIT[k]), v
            return k, v - LDEC[k]
    raise ValueError(v)


def slot_vzd(v):
    for s in range(59, -1, -1):
        if DDEC[s] <= v:
            assert v - DDEC[s] < (1 << DBIT[s]) or s == 59, v
            return s
    raise ValueError(v)


def slot_kratke(d):
    for i in range(7, -1, -1):
        if SDDEC[i] + 1 <= d:
            assert d - SDDEC[i] - 1 < (1 << SDBIT[i])
            return i
    raise ValueError(d)


class Stav:
    """stav rozbalovace, ktery koder hlida (stejne jako Rar29 v nap_archiv.h / unrar)"""
    def __init__(self):
        self.stare = [0, 0, 0, 0]; self.posledni = 0; self.prev_low = 0; self.low_rep = 0

    def vloz(self, d):
        self.stare = [d] + self.stare[:3]

    def tabulka(self):
        self.prev_low = 0; self.low_rep = 0


def kopiruj(out, delka, vzd):
    for _ in range(delka):
        out.append(out[-vzd])


def aplikuj(t, out, st):
    """provede token na vystupu a stavu presne jako rozbalovac"""
    if t[0] == 'lit':
        out.append(t[1])
    elif t[0] == 'rep':
        kopiruj(out, st.posledni, st.stare[0])
    elif t[0] == 'old':
        k, L = t[1], t[2]
        d = st.stare[k]
        st.stare = [d] + st.stare[:k] + st.stare[k + 1:]
        st.posledni = L
        kopiruj(out, L, d)
    elif t[0] == 'short':
        st.vloz(t[1]); st.posledni = 2
        kopiruj(out, 2, t[1])
    elif t[0] == 'match':
        L, d, s16 = t[1], t[2], t[3]
        v = d - 1
        s = slot_vzd(v)
        if s > 9:
            low = (v - DDEC[s]) & 15
            if st.low_rep > 0:
                assert low == st.prev_low; st.low_rep -= 1
            elif s16:
                assert low == st.prev_low; st.low_rep = 15
            else:
                st.prev_low = low
        st.vloz(d); st.posledni = L
        kopiruj(out, L, d)
    elif t[0] == 'newtab':
        st.tabulka()
    elif t[0] == 'eof':
        if t[1]:                       # dalsi soubor zacne novou tabulkou (ta nuluje nizke bity)
            st.tabulka()


# ---------------- generator tokenu: vsechny kody (data vzniknou z tokenu) ----------------
def skript(rng, kolik, out, st, nova_tabulka_po=60000):
    # Nova tabulka nuluje stav "nizkych bitu" (unrar, 7-Zip, libarchive od 2025). Starsi libarchive
    # (bsdtar 3.7) ho nenuluje -> aby slo vsechno overit i jim: nova tabulka jen mimo opakovani (kod 16)
    # a po ni nejdriv explicitni nizke bity. Presne nulovani zkousi rar_nizke_po_tabulce.rar.
    toks = []
    zac = len(out)
    dalsi_tab = zac + nova_tabulka_po
    po_tabulce = True
    while len(out) - zac < kolik:
        zbyva = kolik - (len(out) - zac)
        n = len(out)
        if n >= dalsi_tab and zbyva > 1000 and st.low_rep == 0:
            t = ('newtab', rng.random() < 0.5)
            aplikuj(t, out, st); toks.append(t)
            dalsi_tab = n + nova_tabulka_po
            po_tabulce = True
            continue
        r = rng.random()
        if n < 16 or r < 0.22:
            for _ in range(min(zbyva, rng.randint(1, 6))):
                t = ('lit', min(255, int(rng.expovariate(0.06))))
                aplikuj(t, out, st); toks.append(t)
            continue
        if r < 0.29 and st.posledni and 0 < st.stare[0] <= n and st.posledni <= zbyva:
            t = ('rep',)
            aplikuj(t, out, st); toks.append(t)
            continue
        if r < 0.42:
            k = rng.randrange(4)
            if 0 < st.stare[k] <= n and zbyva >= 2:
                t = ('old', k, rng.randint(2, min(257, zbyva)))
                aplikuj(t, out, st); toks.append(t)
                continue
        if r < 0.52 and zbyva >= 2:
            t = ('short', rng.randint(1, min(256, n)))
            aplikuj(t, out, st); toks.append(t)
            continue
        c = rng.random()
        if c < 0.3:
            d = rng.randint(1, min(n, 256))
        elif c < 0.6:
            d = rng.randint(1, min(n, 0x1FFF))
        elif c < 0.85:
            d = rng.randint(1, min(n, 0x3FFFF))
        else:
            d = rng.randint(1, n)
        v = d - 1
        s = slot_vzd(v)
        s16 = False
        if s > 9:
            low = (v - DDEC[s]) & 15
            if st.low_rep > 0 and low != st.prev_low:
                v2 = v - low + st.prev_low
                if slot_vzd(v2) != s or v2 + 1 > n:
                    continue
                d = v2 + 1
            elif st.low_rep == 0 and not po_tabulce and rng.random() < 0.35:
                v2 = v - low + st.prev_low                  # stejne nizke bity -> kod 16
                if slot_vzd(v2) == s and v2 + 1 <= n:
                    d = v2 + 1; s16 = True
            if st.low_rep == 0 and not s16:
                po_tabulce = False
        adj = (d >= 0x2000) + (d >= 0x40000)               # podle konecne vzdalenosti
        if zbyva < 3 + adj:
            continue
        t = ('match', rng.randint(3 + adj, min(258 + adj, zbyva)), d, s16)
        aplikuj(t, out, st); toks.append(t)
    return toks


# ---------------- jednoduchy kompresor (data -> tokeny) pro skutecny obsah (ATR ...) ----------------
def lz_tokeny(data, out, st, nova_tabulka_po=65536):
    toks = []
    zac = len(out)
    buf = bytes(out) + data
    hlavy = {}
    def pridej_hash(p):
        if p + 3 <= len(buf):
            hlavy.setdefault(buf[p:p + 3], []).append(p)
    for p in range(max(0, zac - 65536), zac):
        pridej_hash(p)
    p = zac
    dalsi_tab = zac + nova_tabulka_po
    while p < len(buf):
        if p >= dalsi_tab and len(buf) - p > 1000:
            t = ('newtab', True); aplikuj(t, out, st); toks.append(t); dalsi_tab = p + nova_tabulka_po
        nejL, nejD = 0, 0
        maxL = min(257, len(buf) - p)
        for d in st.stare:                                   # stare vzdalenosti (levnejsi)
            if 0 < d <= p:
                L = 0
                while L < maxL and buf[p + L] == buf[p + L - d]:
                    L += 1
                if L > nejL:
                    nejL, nejD = L, d
        for q in reversed(hlavy.get(buf[p:p + 3], [])[-24:]):
            if nejL >= 258:
                break
            d = p - q
            if d > 0x3FFFF:
                break
            L = 0
            m = min(258, len(buf) - p)
            while L < m and buf[p + L] == buf[q + L]:
                L += 1
            if L > nejL + 1 or (L > nejL and nejD not in st.stare):
                nejL, nejD = L, d
        t = None
        if nejL >= 2 and nejD in st.stare and nejL <= 257:
            if nejD == st.stare[0] and nejL == st.posledni:
                t = ('rep',)
            else:
                t = ('old', st.stare.index(nejD), nejL)
        elif nejL == 2 and nejD <= 256:
            t = ('short', nejD)
        elif nejL >= 3 + (nejD >= 0x2000) + (nejD >= 0x40000):
            t = ('match', nejL, nejD, False)
        if t is None:
            t = ('lit', buf[p])
        pred = len(out)
        aplikuj(t, out, st); toks.append(t)
        for q in range(pred, len(out)):
            pridej_hash(q)
        p = len(out)
    assert bytes(out[zac:]) == data
    return toks


# ---------------- bitovy zapis a Huffman ----------------
class Bity:
    def __init__(self):
        self.out = bytearray(); self.acc = 0; self.n = 0

    def put(self, hodnota, bitu):
        for i in range(bitu - 1, -1, -1):
            self.acc = (self.acc << 1) | ((hodnota >> i) & 1); self.n += 1
            if self.n == 8:
                self.out.append(self.acc); self.acc = 0; self.n = 0

    def zarovnej(self):
        if self.n:
            self.put(0, 8 - self.n)

    def data(self):
        self.zarovnej()
        return bytes(self.out)


def delky_huff(cetnost, maxd=15):
    import heapq
    L = [0] * len(cetnost)
    sym = [(f, i) for i, f in enumerate(cetnost) if f > 0]
    if not sym:
        return L
    if len(sym) == 1:
        # jediny symbol: kod musi byt uplny (libarchive neuplny kod odmitne) -> druhy, nepouzity
        L[sym[0][1]] = 1
        L[1 if sym[0][1] == 0 else 0] = 1
        return L
    while True:
        h = [(f, i, (i,)) for f, i in sym]
        heapq.heapify(h)
        hl = {i: 0 for f, i in sym}
        k = len(cetnost)
        while len(h) > 1:
            f1, _, m1 = heapq.heappop(h); f2, _, m2 = heapq.heappop(h)
            for s in m1 + m2:
                hl[s] += 1
            heapq.heappush(h, (f1 + f2, k, m1 + m2)); k += 1
        if max(hl.values()) <= maxd:
            for s, d in hl.items():
                L[s] = d
            return L
        sym = [((f >> 1) + 1, i) for f, i in sym]


def kody(L):
    k = [0] * len(L); c = 0
    for d in range(1, 16):
        for s in range(len(L)):
            if L[s] == d:
                k[s] = c; c += 1
        c <<= 1
    return k


def symboly_useku(useky_toks):
    """tokeny jednoho useku tabulky -> pro kazdy token seznam (tabulka, symbol, [(bitu, hodnota)...])"""
    st = Stav(); st.tabulka()
    vse = []
    for t in useky_toks:
        sy = []
        vse.append(sy)
        if t[0] == 'lit':
            sy.append(('LD', t[1], []))
        elif t[0] == 'rep':
            sy.append(('LD', 258, []))
        elif t[0] == 'old':
            k, x = slot_delky(t[2] - 2)
            sy.append(('LD', 259 + t[1], [])); sy.append(('RD', k, [(LBIT[k], x)]))
        elif t[0] == 'short':
            i = slot_kratke(t[1])
            sy.append(('LD', 263 + i, [(SDBIT[i], t[1] - SDDEC[i] - 1)]))
        elif t[0] == 'match':
            L, d, s16 = t[1], t[2], t[3]
            adj = (d >= 0x2000) + (d >= 0x40000)
            k, x = slot_delky(L - adj - 3)
            sy.append(('LD', 271 + k, [(LBIT[k], x)]))
            v = d - 1; s = slot_vzd(v); x = v - DDEC[s]
            if s > 9:
                ext = [(DBIT[s] - 4, x >> 4)] if DBIT[s] > 4 else []
                sy.append(('DD', s, ext))
                low = x & 15
                if st.low_rep > 0:
                    st.low_rep -= 1
                elif s16:
                    sy.append(('LDD', 16, [])); st.low_rep = 15
                else:
                    sy.append(('LDD', low, [])); st.prev_low = low
            else:
                sy.append(('DD', s, [(DBIT[s], x)] if DBIT[s] else []))
        elif t[0] == 'newtab':
            sy.append(('LD', 256, [(1, 1)]))
        elif t[0] == 'eof':
            sy.append(('LD', 256, [(2, 1 if t[1] else 0)]))
    return vse


TAB_ROZSAH = {'LD': (0, NC), 'DD': (NC, DC), 'LDD': (NC + DC, LDC), 'RD': (NC + DC + LDC, RC)}


def zapis_tabulky(bw, L, stara, drzet):
    bw.zarovnej()
    bw.put(0, 1)                               # LZ, ne PPMd
    bw.put(1 if drzet else 0, 1)               # 1 = delta proti stare tabulce
    zaklad = stara if drzet else [0] * HTS
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
        sy.append(((v - zaklad[i]) & 15, 0, 0)); i += 1
    cet = [0] * BC
    for s, _, _ in sy:
        cet[s] += 1
    bl = delky_huff(cet, 15)
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
    kb = kody(bl)
    for s, nb, x in sy:
        bw.put(kb[s], bl[s])
        if nb:
            bw.put(x, nb)


def koduj(soubory_toks):
    """tokeny souboru (pevny archiv: vic souboru jednim proudem) -> zabalena data souboru"""
    useky, cur = [], []
    for fi, toks in enumerate(soubory_toks):
        for t in toks:
            cur.append((fi, t))
            if t[0] == 'newtab' or (t[0] == 'eof' and t[1]):
                useky.append(cur); cur = []
    if cur:
        useky.append(cur)
    vysledky = []
    bw = Bity()
    stara = [0] * HTS
    drzet_dalsi = False
    for u in useky:
        sy = symboly_useku([t for _, t in u])
        cet = {k: [0] * n for k, (_, n) in TAB_ROZSAH.items()}
        for st_ in sy:
            for tab, s, _ in st_:
                cet[tab][s] += 1
        L = [0] * HTS
        for k, (o, n) in TAB_ROZSAH.items():
            L[o:o + n] = delky_huff(cet[k], 15)
        zapis_tabulky(bw, L, stara, drzet_dalsi)
        stara = L
        kk = {k: kody(L[o:o + n]) for k, (o, n) in TAB_ROZSAH.items()}
        ll = {k: L[o:o + n] for k, (o, n) in TAB_ROZSAH.items()}
        for (fi, t), st_ in zip(u, sy):
            for tab, s, ext in st_:
                assert ll[tab][s] > 0
                bw.put(kk[tab][s], ll[tab][s])
                for nb, x in ext:
                    if nb:
                        bw.put(x, nb)
            if t[0] == 'newtab':
                drzet_dalsi = t[1]
            if t[0] == 'eof':
                vysledky.append(bw.data()); bw = Bity()
                drzet_dalsi = len(t) > 2 and t[2]
    return vysledky


def rar4(clenove, pevny=False):
    """clenove: (jmeno, data, zabalena_data nebo None = ulozeno)"""
    out = bytearray(b'Rar!\x1a\x07\x00')
    telo = struct.pack('<BHHHI', 0x73, 0x0008 if pevny else 0, 13, 0, 0)
    out += struct.pack('<H', zlib.crc32(telo) & 0xFFFF) + telo
    for i, (jm, d, z) in enumerate(clenove):
        nb = jm.encode('ascii')
        metoda, ver = (0x30, 20) if z is None else (0x33, 29)
        bal = d if z is None else z
        fl = 0x8000 | 0x0080 | (0x0010 if pevny and i > 0 else 0)
        telo = struct.pack('<BHHIIBIIBBHI', 0x74, fl, 32 + len(nb), len(bal), len(d), 2, zlib.crc32(d), DOS_CAS, ver, metoda, len(nb), 0x20) + nb
        out += struct.pack('<H', zlib.crc32(telo) & 0xFFFF) + telo + bal
    telo = struct.pack('<BHH', 0x7B, 0x4000, 7)
    out += struct.pack('<H', zlib.crc32(telo) & 0xFFFF) + telo
    return bytes(out)


def rar_lz(clenove, pevny=False):
    """clenove: (jmeno, data) -> komprimovany RAR (pevny = jeden proud)"""
    out = bytearray(); st = Stav()
    toks_vse, vysl = [], []
    for jm, d in clenove:
        if not pevny:
            out = bytearray(); st = Stav()
        toks = lz_tokeny(d, out, st) + [('eof', False)]
        if pevny:
            toks_vse.append(toks)
        else:
            vysl.append(koduj([toks])[0])
    if pevny:
        vysl = koduj(toks_vse)
    return rar4([(jm, d, z) for (jm, d), z in zip(clenove, vysl)], pevny)


def main():
    rng = random.Random(349)
    nahodna = bytes(rng.getrandbits(8) for _ in range(20000))

    # ---------- A) rozbaleni ----------
    zip_py('zip_ulozeny.zip', [('nahodna.bin', nahodna[:3000]), ('text.txt', TEXT), ('prazdny.bin', b'')], zipfile.ZIP_STORED)
    zip_py('zip_deflate.zip', [
        ('text_6.txt', TEXT * 20),                                        # dynamicke Huffmanovy bloky
        ('kratky.txt', b'ahoj ahoj ahoj atari'),                          # pevne (fixed) bloky
        ('nahodna.bin', nahodna),                                          # ulozene bloky (nejde zmensit)
        ('ulozene_bloky.bin', TEXT * 3, zipfile.ZIP_DEFLATED, 0),         # uroven 0 = jen ulozene bloky
        ('uroven1.atr', atr(720, True, 1), zipfile.ZIP_DEFLATED, 1),
        ('uroven9.atr', atr(1040, True, 2), zipfile.ZIP_DEFLATED, 9),
        ('nuly_17MB.bin', bytes(17 << 20), zipfile.ZIP_DEFLATED, 9),      # dlouhe shody, vzdalenost 32 kB
        ('prazdny.bin', b''),
        ('slozka/', b''),
    ])
    z = bytearray(zip_py('zip_poskozeny_crc.zip', [('hra.xex', xex(1), zipfile.ZIP_STORED), ('hra2.xex', xex(2), zipfile.ZIP_STORED)]))
    z[30 + len('hra.xex') + 40] ^= 0x55                                    # bajt v datech prvniho souboru
    uloz('zip_poskozeny_crc.zip', bytes(z), [])
    uloz('zip_zkraceny.zip', z[:len(z) * 6 // 10], [])
    uloz('zip64.zip', zip_rucne([(b'hra.atr', atr(720, True, 3), 8, 0, True), (b'Hry\\druha.xex', xex(4), 0, 0, False)]),
         [('hra.atr', atr(720, True, 3)), ('Hry/druha.xex', xex(4))])
    uloz('zip_heslo.zip', zip_rucne([(b'tajna.atr', atr(720, True, 5), 0, 1, False)]), [])
    uloz('zip_heslo_a_hra.zip', zip_rucne([(b'tajna.atr', atr(720, True, 5), 0, 1, False), (b'volna.xex', xex(5), 8, 0, False)]),
         [('volna.xex', xex(5))])

    ulozeny = rar4([('hra.xex', xex(6), None), ('navod.txt', TEXT, None)])
    uloz('rar_ulozeny.rar', ulozeny, [('hra.xex', xex(6)), ('navod.txt', TEXT)])

    # komprimovany RAR se vsemi kody (data vzniknou z tokenu): 300 kB, vzdalenosti i nad 256 kB
    out, st = bytearray(), Stav()
    toks = skript(rng, 300000, out, st) + [('eof', False)]
    pouzite = sorted({t[0] for t in toks})
    dlouhe = sum(1 for t in toks if t[0] == 'match' and t[2] >= 0x40000)
    s16 = sum(1 for t in toks if t[0] == 'match' and t[3])
    data_a = bytes(out)
    uloz('rar_kody.rar', rar4([('kody.bin', data_a, koduj([toks])[0])]), [('kody.bin', data_a)])
    print('rar_kody.rar: %d tokenu (%s), shod nad 256 kB: %d, kod 16 (nizke bity): %d, novych tabulek: %d'
          % (len(toks), ' '.join(pouzite), dlouhe, s16, sum(1 for t in toks if t[0] == 'newtab')))

    # pevny RAR: 4 soubory jednim proudem (2. konci "01" = nova tabulka v dalsim souboru, 3. je prazdny)
    out, st = bytearray(), Stav()
    f1 = skript(rng, 40000, out, st) + [('eof', False)]; d1 = bytes(out)
    f2 = skript(rng, 30000, out, st) + [('eof', True, True)]; d2 = bytes(out[len(d1):])
    f3 = [('eof', False)]; d3 = b''
    f4 = skript(rng, 20000, out, st) + [('eof', False)]; d4 = bytes(out[len(d1) + len(d2):])
    zab = koduj([f1, f2, f3, f4])
    clen = [('pevny1.bin', d1), ('pevny2.bin', d2), ('pevny3_prazdny.bin', d3), ('pevny4.bin', d4)]
    uloz('rar_pevny.rar', rar4([(j, d, z) for (j, d), z in zip(clen, zab)], True), clen)

    # kod 16 (opakovat nizke bity) hned po nove tabulce: nizke bity jsou po tabulce 0, ne 11 z minula
    out, st = bytearray(), Stav()
    toks = [('lit', rng.getrandbits(8)) for _ in range(600)]
    toks += [('match', 10, 300, False), ('newtab', False), ('match', 10, 289, True)]
    toks += [('lit', rng.getrandbits(8)) for _ in range(20)] + [('eof', False)]
    for t in toks:
        aplikuj(t, out, st)
    uloz('rar_nizke_po_tabulce.rar', rar4([('nizke.bin', bytes(out), koduj([toks])[0])]), [('nizke.bin', bytes(out))])

    uloz('rar5.rar', b'Rar!\x1a\x07\x01\x00' + bytes(64), [])
    uloz('rar_zkraceny.rar', ulozeny[:len(ulozeny) // 2], [])

    # ---------- B) vyber pro Atari ----------
    pop = [('PoP_DD_final.atr', atr(1040, True, 10)), ('PoP_SDA_final.atr', atr(720, True, 11)),
           ('PoP_SDB_final.atr', atr(720, True, 12)), ('readme.txt', TEXT)]
    uloz('vyber_pop.rar', rar_lz(pop), pop)
    sd = [('PoP_SDB_final.atr', atr(720, True, 12)), ('readme.txt', TEXT), ('PoP_SDA_final.atr', atr(720, True, 11))]
    uloz('vyber_sd_pevny.rar', rar_lz(sd, True), sd)
    zip_py('vyber_disky.zip', [('Hra Disk 3.atr', atr(720, False, 23)), ('Hra Disk 1.atr', atr(720, True, 21)),
                               ('Hra Disk 2.atr', atr(720, False, 22))])
    zip_py('vyber_strany.zip', [('Hra (Side B).atr', atr(720, False, 32)), ('Hra (Side A).atr', atr(720, True, 31))])
    zip_py('vyber_xex_readme.zip', [('hra.xex', xex(40)), ('navod.txt', TEXT), ('obrazek.png', b'\x89PNG' + bytes(100))])
    zip_py('vyber_data_a_xex.zip', [('data.atr', atr(720, False, 41)), ('zavadec.xex', xex(41))])
    zip_py('vyber_pal_ntsc.zip', [('Hra (NTSC).xex', xex(42)), ('Hra (PAL).xex', xex(43))])
    vnitrni = zip_py('vyber_vnitrni.zip', [('hra.xex', xex(44))])
    zip_py('vyber_vnoreny.zip', [('info.txt', TEXT), ('vnitrni.zip', vnitrni, zipfile.ZIP_STORED)])
    zip_py('vyber_vnoreny_rar.zip', [('hra.rar', rar_lz([('hra.atr', atr(720, True, 45))]), zipfile.ZIP_STORED)])
    zip_py('vyber_xfd.zip', [('hra.xfd', xfd())])
    zip_py('vyber_macosx.zip', [('__MACOSX/._hra.atr', b'\x00\x05\x16\x07' + bytes(60)), ('._hra.atr', b'\x00\x05\x16\x07' + bytes(60)),
                                ('hra.atr', atr(720, True, 46))])
    zip_py('vyber_jen_readme.zip', [('readme.txt', TEXT), ('obal.jpg', bytes(500))])
    zip_py('vyber_cartridge.zip', [('hra.car', car(47)), ('hra.txt', TEXT)])
    zip_py('vyber_dve_hry.zip', [('Boulder.atr', atr(720, True, 48)), ('Jetboy.atr', atr(1040, True, 49))])
    zip_py('vyber_slozky.zip', [('Hry/', b''), ('Hry/Atari/', b''), ('Hry/Atari/hra.atr', atr(720, True, 50))])
    zip_py('vyber_velky.zip', [('obrovsky.bin', bytes(17 << 20), zipfile.ZIP_DEFLATED, 9), ('hra.xex', xex(51))])
    zip_py('vyber_neznamy.zip', [('hra.bin', bytes(range(256)) * 4), ('hra.xex', xex(52))])
    uloz('vyber_jmena.zip', zip_rucne([('Hra – Řež.xex'.encode('utf-8'), xex(53), 8, 0x800, False),
                                       (b'tab\tnovy\nradek.xex', xex(54), 8, 0, False)]), [])
    print('hotovo: %d archivu v %s' % (len([f for f in os.listdir(CIL) if not os.path.isdir(os.path.join(CIL, f))]), CIL))


if __name__ == '__main__':
    main()
