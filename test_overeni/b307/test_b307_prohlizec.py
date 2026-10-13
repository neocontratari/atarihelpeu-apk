#!/usr/bin/env python3
# B307: KONEC-KONCU test uvodniho dema na TV / PC (/intro.stream) s demem B307.
#
#  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java),
#  - Chromium (Playwright), jedno kliknuti do obrazu (jako clovek u TV / PC - zvuk),
#  - Python server = to, co v appce dela Java (/status proud=intro, /intro.stream, /clientlog),
#  - libintro_b307.so = SKUTECNY nap_intro_gl.cpp (tv_intro.cpp, Mesa bez okna, OpenSL "karta").
#  Demo bezi v realnem case az za defile na CRT (35 s): na TV jde obraz 16:9 i zvuk;
#  zvuk v paketech = PRESNE stopa dema vcetne zvuku Reneho hry Pitt-Kitt (jizda Las Vegas,
#  33,40 - 34,25 s), konci v case obrazu; snimek z prohlizece behem Pitt-Kitt se ulozi.
#
#   python3 test_b307_prohlizec.py          -> VYSTUP_test_b307_prohlizec.txt (+ PRAC/tv_pittkitt.png)
import base64
import ctypes
import http.server
import os
import re
import socket
import statistics
import struct
import sys
import threading
import time
import urllib.parse
import zlib

import numpy as np

os.environ.setdefault('EGL_PLATFORM', 'surfaceless')
ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
PRAC = os.environ.get('PRAC', '/tmp/nap_b307')
os.makedirs(PRAC, exist_ok=True)


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


HTML = stranka_z_javy(open(os.path.join(KOREN, JAVA_CESTA), encoding='utf-8').read()).encode('utf-8')

LIB = ctypes.CDLL(os.environ.get('NAP_INTRO_LIB', os.path.join(PRAC, 'libintro_b307.so')))
LIB.intro_start.argtypes = [ctypes.c_char_p]
for f in ('intro_prehrano', 'intro_zacatek', 'intro_snimku'):
    getattr(LIB, f).restype = ctypes.c_longlong
LIB.intro_reference.argtypes = [ctypes.c_void_p, ctypes.c_int]
LIB.intro_dalsi.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
LIB.intro_log.restype = ctypes.c_char_p
LIB.intro_info.restype = ctypes.c_char_p

PZ = threading.Lock()
DOTAZY = {}
CLIENTLOG = []
NA_OBRAZOVCE = ['']
PAKETY = []                    # (cas, seq, ver, vzorku)
INTRO_ZVUK = bytearray()       # vsechen zvuk dema z paketu (int16 stereo)
INTRO_CAS = []                 # (ramcu zvuku z paketu celkem, cas dema [ms] v okamziku paketu)
LOG = []


def rozeber(p):
    """paket verze 3 (JPEG): hlavicka, preskocit obraz, zvuk; vraci (seq, ver, vzorku, pcm stereo)"""
    ver = p[2]
    seq = struct.unpack_from('<I', p, 8)[0]
    o = 28
    if ver == 3:
        dl = struct.unpack_from('<I', p, o + 9)[0]
        o += 13 + dl
    elif ver == 2:
        pz = struct.unpack_from('<H', p, o)[0]
        o += 2 + 4 * pz
        dl = struct.unpack_from('<I', p, o)[0]
        o += 4 + dl
    else:
        return seq, ver, 0, b''
    an = struct.unpack_from('<H', p, o)[0]
    kan = p[o + 2]
    o += 3
    pcm = p[o:o + an * 2 * kan]
    if kan == 1:
        pcm = np.repeat(np.frombuffer(pcm, dtype='<i2'), 2).astype('<i2').tobytes()
    return seq, ver, an, pcm


