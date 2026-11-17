#!/usr/bin/env python3
"""B335: VODOROVNE CARKY V DUNE 2000 NA TV - SKUTECNA STRANKA WEB TV V CHROMIU (dekoder JPEG prohlizece).

Rene (B333 log, fotka B332): "v dune vodorovne carky zustaly ale mazou se" - slabe zelene carky na pisku
po ceste jednotek se zelenym pruhem zdravi, na radcich obrazu 16k-1.

  - stranka = PRESNE ta z appky (vytahne se z MainActivity.napTvWebWriteHtml),
  - proud = SKUTECNY Hub + KodekJpeg z appky (gen_proud_b335: scena Dune ve 2x, kvalita 55 = slaba Wi-Fi,
    600 snimku jednotky jdou, 120 stoji), jednou jako B334 (barvy 4:2:0), jednou B335,
  - Python server = co dela Java (/status proud=ps1, /ps1.stream), pakety tempem hry; v kontrolnich bodech
    pocka, az stranka vse nakresli, a obsah canvasu porovna se snimkem hry.
Kontroly:
  1) stranka si rekne o proud s volbami (v >= 319)
  2) B334 v Chromiu: carky na radcich 16k-1 (prohlizec opravdu dopocitava barvy 4:2:0 pres hranici
     ctvercu - pricina potvrzena ve skutecnem prohlizeci, ne jen v C++)
  3) B335: zadna carka v zadnem kontrolnim bode; po zastaveni cisto oboje (B334 = "mazou se")
  4) obraz B335 v canvasu = snimek hry (PSNR, nejhorsi ctverec 16x16)
  5) zadna chyba JavaScriptu ani v datech
Pouziti: python3 test_b335_prohlizec.py PRAC   (PRAC/b334, PRAC/b335 z gen_proud_b335)
         -> VYSTUP_test_b335_prohlizec.txt
"""
import base64
import http.server
import os
import sys
import threading
import time
import urllib.parse

import numpy as np
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
W, H, VW = 736, 480, 592
vystup = []
kontrol = chyb = 0


def tisk(s):
    print(s, flush=True)
    vystup.append(s)


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    tisk(('OK     ' if ok else 'CHYBA  ') + co)


def stranka_z_javy(src):
    """HTML prohlizece presne tak, jak ho sklada MainActivity.napTvWebWriteHtml (String body = ...;)."""
    i = src.index('String body = ', src.index('private void napTvWebWriteHtml(OutputStream out)')) + len('String body = ')
    out = []
    while True:
        while src[i] in ' \t\r\n+':
            i += 1
        if src.startswith('//', i):
            i = src.index('\n', i)
            continue
        if src[i] == ';':
            break
        if src[i] != '"':
            raise RuntimeError('neocekavany vyraz v HTML: ' + src[i:i + 60])
        i += 1
        b = []
        while src[i] != '"':
            if src[i] == '\\':
                n = src[i + 1]
                if n == 'u':
                    b.append(chr(int(src[i + 2:i + 6], 16)))
                    i += 6
                    continue
                b.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}[n])
                i += 2
            else:
                b.append(src[i])
                i += 1
        i += 1
        out.append(''.join(b))
    return ''.join(out)


HTML = stranka_z_javy(open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read())


class Proud:
    def __init__(self, adr):
        self.data = open(os.path.join(adr, 'proud.bin'), 'rb').read()
        self.snimky = []          # (konec v proud.bin, hlavnich paketu)
        for r in open(os.path.join(adr, 'snimky.txt')):
            i, k, h, ko, po = map(int, r.split())
            self.snimky.append((k, h))
        self.adr = adr

    def obraz(self, i):
        return np.frombuffer(open(os.path.join(self.adr, 'snimek_%d.raw' % i), 'rb').read(), dtype='<u4')


