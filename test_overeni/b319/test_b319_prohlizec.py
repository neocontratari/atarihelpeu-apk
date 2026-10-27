#!/usr/bin/env python3
"""B319: STRANKA WEB TV V CHROMIU - PROUD PS1 S KOPII (POSUN OBRAZU) A JPEG V POLOVICNIM ROZLISENI.

Rene: "mam pocit, ze doslo ke zpomaleni pri prenosu LAN". Log B318: Dune 2000 na PC 48-57 snimku/s
misto 59 - pri posunu mapy cely obraz 736x480 20x za sekundu (az 6 Mbit/s), Wi-Fi nestihala.

  - stranka = PRESNE ta z appky (vytahne se z MainActivity.napTvWebWriteHtml),
  - proud = SKUTECNY Hub + KodekJpeg z appky (gen_proud_b319: 2D strategie jako Dune 2000 v 2x
    Enhanced Resolution, posun mapy ruznymi smery, prohlizec obcas vynecha snimek); stranka B319 si
    v adrese rekne v=319 -> server posle proud s volbami B319 (jako Java: napTvWebQueryLong v >= 319),
  - Python server = co dela Java (/status, /ps1.stream, /clientlog ...), pakety posila tempem hry
    a v kontrolnich bodech pocka, az stranka vse nakresli - pak se obsah canvasu porovna se snimkem.
Kontroly:
  1) stranka si rekne o proud B319 (v=319), stara stranka B318 ne (dostane proud jako drive)
  2) obraz v canvasu = snimek hry v kontrolnich bodech uprostred posunu (kopie a JPEG ve spravnem
     poradi i pri rozbalovani JPEG na pozadi) - PSNR, nejhorsi ctverec 16x16
  3) kopie se opravdu delaly (ap.kop) a JPEG v polovicnim rozliseni se kreslil 2x bez vyhlazeni
     (2x2 body v canvasu stejne)
  4) dat B319 proti drivejsku (stejne snimky)
  5) stara stranka B318 se starym proudem: obraz spravne (nic se pro ni nezmenilo)
  6) zadna chyba JavaScriptu, log prohlizece do appky ("B319: kopii ...")
Pouziti: python3 test_b319_prohlizec.py <adresar s proud_v3 a proud_v0>   -> VYSTUP_test_b319_prohlizec.txt
"""
import base64
import http.server
import os
import struct
import subprocess
import sys
import threading
import time
import urllib.parse

import numpy as np
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
B318 = 'e465b07'
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


JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
CZ = stranka_z_javy(JAVA)
STARA = stranka_z_javy(subprocess.run(['git', '-C', KOREN, 'show', B318 + ':app/src/main/java/eu/atarihelp/emu10/MainActivity.java'],
                                      capture_output=True, text=True, check=True).stdout)


class Proud:
    def __init__(self, adr):
        self.data = open(os.path.join(adr, 'proud.bin'), 'rb').read()
        self.snimky = []          # (konec v proud.bin, hlavnich paketu, kopii, polovicnich)
        for r in open(os.path.join(adr, 'snimky.txt')):
            i, k, h, ko, po = map(int, r.split())
            self.snimky.append((k, h, ko, po))
        self.adr = adr

    def obraz(self, i):
        a = np.frombuffer(open(os.path.join(self.adr, 'snimek_%d.raw' % i), 'rb').read(), dtype='<u4')
        return a


