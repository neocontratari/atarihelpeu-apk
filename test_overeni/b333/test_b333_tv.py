#!/usr/bin/env python3
"""B333: STRANKY APPKY NA TV / PC CELE Z C++ - BEZ JAVY (konec-koncu, Chromium).

Rene: "tady budu nekompromisni - vypni Javu". Stranky appky (OPTIONS, napoveda, seznam her,
prehravac MP3, YouTube ...) sly na TV pres Javu (H.264 115 Mbit/s, JMuxer z internetu, MJPEG,
JPEG, /audio.raw). Od B333 jdou stejnym proudem z C++ jako menu a hry: /stranka.stream.

  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java),
  - Python server = to, co v appce dela Java: /status (proud=stranka), /stranka.stream (bajty, ktere
    vyrobil SKUTECNY nap_tv_stranka.h v test_stranka - PRAC/proud.bin), stara cesta -> 410 jako appka.
Scenare:
  A) proud=stranka: prohlizec otevre /stranka.stream, stranka na sirku (1920 x 1034) cela pres sirku
     okna, otoceni zpet na vysku (582 x 1080): cela vyska okna, po stranach cerno - NEROZTAZENA na 16:9
     (vychozi "OBRAZ: vyplnit obrazovku" plati jen pro hry), obsah canvasu = obraz stranky (PSNR),
     popisek "proud z C++: stranka" (ne MJPEG / H264), zadny dotaz na /stream.h264, /stream.mjpg,
     /frame.jpg, /audio.raw ani na internet (drive JMuxer z CDN), zadna chyba JavaScriptu,
  B) /status chvili rika proud=0: prohlizec ceka ("cekam na proud z C++"), starou cestu nezkousi;
     pak proud=stranka -> /stranka.stream do 2 s,
  C) prohlizec bez fetch streamu (ReadableStream): uprostred VIDITELNA hlaska "TENTO PROHLIZEC NEUMI PROUD
     Z C++ - OTEVRI ADRESU V CHROME, EDGE NEBO FIREFOXU", jednou do logu appky, stara cesta se nezkousi,
     zadna chyba JavaScriptu.
Pouziti: python3 test_b333_tv.py PRAC      (PRAC = slozka, kam test_stranka zapsal proud.bin)
                                           -> VYSTUP_test_b333_tv.txt
"""
import base64
import http.server
import io
import os
import re
import sys
import threading
import time
import urllib.parse

import numpy as np
from PIL import Image
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
PRAC = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
STARA = ('/stream.h264', '/stream.mjpg', '/frame.jpg', '/audio.raw')
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
PROUD = open(os.path.join(PRAC, 'proud.bin'), 'rb').read()
ZNACKA = int(open(os.path.join(PRAC, 'proud_znacka.txt')).read().strip())


def pakety(b):
    o, v = 0, []
    while o + 4 <= len(b):
        n = int.from_bytes(b[o:o + 4], 'little')
        v.append((o, b[o:o + 4 + n]))
        o += 4 + n
    assert o == len(b), 'proud.bin neni cely'
    return v


PAKETY = pakety(PROUD)
PRED = [p for o, p in PAKETY if o < ZNACKA]
PO = [p for o, p in PAKETY if o >= ZNACKA]
assert PRED and PO, 'znacka mimo proud'


def obraz(jmeno, W, H):
    a = np.frombuffer(open(os.path.join(PRAC, jmeno), 'rb').read(), dtype=np.uint32, count=W * H).reshape(H, W)
    return np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255], axis=2).astype(np.float64)


def psnr(a, b):
    mse = ((a - b) ** 2).mean()
    return 99.0 if mse < 1e-9 else 10 * np.log10(255.0 ** 2 / mse)


STAV_JS = """(function(){var a=window.napAp||{},c=a.cv,r=c?c.getBoundingClientRect():null,s=document.getElementById('s');
 return {druh:a.druh||'',ver:a.ver||0,W:a.W||0,H:a.H||0,px:a.px||0,py:a.py||0,pak:a.pak||0,chyb:a.chyb||0,
   cv:c?getComputedStyle(c).display:'-',x:r?r.left:-1,y:r?r.top:-1,w:r?r.width:-1,h:r?r.height:-1,
   iw:innerWidth,ih:innerHeight,popis:s?s.textContent:''};})()"""