def server(stav, dotazy, log, pr):
    T0 = time.time()

    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def posli(self, kod, typ, data):
            try:
                self.send_response(kod)
                self.send_header('Content-Type', typ)
                self.send_header('Content-Length', str(len(data)))
                self.send_header('Cache-Control', 'no-store')
                self.end_headers()
                self.wfile.write(data)
            except Exception:
                pass

        def do_GET(self):
            p = self.path.split('?')[0]
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            dotazy.append((round(time.time() - T0, 2), self.path))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', HTML.encode('utf-8'))
            if p == '/status':
                t = 'running=true atari=0 atariCpp=0 atariProud=0 proud=ps1 sega=0 ps1=1 ps1mys=0 menu=0 seq=1 h264Seq=0 url=x\n'
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                log.append((round(time.time() - T0, 2), q.get('m', [''])[0]))
                return self.posli(200, 'text/plain', b'ok')
            if p in ('/quality', '/klavesa', '/klavesy', '/mys'):
                return self.posli(200, 'text/plain', b'')
            if p == '/ps1.stream':
                stav['v'] = int(q.get('v', ['0'])[0] or 0)
                stav['pripojeni'] += 1
                if stav['pripojeni'] > 1:
                    stav['znovu'] = True
                    return self.posli(409, 'text/plain', b'proud=0\n')
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                    self.wfile.flush()
                    zac = 0
                    for i, (konec, _) in enumerate(pr.snimky):
                        self.wfile.write(pr.data[zac:konec])
                        self.wfile.flush()
                        zac = konec
                        if i in stav['body']:
                            stav['ceka'].set()
                            stav['dal'].wait(30)
                            stav['dal'].clear()
                        else:
                            time.sleep(1 / 120)
                    stav['hotovo'] = True
                    while not stav['konec']:
                        time.sleep(0.1)
                except Exception:
                    pass
                return
            return self.posli(410 if p in ('/frame.jpg', '/stream.mjpg', '/stream.h264', '/audio.raw') else 404, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def canvas(pg):
    r = pg.evaluate("""() => {var a=window.napAp,d=a.cx.getImageData(0,0,a.W,a.H).data,s='',k=0x8000;
        for(var i=0;i<d.length;i+=k)s+=String.fromCharCode.apply(null,d.subarray(i,Math.min(d.length,i+k)));
        return [a.W,a.H,btoa(s)];}""")
    w, h, b64 = r
    d = np.frombuffer(base64.b64decode(b64), dtype=np.uint8).reshape(h * w, 4).astype(np.uint32)
    return w, h, (d[:, 0] << 16) | (d[:, 1] << 8) | d[:, 2]


def rgb(a):
    return np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255], -1).reshape(H, W, 3).astype(np.float64)