class Server(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def posli(self, kod, typ, data):
        self.send_response(kod)
        self.send_header('Content-Type', typ)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.wfile.write(data)

    def proud(self):
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n'
                         b'Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n'
                         b'Pragma: no-cache\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        if NA_OBRAZOVCE[0] != 'intro':
            return
        cid = LIB.intro_otevri()
        kap = 4 * 1024 * 1024
        b = ctypes.create_string_buffer(kap)
        zapis = 0
        posledni = time.monotonic()
        try:
            while NA_OBRAZOVCE[0] == 'intro':
                n = LIB.intro_dalsi(cid, b, kap, 250, zapis)
                if n == -2 ** 31:
                    break
                if n < 0:
                    kap = -n + 65536
                    b = ctypes.create_string_buffer(kap)
                    continue
                if n == 0:
                    if (LIB.intro_stav() & 1) == 0 and time.monotonic() - posledni > 3:
                        break
                    continue
                posledni = time.monotonic()
                data = b.raw[:n]
                seq, ver, an, pcm = rozeber(data[4:])
                with PZ:
                    PAKETY.append((time.monotonic(), seq, ver, an))
                    INTRO_ZVUK.extend(pcm)
                    if LIB.intro_zacatek() >= 0:
                        INTRO_CAS.append((len(INTRO_ZVUK) // 4, LIB.intro_cas_ms()))
                t0 = time.monotonic()
                self.wfile.write(data)
                self.wfile.flush()
                zapis = int(min(1000, (time.monotonic() - t0) * 1000))
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            LIB.intro_zavri(cid)
        self.close_connection = True

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        with PZ:
            DOTAZY[u.path] = DOTAZY.get(u.path, 0) + 1
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            proud = 'intro' if NA_OBRAZOVCE[0] == 'intro' and (LIB.intro_stav() & 1) else '0'
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=0 seq=0 h264Seq=0\n' % proud).encode())
        elif u.path == '/intro.stream':
            self.proud()
        elif u.path == '/clientlog':
            with PZ:
                CLIENTLOG.append(q.get('m', [''])[0])
            self.posli(200, 'text/plain', b'ok')
        else:
            self.posli(404, 'text/plain', b'nic')


def png_zapis(cesta, w, h, rgba):
    raw = b''.join(b'\x00' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))

    def blok(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(cesta, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + blok(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + blok(b'IDAT', zlib.compress(raw, 6)) + blok(b'IEND', b''))


def main():
    vystup = []

    def tisk(s):
        print(s)
        vystup.append(s)
        sys.stdout.flush()

    chyb = [0]
    kontrol = [0]

    def over(ok, co):
        kontrol[0] += 1
        if not ok:
            chyb[0] += 1
        tisk(('OK     ' if ok else 'CHYBA  ') + co)

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    def canvas(page):
        r = page.evaluate("""(function(){var a=window.napAp;if(!a||!a.cx||!a.W)return null;
            var d=a.cx.getImageData(0,0,a.W,a.H).data,s='';
            for(var i=0;i<d.length;i+=32768){s+=String.fromCharCode.apply(null,d.subarray(i,Math.min(d.length,i+32768)));}
            return {W:a.W,H:a.H,b:btoa(s)};})()""")
        if not r:
            return None
        return r['W'], r['H'], base64.b64decode(r['b'])

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        try:
            br = p.chromium.launch()
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome')
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        page = ctx.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        tisk('B307: prohlizec (Chromium) <- server jako Java <- SKUTECNY nap_intro_gl.cpp (demo B307)')
        tisk('stranka z MainActivity.java: %d bajtu' % len(HTML))
        page.goto(url)
        page.wait_for_timeout(1500)
        page.mouse.click(640, 360)             # clovek u TV / PC klikne (zvuk)
        page.wait_for_timeout(500)

        st = LIB.intro_start(os.path.join(KOREN, 'app/src/main/assets/').encode())
        over(st == 3, 'demo: introStart - zvuk prehrava C++ (OpenSL ES), zvuk Reneho hry ve stope (bity %d)' % st)
        t0 = time.monotonic()
        while not LIB.intro_pripraveno() and time.monotonic() - t0 < 20:
            time.sleep(0.05)
        NA_OBRAZOVCE[0] = 'intro'
        try:
            page.wait_for_function("napAp.on&&napAp.druh==='intro'&&napAp.W===854", timeout=12000)
            prep = True
        except Exception:
            prep = False
        s = page.evaluate("({W:napAp.W,H:napAp.H,ver:napAp.ver,druh:napAp.druh,px:napAp.px,py:napAp.py,ow:napAp.ow})")
        over(prep and s['W'] == 854 and s['H'] == 480 and s['ver'] == 3 and s['px'] == 16 and s['py'] == 9,
             'stranka sama prepnula na /intro.stream - 854x480 16:9 %s' % s)
        info = LIB.intro_info().decode('utf-8', 'replace')
        over('zabery Pitt-Kitt ano, zvuk hry ano' in info, 'C++ nacetl zabery i zvuk Pitt-Kitt (%s)' % info[info.find('zabery'):info.find('zabery') + 36])
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod})')
        tA = time.monotonic()
        # az za defile (cas dema 35,5 s); behem Pitt-Kitt (jizda Las Vegas) snimek z prohlizece
        snimek = None
        while time.monotonic() - tA < 60:
            c = LIB.intro_cas_ms()
            if snimek is None and 33550 <= c <= 34100:
                cv = canvas(page)
                if cv:
                    snimek = (c, cv)
            if c >= 35500:
                break
            time.sleep(0.1)
        dt = time.monotonic() - tA
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod,chyb:napAp.chyb})')
        jps = (a1['jpg'] - a0['jpg']) / dt
        zv = (a1['zv'] - a0['zv']) / dt
        tisk('demo %.1f s v prohlizeci: %.1f obrazku/s (Mesa bez GPU kresli pomalu), zvuk %.0f vzorku/s, podteceni %d, cas dema %d ms'
             % (dt, jps, zv, a1['pod'] - a0['pod'], LIB.intro_cas_ms()))
        over(jps >= 5 and a1['chyb'] == 0, 'obraz dema z C++ chodi do prohlizece (JPEG 854x480)')
        over(44100 * 0.95 < zv < 44100 * 1.05, 'zvuk 44100 vzorku/s jde proudem')
        if snimek:
            c, (W, H, d) = snimek
            png_zapis(os.path.join(PRAC, 'tv_pittkitt.png'), W, H, d)
            a = np.frombuffer(d, dtype=np.uint8).reshape(H, W, 4)[:, :, :3].astype(np.float32)
            stred = a[int(H * 0.2):int(H * 0.7), int(W * 0.3):int(W * 0.7)]
            modra = float(stred[:, :, 2].mean() - stred[:, :, 0].mean())
            tisk('snimek z prohlizece v case dema %d ms (Pitt-Kitt): jas stredu %.1f, modra - cervena %.1f -> PRAC/tv_pittkitt.png' % (c, stred.mean(), modra))
            over(stred.mean() > 25 and modra > 15, 'na TV je behem Pitt-Kitt videt obrazovka hry (modra noc Las Vegas na CRT)')
        else:
            over(False, 'snimek z prohlizece behem Pitt-Kitt se nepodaril')
        # zvuk v paketech = presne stopa dema vcetne zvuku hry
        ref = (ctypes.c_int16 * (2 * 44100 * 50))()
        nref = LIB.intro_reference(ref, 44100 * 50)
        refa = np.frombuffer(ref, dtype=np.int16)[:nref * 2].reshape(-1, 2)
        with PZ:
            pr = np.frombuffer(bytes(INTRO_ZVUK), dtype='<i2').reshape(-1, 2)
            cas = list(INTRO_CAS)
        shoda_ok = False
        info = 'nic'
        off = None
        nz = np.nonzero(np.abs(pr[:, 0].astype(np.int32)) > 50)[0]
        if len(nz) and len(pr) > 44100:
            i0 = int(nz[0])
            vzor = pr[i0:i0 + 256, 0]
            for k in np.nonzero(refa[:, 0] == vzor[0])[0]:
                if k + 256 <= len(refa) and np.array_equal(refa[k:k + 256, 0], vzor):
                    off = int(k) - i0
                    a = max(0, -off)
                    b = min(len(pr), len(refa) - off)
                    ruzne = int(np.sum(np.any(pr[a:b] != refa[a + off:b + off], axis=1))) if b > a else -1
                    od_s, do_s = (a + off) / 44100.0, (b + off) / 44100.0
                    info = 'porovnano %d ramcu (stopa %.2f - %.2f s), rozdilnych %d' % (b - a, od_s, do_s, ruzne)
                    shoda_ok = ruzne == 0 and od_s < 33.0 and do_s > 34.3
                    break
        tisk('zvuk dema v paketech proti stope (bit po bitu): %s' % info)
        over(shoda_ok, 'zvuk na TV = PRESNE stopa dema vcetne zvuku Pitt-Kitt (33,40 - 34,25 s), vzorek po vzorku bez mezer')
        rozd = [(kon + off) / 44.1 - cms for kon, cms in cas] if shoda_ok else []
        if rozd:
            tisk('zvuk paketu konci proti casu dema (obraz): median %.0f ms, min %.0f, max %.0f (%d paketu)'
                 % (statistics.median(rozd), min(rozd), max(rozd), len(rozd)))
        over(bool(rozd) and abs(statistics.median(rozd)) <= 60, 'v paketu je zvuk prave prehraneho okamziku dema (obraz a zvuk spolu)')
        LIB.intro_stop()
        over(LIB.intro_hracu() == 101 and LIB.intro_okno_refs() == 1, 'demo konec (introStop): prehravac OpenSL zniceny, okno uvolnene')
        NA_OBRAZOVCE[0] = ''
        try:
            page.wait_for_function("!napAp.chce&&document.getElementById('v').getAttribute('src')", timeout=6000)
            zpet = True
        except Exception:
            zpet = False
        over(zpet, 'demo pryc z obrazovky -> proud konci, stranka zpet postaru')
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2],))
        br.close()
    srv.shutdown()
    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b307_prohlizec.txt'), 'w') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