def scenar(jmeno, status_fn, bez_streamu=False, pri_behu=None):
    T0 = time.time()
    POZ, LOG, EXT = [], [], []
    STAV = {'dal': False, 'konec': False}

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
            try:
                if p in ('/', '/index.html'):
                    return self.posli(200, 'text/html; charset=utf-8', HTML)
                if p == '/status':
                    t = ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=0 ps1=0 menu=0 seq=1 h264Seq=0 url=x\n'
                         % status_fn(time.time() - T0))
                    return self.posli(200, 'text/plain', t.encode())
                if p == '/clientlog':
                    m = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get('m', [''])[0]
                    LOG.append((round(time.time() - T0, 2), m))
                    return self.posli(200, 'text/plain', b'ok')
                if p in STARA:
                    # jako appka B333: 410 Gone
                    return self.posli(410, 'text/plain; charset=utf-8', b'B333: stara cesta pres Javu je vypnuta\n')
                if p == '/stranka.stream':
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nCache-Control: no-store\r\n\r\n')
                    self.wfile.flush()
                    for pk in PRED:
                        self.wfile.write(pk)
                        self.wfile.flush()
                        time.sleep(0.02)
                    while not STAV['dal'] and not STAV['konec']:
                        time.sleep(0.05)
                    for pk in PO:
                        if STAV['konec']:
                            break
                        self.wfile.write(pk)
                        self.wfile.flush()
                        time.sleep(0.02)
                    while not STAV['konec']:
                        time.sleep(0.05)
                    return
                if p.endswith('.stream'):
                    je = status_fn(time.time() - T0)
                    return self.posli(409, 'text/plain', ('proud=' + je + '\n').encode(), {'X-Nap-Proud': je})
                return self.posli(200, 'text/plain', b'')
            except (BrokenPipeError, ConnectionResetError):
                return

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    port = srv.server_address[1]
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    chybyJs = []
    vysl = {}
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        pg = b.new_page(viewport={'width': 1280, 'height': 720})
        pg.on('pageerror', lambda e: chybyJs.append(str(e)))

        def venku(r):
            EXT.append(r.request.url)
            r.abort()
        pg.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), venku)
        if bez_streamu:
            pg.add_init_script('try{delete window.ReadableStream;}catch(e){}')
        pg.goto('http://127.0.0.1:%d/' % port)
        if pri_behu:
            vysl = pri_behu(pg, STAV, T0, POZ)
        else:
            pg.wait_for_timeout(5000)
        STAV['konec'] = True
        b.close()
    srv.shutdown()
    return POZ, LOG, EXT, chybyJs, vysl


def cekej(pg, podminka, ms):
    t = time.time() + ms / 1000.0
    s = pg.evaluate(STAV_JS)
    while time.time() < t:
        s = pg.evaluate(STAV_JS)
        if podminka(s):
            return s
        pg.wait_for_timeout(100)
    return s


def canvas(pg):
    url = pg.evaluate("window.napAp.cv.toDataURL('image/png')")
    im = Image.open(io.BytesIO(base64.b64decode(url.split(',', 1)[1]))).convert('RGB')
    return np.asarray(im).astype(np.float64)


