#!/usr/bin/env python3
"""B313: PROHLIZEC NA TV / PC SE PO ODCHODU Z MENU PREPNE NA SPRAVNY PROUD.

U Reneho (log B312): menu bylo na TV, Rene otevrel Atari 130XE - prohlizec se jeste 25 s
kazdou vterinu pokousel otevrit /menu.stream ("B306 MENU PROUD start") a Atari slo na TV
starou pomalou cestou (Java JPEG 1280x720). Server na proud, ktery neni na obrazovce,
odpovidal prazdnou odpovedi 200 a prohlizec mel porad stary udaj proud=menu.

B313: server odpovi 409 a v hlavicce X-Nap-Proud rekne, co na obrazovce opravdu je
(MainActivity.napTvProudNeniTu); prohlizec se hned prepne.

  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java),
  - Python server = co dela Java: /status, /menu.stream, /atari.stream, /clientlog.
Scenare:
  A) /status po odchodu z menu hned rika proud=atari (jak ma) - prepnuti do 2 s,
  B) /status porad rika proud=menu (jako u Reneho) a /menu.stream odpovi 409 + X-Nap-Proud:
     atari - prohlizec se i tak prepne na /atari.stream (do 2 s), menu uz nezkousi dokola,
  C) B ale bez hlavicky (stary server) - prohlizec nesmi spadnout (zadna chyba JS).
Pouziti: python3 test_b313_tv_prepnuti.py      -> VYSTUP_test_b313_tv_prepnuti.txt
"""
import http.server
import os
import re
import sys
import threading
import time
import urllib.parse
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
vystup = []
kontrol = chyb = 0


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    r = ('OK     ' if ok else 'CHYBA  ') + co
    print(r, flush=True)
    vystup.append(r)


def stranka_z_javy(src):
    """HTML prohlizece presne tak, jak ho sklada MainActivity.napTvWebWriteHtml."""
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


HTML = stranka_z_javy(JAVA).encode('utf-8')
assert b"X-Nap-Proud" in HTML, 'stranka nema B313 prepinani'


def scenar(jmeno, status_po, hlavicka):
    T0 = time.time()
    LOG, POZ = [], []
    STAV = {'faze': 'menu'}

    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def posli(self, kod, typ, data, hl=None):
            self.send_response(kod)
            self.send_header('Content-Type', typ)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            for k, v in (hl or {}).items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            p = self.path.split('?')[0]
            POZ.append((round(time.time() - T0, 2), p))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', HTML)
            if p == '/status':
                proud = 'menu' if STAV['faze'] == 'menu' else status_po
                atari = '0' if STAV['faze'] == 'menu' else '1'
                t = ('running=true atari=%s atariCpp=%s atariProud=%s proud=%s sega=0 ps1=0 menu=%s seq=1 h264Seq=0 url=x\n'
                     % (atari, atari, atari, proud, '1' if STAV['faze'] == 'menu' else '0'))
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                m = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get('m', [''])[0]
                LOG.append((round(time.time() - T0, 2), m))
                return self.posli(200, 'text/plain', b'ok')
            if p.endswith('.stream'):
                druh = p[1:-7]
                je = 'menu' if STAV['faze'] == 'menu' else 'atari'
                if druh != je:
                    # jako Java B313: napTvProudNeniTu
                    return self.posli(409, 'text/plain', ('proud=' + je + '\n').encode(), {'X-Nap-Proud': je} if hlavicka else None)
                self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                self.wfile.flush()
                f0 = STAV['faze']
                while STAV['faze'] == f0 and time.time() - T0 < 40:
                    time.sleep(0.05)
                return
            return self.posli(200, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    port = srv.server_address[1]
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    chybyJs = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        pg = b.new_page()
        pg.on('pageerror', lambda e: chybyJs.append(str(e)))
        pg.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        pg.goto('http://127.0.0.1:%d/' % port)
        pg.wait_for_timeout(4500)
        t_prepnuti = round(time.time() - T0, 2)
        STAV['faze'] = 'atari'
        pg.wait_for_timeout(6000)
        b.close()
    srv.shutdown()
    menu_pred = [t for t, p in POZ if p == '/menu.stream' and t < t_prepnuti]
    menu_po = [t for t, p in POZ if p == '/menu.stream' and t >= t_prepnuti]
    atari = [t for t, p in POZ if p == '/atari.stream' and t >= t_prepnuti]
    zpravy = [m for t, m in LOG if 'PROUD' in m or 'proud' in m]
    return t_prepnuti, menu_pred, menu_po, atari, zpravy, chybyJs


# A) status rika hned atari
tp, mp, mpo, at, zp, js = scenar('A', 'atari', True)
over(mp and at and at[0] - tp <= 2.0 and not js,
     'A) /status po odchodu z menu rika atari: prohlizec otevre /atari.stream za %.1f s' % ((at[0] - tp) if at else -1))
# B) status visi na menu (jako u Reneho), /menu.stream -> 409 + X-Nap-Proud: atari
tp, mp, mpo, at, zp, js = scenar('B', 'menu', True)
print('   zpravy prohlizece:', [z for z in zp][-6:])
over(mp and at and at[0] - tp <= 2.0 and len(mpo) <= 3 and any('B313 proud menu na obrazovce neni, server rika: atari' in z for z in zp) and not js,
     'B) /status porad rika menu (jako u Reneho): 409 + X-Nap-Proud: atari -> /atari.stream za %.1f s, /menu.stream po prepnuti %dx (drive kazdou vterinu porad)'
     % ((at[0] - tp) if at else -1, len(mpo)))
# C) bez hlavicky: nesmi spadnout
tp, mp, mpo, at, zp, js = scenar('C', 'menu', False)
over(not js, 'C) server bez hlavicky X-Nap-Proud: prohlizec nespadne (zadna chyba JavaScriptu)%s' % ((': ' + '; '.join(js)) if js else ''))

vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_test_b313_tv_prepnuti.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