def server(html, stav, dotazy, log, proudy):
    T0 = time.time()

    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def posli(self, kod, typ, data, hl=None):
            try:
                self.send_response(kod)
                self.send_header('Content-Type', typ)
                self.send_header('Content-Length', str(len(data)))
                self.send_header('Cache-Control', 'no-store')
                for k, v in (hl or {}).items():
                    self.send_header(k, v)
                self.end_headers()
                self.wfile.write(data)
            except Exception:
                pass

        def do_GET(self):
            p = self.path.split('?')[0]
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            dotazy.append((round(time.time() - T0, 2), self.path))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', html.encode('utf-8'))
            if p == '/status':
                t = 'running=true atari=0 atariCpp=0 atariProud=0 proud=ps1 sega=0 ps1=1 ps1mys=0 menu=0 seq=1 h264Seq=0 url=x\n'
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                log.append((round(time.time() - T0, 2), q.get('m', [''])[0]))
                return self.posli(200, 'text/plain', b'ok')
            if p in ('/quality', '/klavesa', '/klavesy', '/audio.raw', '/mys'):
                return self.posli(200, 'text/plain', b'')
            if p == '/frame.jpg':
                return self.posli(503, 'text/plain', b'frame not ready\n')
            if p == '/stream.mjpg':
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=napframe\r\n\r\n')
                    self.wfile.flush()
                    while not stav['konec']:
                        time.sleep(0.1)
                except Exception:
                    pass
                return
            if p == '/ps1.stream':
                # jako Java (napTvWebHandleClient): v >= 319 -> volby 3, jinak proud jako drive
                v = int(q.get('v', ['0'])[0] or 0)
                pr = proudy[3 if v >= 319 else 0]
                stav['volby'] = 3 if v >= 319 else 0
                stav['pripojeni'] += 1
                if stav['pripojeni'] > 1:          # opakovane pripojeni (chyba v datech) - jen hlasit, nic neposilat
                    stav['znovu'] = True
                    return self.posli(409, 'text/plain', b'proud=0\n', {'X-Nap-Proud': 'ps1'})
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                    self.wfile.flush()
                    zac = 0
                    for i, (konec, _, _, _) in enumerate(pr.snimky):
                        self.wfile.write(pr.data[zac:konec])
                        self.wfile.flush()
                        zac = konec
                        stav['poslano'] = i
                        if i in stav['body']:
                            stav['ceka'].set()
                            stav['dal'].wait(20)
                            stav['dal'].clear()
                        else:
                            time.sleep(stav['tempo'])
                    stav['hotovo'] = True
                    while not stav['konec']:
                        time.sleep(0.1)
                except Exception:
                    pass
                return
            return self.posli(404, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def canvas(pg):
    """obsah canvasu proudu (RGBA) jako numpy pole W*H uint32 0x00RRGGBB"""
    r = pg.evaluate("""() => {var a=window.napAp,d=a.cx.getImageData(0,0,a.W,a.H).data,s='',k=0x8000;
        for(var i=0;i<d.length;i+=k)s+=String.fromCharCode.apply(null,d.subarray(i,Math.min(d.length,i+k)));
        return [a.W,a.H,btoa(s)];}""")
    W, H, b64 = r
    d = np.frombuffer(base64.b64decode(b64), dtype=np.uint8).reshape(H * W, 4).astype(np.uint32)
    return W, H, (d[:, 0] << 16) | (d[:, 1] << 8) | d[:, 2]


def srovnej(a, b, W, H):
    """PSNR a nejhorsi ctverec 16x16 (prumerna absolutni chyba)"""
    A = np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255], -1).reshape(H, W, 3).astype(np.float64)
    B = np.stack([(b >> 16) & 255, (b >> 8) & 255, b & 255], -1).reshape(H, W, 3).astype(np.float64)
    mse = np.mean((A - B) ** 2)
    ps = 99.0 if mse < 1e-9 else 10 * np.log10(255 * 255 / mse)
    e = np.abs(A - B).mean(-1)
    bh, bw = H // 16, W // 16
    ct = e[:bh * 16, :bw * 16].reshape(bh, 16, bw, 16).mean((1, 3))
    return ps, float(ct.max())


def zdvojeni(c, W, H, x1):
    """podil ctveric 2x2 se stejnou barvou (mapa vlevo, x < x1)"""
    a = c.reshape(H, W)[:, :x1]
    return float(np.mean((a[0::2, 0::2] == a[0::2, 1::2]) & (a[0::2, 0::2] == a[1::2, 0::2]) & (a[0::2, 0::2] == a[1::2, 1::2])))


def beh(b, html, proudy, body, popis):
    stav = {'konec': False, 'body': set(body), 'ceka': threading.Event(), 'dal': threading.Event(), 'tempo': 1 / 60,
            'poslano': -1, 'hotovo': False, 'pripojeni': 0, 'znovu': False, 'volby': -1}
    dotazy, log, chybyJs = [], [], []
    srv = server(html, stav, dotazy, log, proudy)
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto('http://127.0.0.1:%d/' % srv.server_address[1])
    vysl = []
    for i in body:
        if not stav['ceka'].wait(30):
            vysl.append((i, None))
            break
        stav['ceka'].clear()
        pr = proudy[stav['volby']]
        cil = sum(s[1] for s in pr.snimky[:i + 1])         # hlavnich paketu do snimku i
        t0 = time.time()
        while time.time() - t0 < 10:
            st = pg.evaluate("[window.napAp.pak, window.napAp.jq.length, window.napAp.jbusy]")
            if st[0] >= cil and st[1] == 0 and not st[2]:
                break
            time.sleep(0.02)
        W, H, c = canvas(pg)
        ps, ct = srovnej(c, pr.obraz(i), W, H)
        vysl.append((i, (ps, ct, zdvojeni(c, W, H, 592), st)))
        stav['dal'].set()
    t0 = time.time()
    while not stav['hotovo'] and time.time() - t0 < 30:
        time.sleep(0.05)
    time.sleep(0.3)
    citace = pg.evaluate("[window.napAp.kop, window.napAp.pol, window.napAp.jpg, window.napAp.chyb, window.napAp.pak]")
    # log prohlizece (stranka ho posila kazdych 30 s od nacteni) - pockat na nej
    if popis == 'B319':
        t0 = time.time()
        while time.time() - t0 < 35 and not any('PROHLIZEC PS1' in m for _, m in log):
            time.sleep(0.25)
    stav['konec'] = True
    ctx.close()
    srv.shutdown()
    return stav, dotazy, log, chybyJs, vysl, citace