def carky(c, f):
    """stejne jako test_b335_kodek.cpp: usek 8 bodu v radku o hodne zelenejsi nez hra (> 24), ve hre
    v okoli 3 radku zadna zelena -> (pocet, z toho na radku 16k-1, nejsilnejsi)"""
    C, F = rgb(c)[:, :VW], rgb(f)[:, :VW]
    zel = (F[:, :, 1] > F[:, :, 0] + 20)
    zs = zel.reshape(H, VW // 8, 8).any(-1)
    okoli = np.zeros_like(zs)
    for d in range(-3, 4):
        okoli |= np.roll(zs, d, axis=0)
    d = C - F
    g = (d[:, :, 1] - (d[:, :, 0] + d[:, :, 2]) / 2).reshape(H, VW // 8, 8).mean(-1)
    g[:3] = 0
    g[H - 3:] = 0
    m = (g > 24) & ~okoli
    ys = np.nonzero(m)[0]
    return int(m.sum()), int(np.sum((ys & 15) == 15)), float(g[~okoli].max())


def srovnej(a, b):
    A, B = rgb(a), rgb(b)
    mse = np.mean((A - B) ** 2)
    ps = 99.0 if mse < 1e-9 else 10 * np.log10(255 * 255 / mse)
    e = np.abs(A - B).mean(-1)
    ct = e.reshape(H // 16, 16, W // 16, 16).mean((1, 3))
    return ps, float(ct.max())


def beh(b, pr, body):
    stav = {'konec': False, 'body': set(body), 'ceka': threading.Event(), 'dal': threading.Event(),
            'hotovo': False, 'pripojeni': 0, 'znovu': False, 'v': -1}
    dotazy, log, chybyJs = [], [], []
    srv = server(stav, dotazy, log, pr)
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto('http://127.0.0.1:%d/' % srv.server_address[1])
    vysl = []
    for i in body:
        if not stav['ceka'].wait(60):
            vysl.append((i, None))
            break
        stav['ceka'].clear()
        cil = sum(s[1] for s in pr.snimky[:i + 1])
        t0 = time.time()
        while time.time() - t0 < 15:
            st = pg.evaluate("[window.napAp.pak, window.napAp.jq.length, window.napAp.jbusy]")
            if st[0] >= cil and st[1] == 0 and not st[2]:
                break
            time.sleep(0.02)
        w, h, c = canvas(pg)
        f = pr.obraz(i)
        vysl.append((i, carky(c, f) + srovnej(c, f), c))
        stav['dal'].set()
    t0 = time.time()
    while not stav['hotovo'] and time.time() - t0 < 60:
        time.sleep(0.05)
    citace = pg.evaluate("[window.napAp.jpg, window.napAp.chyb, window.napAp.pak]")
    stav['konec'] = True
    ctx.close()
    srv.shutdown()
    return stav, dotazy, chybyJs, vysl, citace


def main():
    adr = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ZDE, 'prac')
    p334, p335 = Proud(os.path.join(adr, 'b334')), Proud(os.path.join(adr, 'b335'))
    tisk('B335 - Dune 2000 na TV v Chromiu (skutecna stranka WEB TV, dekoder JPEG prohlizece): vodorovne carky')
    tisk('stranka z MainActivity.java (%d znaku); proud B334 %.2f MB, B335 %.2f MB (%d snimku 736x480, kvalita 55)'
         % (len(HTML), len(p334.data) / 1e6, len(p335.data) / 1e6, len(p335.snimky)))
    body = [150, 300, 450, 599, 719]
    with sync_playwright() as p:
        b = p.chromium.launch()
        st4, dot4, js4, v4, cit4 = beh(b, p334, body)
        st5, dot5, js5, v5, cit5 = beh(b, p335, body)
        b.close()
    over(st4['v'] >= 319 and st5['v'] >= 319, '1) stranka si rekne o proud s volbami: v=%d' % st5['v'])
    for nazev, v in (('B334', v4), ('B335', v5)):
        for i, r, _ in v:
            if r:
                tisk('   %s snimek %3d%s: carek %3d (na radku 16k-1 %3d), nejsilnejsi %.1f | PSNR %.2f dB, nejhorsi ctverec %.1f'
                     % (nazev, i, ' (stoji)' if i >= 600 else '', r[0], r[1], r[2], r[3], r[4]))
    pohyb4 = [r for i, r, _ in v4 if r and i < 600]
    pohyb5 = [r for i, r, _ in v5 if r and i < 600]
    k4 = [r for i, r, _ in v4 if r and i >= 600]
    k5 = [r for i, r, _ in v5 if r and i >= 600]
    n4 = pohyb4[-1][0] if pohyb4 else 0
    r15 = pohyb4[-1][1] if pohyb4 else 0
    over(len(pohyb4) == 4 and n4 >= 50 and r15 * 10 >= n4 * 8,
         '2) B334 ve skutecnem Chromiu: %d carek na pisku, %d %% na radcich 16k-1 (jako na fotce) - prohlizec dopocitava barvy 4:2:0'
         ' pres hranici ctvercu' % (n4, r15 * 100 // max(1, n4)))
    over(len(pohyb5) == 4 and all(r[0] == 0 for r in pohyb5) and k4 and k5 and k4[0][0] == 0 and k5[0][0] == 0,
         '3) B335: zadna carka ve %d kontrolnich bodech behem pohybu (nejsilnejsi %.1f); po zastaveni cisto B334 i B335 ("mazou se")'
         % (len(pohyb5), max(r[2] for r in pohyb5) if pohyb5 else -1))
    spatne = [(i, r) for i, r, _ in v5 if not r or r[3] < 28.0 or r[4] > 15.0]
    over(len(v5) == len(body) and not spatne,
         '4) obraz B335 v canvasu = snimek hry ve %d kontrolnich bodech (PSNR nejmene %.2f dB, nejhorsi ctverec nejvys %.1f)'
         % (len(v5), min(r[3] for _, r, _ in v5 if r), max(r[4] for _, r, _ in v5 if r)))
    over(not js4 and not js5 and cit4[1] == 0 and cit5[1] == 0 and not st4['znovu'] and not st5['znovu'],
         '5) zadna chyba JavaScriptu (%s), chyb v datech %d / %d, proud se neotviral znovu' % ((js4 + js5)[:2] or 'zadna', cit4[1], cit5[1]))
    # vyrez pro oci (PRAC/carky_b334_b335.png): nahore B334, dole B335, snimek 599
    try:
        from PIL import Image
        c4 = [c for i, r, c in v4 if i == 599][0]
        c5 = [c for i, r, c in v5 if i == 599][0]
        obr = np.concatenate([rgb(c4)[:, :VW], np.full((8, VW, 3), 255.0), rgb(c5)[:, :VW]], 0).astype(np.uint8)
        Image.fromarray(obr).save(os.path.join(adr, 'carky_b334_b335.png'))
        tisk('   obrazek: PRAC/carky_b334_b335.png (nahore B334, dole B335, snimek 599)')
    except Exception as e:
        tisk('   (obrazek nejde: %s)' % e)
    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b335_prohlizec.txt'), 'w').write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