tisk('B333 - stranky appky na TV / PC z C++ (SKUTECNA stranka prohlizece z MainActivity, proud ze SKUTECNEHO nap_tv_stranka.h: %d paketu, %d kB)'
     % (len(PAKETY), len(PROUD) // 1024))


# ---------- A) proud=stranka ----------
def beh_a(pg, STAV, T0, POZ):
    r = {}
    s = cekej(pg, lambda s: s['W'] == 1920 and s['H'] == 1034 and s['pak'] >= len(PRED) - 2, 9000)
    pg.wait_for_timeout(600)
    s = pg.evaluate(STAV_JS)
    r['sirka'] = s
    r['sirka_psnr'] = psnr(canvas(pg), obraz('snimek_D.raw', 1920, 1034)) if s['W'] == 1920 else 0
    STAV['dal'] = True
    s = cekej(pg, lambda s: s['W'] == 582 and s['H'] == 1080 and s['pak'] >= len(PAKETY) - 40, 9000)
    pg.wait_for_timeout(800)
    s = pg.evaluate(STAV_JS)
    r['vyska'] = s
    r['vyska_psnr'] = psnr(canvas(pg), obraz('snimek_A.raw', 582, 1080)) if s['W'] == 582 else 0
    pg.keyboard.press('Shift')            # zvuk se odemkne klavesou (startAudio) - popisek nesmi rict MJPEG
    pg.wait_for_timeout(1500)
    r['popis_po_klavese'] = pg.evaluate(STAV_JS)['popis']
    pg.wait_for_timeout(2500)
    return r


POZ, LOG, EXT, JS, R = scenar('A', lambda t: 'stranka', pri_behu=beh_a)
proud = [t for t, p in POZ if p == '/stranka.stream']
stare = [p for t, p in POZ if p in STARA]
tisk('   dotazy prohlizece: ' + ', '.join(sorted(set(p for t, p in POZ))))
over(bool(proud) and proud[0] <= 3.0, 'A) /status rika proud=stranka: prohlizec otevrel /stranka.stream za %.1f s' % (proud[0] if proud else -1))
sa = R.get('sirka', {})
# na sirku: cela sirka okna, vyska podle pomeru 255:137
ocek_h = round(1280 / (255 / 137))
over(sa.get('W') == 1920 and sa.get('H') == 1034 and sa.get('px') == 255 and sa.get('py') == 137 and abs(sa.get('w', 0) - 1280) <= 1
     and abs(sa.get('h', 0) - ocek_h) <= 2 and abs(sa.get('y', 0) - (720 - ocek_h) / 2) <= 2 and R.get('sirka_psnr', 0) > 35,
     'A) stranka na sirku 1920 x 1034 (pomer %s:%s): canvas %dx%d na %d,%d v okne 1280x720, obsah PSNR %.1f dB'
     % (sa.get('px'), sa.get('py'), sa.get('w', 0), sa.get('h', 0), sa.get('x', 0), sa.get('y', 0), R.get('sirka_psnr', 0)))
sv = R.get('vyska', {})
ocek_w = round(720 * 97 / 180)
over(sv.get('W') == 582 and sv.get('H') == 1080 and sv.get('px') == 97 and sv.get('py') == 180 and abs(sv.get('h', 0) - 720) <= 1
     and abs(sv.get('w', 0) - ocek_w) <= 2 and abs(sv.get('x', 0) - (1280 - ocek_w) / 2) <= 2 and R.get('vyska_psnr', 0) > 40,
     'A) otoceni zpet na vysku 582 x 1080: canvas %dx%d na %d,%d (cela vyska, po stranach cerno - NE roztazene na 16:9 i pri '
     '"vyplnit obrazovku"), obsah PSNR %.1f dB' % (sv.get('w', 0), sv.get('h', 0), sv.get('x', 0), sv.get('y', 0), R.get('vyska_psnr', 0)))
popis = sv.get('popis', '')
pk = R.get('popis_po_klavese', '')
over('proud z C++: stranka' in popis and 'proud z C++: stranka' in pk and not re.search(r'MJPEG|H264|JPEG', popis + pk),
     'A) popisek "%s" i po klavese (zvuk) - zadne MJPEG / H264 / JPEG' % re.sub(r' \d+:\d+:\d+.*', '', pk or popis))
over(not stare and not EXT, 'A) zadny dotaz na starou cestu pres Javu (%s) ani na internet (drive JMuxer z CDN)%s'
     % (', '.join(STARA), (': ' + ', '.join(stare + EXT)) if (stare or EXT) else ''))
over(not JS and sv.get('chyb', 1) == 0, 'A) zadna chyba JavaScriptu ani v paketech%s' % ((': ' + '; '.join(JS)) if JS else ''))
zpr = [m for t, m in LOG if 'STRANKA' in m]
tisk('   log prohlizece: ' + ' | '.join(zpr[:3]))


# ---------- B) chvili proud=0 ----------
def beh_b(pg, STAV, T0, POZ):
    STAV['dal'] = True
    pg.wait_for_timeout(5500)
    r = {'cekani': pg.evaluate(STAV_JS)}
    pg.wait_for_timeout(4500)
    r['po'] = pg.evaluate(STAV_JS)
    return r


POZ, LOG, EXT, JS, R = scenar('B', lambda t: '0' if t < 6.0 else 'stranka', pri_behu=beh_b)
proud = [t for t, p in POZ if p == '/stranka.stream']
stare = [p for t, p in POZ if p in STARA]
ck = R.get('cekani', {})
over('cekam na proud z C++' in ck.get('popis', '') and not [t for t in proud if t < 6.0] and not stare,
     'B) /status rika proud=0: prohlizec ceka ("cekam na proud z C++"), starou cestu nezkousi')
over(bool(proud) and proud[0] - 6.0 <= 2.0 and R.get('po', {}).get('W') == 582 and not JS and not EXT,
     'B) pak proud=stranka: /stranka.stream za %.1f s, obraz %dx%d, chyb JS %d' % ((proud[0] - 6.0) if proud else -1, R.get('po', {}).get('W', 0),
                                                                                R.get('po', {}).get('H', 0), len(JS)))


# ---------- C) prohlizec bez fetch streamu ----------
def beh_c(pg, STAV, T0, POZ):
    pg.wait_for_timeout(4000)
    h = pg.evaluate("""(function(){var e=document.getElementById('napNeumi');if(!e)return null;var r=e.getBoundingClientRect(),c=getComputedStyle(e);
      return {t:e.textContent,v:c.display!=='none'&&c.visibility!=='hidden'&&r.width>100&&r.height>20,x:r.left,y:r.top,w:r.width,h:r.height};})()""")
    return {'s': pg.evaluate(STAV_JS), 'hlaska': h}


POZ, LOG, EXT, JS, R = scenar('C', lambda t: 'stranka', bez_streamu=True, pri_behu=beh_c)
stare = [p for t, p in POZ if p in STARA]
neumi = [m for t, m in LOG if 'B333 PROHLIZEC: neumi proud z C++' in m]
hl = R.get('hlaska') or {}
over('tento prohlizec neumi proud z C++' in R.get('s', {}).get('popis', '') and len(neumi) == 1 and not stare and not JS and not EXT
     and hl.get('v') and hl.get('t') == 'TENTO PROHLIZEC NEUMI PROUD Z C++ - OTEVRI ADRESU V CHROME, EDGE NEBO FIREFOXU',
     'C) prohlizec bez fetch streamu: uprostred obrazovky VIDITELNA hlaska "%s" (%dx%d), v logu appky %dx, stara cesta %dx, chyb JS %d'
     % (hl.get('t'), hl.get('w', 0), hl.get('h', 0), len(neumi), len(stare), len(JS)))

v = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
tisk(v)
open(os.path.join(ZDE, 'VYSTUP_test_b333_tv.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