def main():
    adr = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ZDE, 'prac')
    proudy = {3: Proud(os.path.join(adr, 'proud_v3')), 0: Proud(os.path.join(adr, 'proud_v0'))}
    tisk('B319 - stranka WEB TV v Chromiu: proud PS1 s kopii (posun obrazu) a JPEG v polovicnim rozliseni')
    tisk('stranka z MainActivity.java (%d znaku), B318 z %s (%d znaku); proud B319 %.2f MB, drive %.2f MB (%d snimku 736x480)'
         % (len(CZ), B318, len(STARA), len(proudy[3].data) / 1e6, len(proudy[0].data) / 1e6, len(proudy[3].snimky)))
    body = [0, 12, 25, 38, 52, 66, 80, 95, 110, 125, len(proudy[3].snimky) - 1]
    with sync_playwright() as p:
        b = p.chromium.launch()

        # ===== stranka B319 =====
        stav, dotazy, log, chybyJs, vysl, citace = beh(b, CZ, proudy, body, 'B319')
        url = [d[1] for d in dotazy if d[1].startswith('/ps1.stream')]
        over(url and 'v=319' in url[0] and stav['volby'] == 3,
             '1) stranka B319 si rekne o proud s kopii a polovicnim rozlisenim: %s' % (url[0] if url else '-'))
        spatne = []
        for i, r in vysl:
            if r is None:
                spatne.append('%d: nedoslo' % i)
                continue
            ps, ct, zd, st = r
            tisk('   snimek %3d: PSNR %.2f dB, nejhorsi ctverec %.1f, 2x2 stejne %.1f %% (paketu %d)' % (i, ps, ct, zd * 100, st[0]))
            if ps < 28.0 or ct > 15.0:
                spatne.append('%d: %.2f dB / %.1f' % (i, ps, ct))
        over(not spatne and len(vysl) == len(body),
             '2) obraz v canvasu = snimek hry ve %d kontrolnich bodech (uprostred posunu, kopie + JPEG v poradi): %s'
             % (len(vysl), 'vse spravne' if not spatne else ', '.join(spatne)))
        zd_min = min(r[2] for _, r in vysl if r)
        over(citace[0] >= 60 and citace[1] >= 300 and zd_min >= 0.9,
             '3) kopii nakresleno %d, JPEG v polovicnim rozliseni %d (z %d) - 2x2 body v canvasu stejne v %.1f %% (bez vyhlazeni)'
             % (citace[0], citace[1], citace[2], zd_min * 100))
        over(len(proudy[3].data) * 100 <= len(proudy[0].data) * 35,
             '4) dat pro stejnych %d snimku: B319 %.2f MB, drive %.2f MB (%.0f %%)'
             % (len(proudy[3].snimky), len(proudy[3].data) / 1e6, len(proudy[0].data) / 1e6, 100.0 * len(proudy[3].data) / len(proudy[0].data)))
        over(not chybyJs and citace[3] == 0 and not stav['znovu'],
             '6) zadna chyba JavaScriptu (%s), chyb v datech %d, proud se neotviral znovu' % (chybyJs[:2] if chybyJs else 'zadna', citace[3]))
        b319log = [m for _, m in log if 'PROHLIZEC PS1' in m]
        tisk('   log prohlizece: %s' % (b319log[0] if b319log else '(nic)'))
        over(b319log and 'B319: kopii (posun obrazu)' in b319log[0] and 'JPEG v polovicnim rozliseni' in b319log[0],
             '6) log prohlizece do appky (kazdych 30 s): "... B319: kopii (posun obrazu) x/s, JPEG v polovicnim rozliseni n ..."')

        # ===== stara stranka B318: proud jako drive =====
        stav2, dotazy2, log2, chybyJs2, vysl2, citace2 = beh(b, STARA, proudy, [0, 40, 80, len(proudy[0].snimky) - 1], 'B318')
        url2 = [d[1] for d in dotazy2 if d[1].startswith('/ps1.stream')]
        ok2 = all(r and r[0] >= 28.0 and r[1] <= 15.0 for _, r in vysl2) and len(vysl2) == 4
        for i, r in vysl2:
            if r:
                tisk('   B318 snimek %3d: PSNR %.2f dB, nejhorsi ctverec %.1f' % (i, r[0], r[1]))
        over(url2 and 'v=319' not in url2[0] and stav2['volby'] == 0 and ok2 and not chybyJs2,
             '5) stara stranka B318 (%s) dostane proud jako drive a obraz je spravne' % (url2[0] if url2 else '-'))
        b.close()
    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b319_prohlizec.txt'), 'w').write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
