#!/usr/bin/env python3
# B305: KONEC-KONCU test prenosu Sega a PS1 na TV / PC cele v C++.
#
#  - stranka prohlizece = PRESNE ta z appky: vytahne se z MainActivity.java
#    (napTvWebWriteHtml), zadna kopie,
#  - Chromium (Playwright) = prohlizec na PC u TV,
#  - tenhle Python server = to, co v appce dela Java: /status (proud=sega|ps1|0),
#    /sega.stream a /ps1.stream (proudDalsiNative ve smycce, 4 B delka + paket,
#    doba zapisu jde zpet do C++ - napTvWebJadroProud), /clientlog,
#  - libsega_b305.so = SKUTECNY nap_sega_native_proof.cpp + jadro clownmdemu
#    (snimky jdou pres skutecne zpetne volani VDP, zvuk pres skutecne JNI
#    pullAudioStereo jako z AudioTrack v appce),
#  - libps1_b305.so = SKUTECNY nap_ps1_native.cpp (start pres JNI
#    ps1BootDoMonitoru, skutecne vlakno emulace, nap_video, OpenSL callback);
#    misto PCSX snimky 3D sceny (hra na 30 snimku/s).
#
#   viz JAK_SPUSTIT.txt; python3 test_b305_prohlizec.py ADRESAR_PNG_320x240
import base64
import ctypes
import http.server
import math
import os
import re
import socket
import statistics
import sys
import threading
import time
import urllib.parse

os.environ.setdefault('EGL_PLATFORM', 'surfaceless')      # Mesa bez okna (PS1 si dela vlastni EGL kontext)
ZDE = os.path.dirname(os.path.abspath(__file__))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'


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


HTML = stranka_z_javy(open(os.path.join(ZDE, '../..', JAVA_CESTA), encoding='utf-8').read()).encode('utf-8')

LIB = {}
for jm in ('sega', 'ps1'):
    lib = ctypes.CDLL(os.path.join(ZDE, 'lib%s_b305.so' % jm))
    getattr(lib, jm + '_dalsi').argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    getattr(lib, jm + '_log').restype = ctypes.c_char_p
    getattr(lib, jm + '_ocekavany').argtypes = [ctypes.c_uint, ctypes.c_void_p]
    LIB[jm] = lib
LIB['ps1'].ps1_start.restype = ctypes.c_char_p
LIB['ps1'].ps1_start.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

PZ = threading.Lock()
DOTAZY = {}
CLIENTLOG = []
NA_OBRAZOVCE = ['']            # 'sega' / 'ps1' / '' - co je na obrazovce telefonu (napTvJadroNaObrazovce)
ZAVRIT_PROUD = threading.Event()
PROUD_BAJTU = {'sega': 0, 'ps1': 0}
ZAPIS_MS = {'sega': [], 'ps1': []}
LOG = []


def zaloguj():
    for jm in ('sega', 'ps1'):
        s = getattr(LIB[jm], jm + '_log')().decode('utf-8', 'replace')
        if s:
            with PZ:
                LOG.append(s)


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

    def proud(self, jm):
        # Java napTvWebJadroProud: hlavicka, pak proudDalsiNative(id, b, 250, zapisMs) dokola
        lib = LIB[jm]
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n'
                         b'Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n'
                         b'Pragma: no-cache\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        if NA_OBRAZOVCE[0] != jm:
            return
        try:
            self.connection.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 64 * 1024)
        except OSError:
            pass
        ZAVRIT_PROUD.clear()
        cid = getattr(lib, jm + '_otevri')()
        kap = 1024 * 1024
        b = ctypes.create_string_buffer(kap)
        zapis = 0
        posledni = time.monotonic()
        try:
            while NA_OBRAZOVCE[0] == jm and not ZAVRIT_PROUD.is_set():
                n = getattr(lib, jm + '_dalsi')(cid, b, kap, 250, zapis)
                if n == -2 ** 31:
                    break
                if n < 0:
                    kap = -n + 65536
                    b = ctypes.create_string_buffer(kap)
                    continue
                if n == 0:
                    if (getattr(lib, jm + '_stav')() & 1) == 0 and time.monotonic() - posledni > 3:
                        break
                    continue
                posledni = time.monotonic()
                t0 = time.monotonic()
                self.wfile.write(b.raw[:n])
                self.wfile.flush()
                zapis = int(min(1000, (time.monotonic() - t0) * 1000))
                PROUD_BAJTU[jm] += n
                ZAPIS_MS[jm].append(zapis)
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            getattr(lib, jm + '_zavri')(cid)
        self.close_connection = True

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        g = lambda k, d='': q.get(k, [d])[0]
        with PZ:
            DOTAZY[u.path] = DOTAZY.get(u.path, 0) + 1
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            d = NA_OBRAZOVCE[0]
            proud = d if d and (getattr(LIB[d], d + '_stav')() & 1) else '0'
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=%d seq=0 h264Seq=0\n'
                                           % (proud, 1 if d == 'sega' else 0)).encode())
        elif u.path in ('/sega.stream', '/ps1.stream'):
            self.proud(u.path[1:].split('.')[0])
        elif u.path == '/zmen':          # test zpozdeni: v jadru se objevi / zmizi bily ctverec
            getattr(LIB[g('d')], g('d') + '_ctverec')(int(g('z', '0')))
            self.posli(200, 'text/plain', b'ok')
        elif u.path == '/clientlog':
            with PZ:
                CLIENTLOG.append(g('m'))
            self.posli(200, 'text/plain', b'ok')
        else:
            self.posli(404, 'text/plain', b'nic')     # MJPEG / JPEG / H.264 / zvuk postaru tu nejsou


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

    png = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ZDE, 'ps1_320')
    prac = os.path.join(ZDE, 'prac_b305')
    os.makedirs(prac, exist_ok=True)
    tisk('B305: prohlizec (Chromium) <- server jako Java <- SKUTECNY nap_sega_native_proof.cpp / nap_ps1_native.cpp')
    tisk('stranka z MainActivity.java: %d bajtu, skriptu %d' % (len(HTML), HTML.count(b'<script>')))
    over(LIB['sega'].sega_start() == 1, 'Sega: jadro bezi (snimky pres skutecne zpetne volani VDP, zvuk pres JNI pullAudioStereo)')
    NA_OBRAZOVCE[0] = 'sega'

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

    def sega_presne(page, pokusu=8):
        """canvas (RGBA) proti snimku, ktery jadro Segy opravdu dalo (ARGB); vraci pocet ruznych bodu"""
        r = -1
        for _ in range(pokusu):
            c = canvas(page)
            if c:
                W, H, d = c
                barva = (d[0] << 16) | (d[1] << 8) | d[2]
                ocek = (ctypes.c_uint32 * (W * H))()
                wh = LIB['sega'].sega_ocekavany(barva, ocek)
                if wh == ((W << 16) | H):
                    j = bytes(memoryview(ocek).cast('B'))
                    r = sum(1 for i in range(0, W * H * 4, 4) if d[i] != j[i + 2] or d[i + 1] != j[i + 1] or d[i + 2] != j[i] or d[i + 3] != 255)
                    if r == 0:
                        return 0
            time.sleep(0.25)
        return r

    def ps1_psnr(page, pokusu=8):
        for _ in range(pokusu):
            c = canvas(page)
            if c:
                W, H, d = c
                n = 0
                for bit in range(16):
                    x, y = (bit % 4) * 4 + 1, (bit // 4) * 4 + 1
                    if d[(y * W + x) * 4 + 1] > 128:
                        n |= 1 << bit
                ocek = (ctypes.c_uint32 * (W * H))()
                if LIB['ps1'].ps1_ocekavany(n, ocek) == ((W << 16) | H):
                    j = bytes(memoryview(ocek).cast('B'))
                    se = 0
                    for i in range(0, W * H * 4, 4):
                        se += (d[i] - j[i + 2]) ** 2 + (d[i + 1] - j[i + 1]) ** 2 + (d[i + 2] - j[i]) ** 2
                    return 10 * math.log10(255 * 255 / max(1e-9, se / (W * H * 3)))
            time.sleep(0.25)
        return -1

    def rozmer(page):
        return page.evaluate("""(function(){var a=napAp,r=a.cv.getBoundingClientRect();return {x:r.left,y:r.top,w:r.width,h:r.height,
            ox:a.ox,oy:a.oy,ow:a.ow,oh:a.oh,W:a.W,H:a.H,fill:/vyplnit/.test(document.getElementById('sF').textContent)?1:0};})()""")

    def zpozdeni(page, druh, x, y):
        return page.evaluate("""async function(a){var lat=[];
            for(var k=0;k<6;k++){var bila=(k%2===0);var t0=performance.now();
              await fetch('/zmen?d='+a.d+'&z='+(bila?1:0));
              while(performance.now()-t0<2000){var p=napAp.cx.getImageData(a.x,a.y,1,1).data;
                if((p[0]>240&&p[1]>240&&p[2]>240)===bila)break;await new Promise(function(r){setTimeout(r,2);});}
              lat.push(performance.now()-t0);await new Promise(function(r){setTimeout(r,300);});}
            return lat;}""", {'d': druh, 'x': x, 'y': y})

    def stare_cesty():
        with PZ:
            return DOTAZY.get('/stream.mjpg', 0) + DOTAZY.get('/frame.jpg', 0) + DOTAZY.get('/stream.h264', 0) + DOTAZY.get('/audio.raw', 0)

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        args = ['--autoplay-policy=no-user-gesture-required']
        try:
            br = p.chromium.launch(args=args)
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome', args=args)
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        page = ctx.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())   # zadny internet (JMuxer z CDN)
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        page.goto(url)

        # ================= SEGA =================
        tisk('\n--- SEGA (/sega.stream, paleta + LZ bez ztraty) ---')
        try:
            page.wait_for_function("window.napAp&&napAp.on&&napAp.druh==='sega'&&napAp.pak>10", timeout=10000)
            zap = True
        except Exception:
            zap = False
        stav = page.evaluate("""(function(){var a=napAp,v=document.getElementById('v'),h=document.getElementById('h264v');
            return {W:a.W,H:a.H,ver:a.ver,cv:a.cv?getComputedStyle(a.cv).display:'',img:getComputedStyle(v).display,imgSrc:v.getAttribute('src')||'',
            h264:getComputedStyle(h).display};})()""")
        tisk('po nacteni: %s' % stav)
        over(zap and stav['W'] == 320 and stav['H'] == 224 and stav['ver'] == 2 and stav['cv'] != 'none',
             'stranka z appky (/status proud=sega) sama prepnula na proud z C++: canvas 320x224, paket verze 2')
        over(stav['img'] == 'none' and stav['imgSrc'] == '' and stav['h264'] == 'none', 'MJPEG obrazek i H.264 video vypnute (zadny JPEG ani H.264 z Javy)')
        mj0 = stare_cesty()
        time.sleep(1.0)
        r = sega_presne(page)
        tisk('canvas proti snimku z jadra Segy: %d ruznych bodu z %d' % (r, 320 * 224))
        over(r == 0, 'Sega: obraz v prohlizeci je PRESNE snimek jadra (vsech 71680 bodu, bez ztraty)')
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah})')
        b0 = PROUD_BAJTU['sega']
        time.sleep(4.0)
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah,cil:napAp.cil,chyb:napAp.chyb})')
        b1 = PROUD_BAJTU['sega']
        fps = (a1['pak'] - a0['pak']) / 4.0
        zv = (a1['zv'] - a0['zv']) / 4.0
        tisk('za 4 s: %.1f snimku/s, zvuk %.0f vzorku/s, podteceni %d, zahozeno %d, polstar %.0f ms, chyb %d, %.2f Mbit/s'
             % (fps, zv, a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], a1['cil'] * 1000, a1['chyb'], (b1 - b0) * 8 / 4.0 / 1e6))
        over(57 <= fps <= 62 and a1['chyb'] == 0, 'Sega: prohlizec dostava kazdy snimek (60/s), zadna chyba paketu')
        over(48000 * 0.9 < zv < 48000 * 1.1, 'Sega: zvuk jde proudem a hraje ve WebAudio (48000 vzorku/s)')
        # rozmer: vyplnit obrazovku (obsah bez cerneho pruhu vpravo na 16:9) / cely 4:3
        rz = rozmer(page)
        tisk('vyplnit obrazovku: %s' % rz)
        sx, sy = 1280 / max(1, rz['ow']), 720 / max(1, rz['oh'])
        over(rz['fill'] == 1 and rz['ow'] == 312 and abs(rz['x'] + rz['ox'] * sx) < 1.5 and abs(rz['w'] - 320 * sx) < 1.5 and abs(rz['h'] - 224 * sy) < 1.5,
             'Sega "vyplnit obrazovku": obsah bez cerneho pruhu (levy sloupec, 312 bodu) presne na celou TV 16:9 jako doted')
        page.evaluate("document.getElementById('sF').click()")
        time.sleep(0.2)
        rz2 = rozmer(page)
        tisk('cely, bez orezu: %s' % rz2)
        over(rz2['fill'] == 0 and abs(rz2['w'] - 960) < 1.5 and abs(rz2['h'] - 720) < 1.5 and abs(rz2['x'] - 160) < 1.5,
             'Sega "cely, bez orezu": cely snimek 4:3 (960x720 uprostred, po stranach cerno)')
        page.evaluate("document.getElementById('sF').click()")
        # zpozdeni: zmena v jadru -> obraz v prohlizeci (ctverec 100..160 x 60..100, po posunu leveho sloupce x-8)
        lat = zpozdeni(page, 'sega', 120, 80)
        tisk('zpozdeni zmena v jadru Segy -> canvas (bez Wi-Fi): %s ms' % ', '.join('%.0f' % x for x in lat))
        med = statistics.median(lat)
        over(len(lat) == 6 and med <= 60 and max(lat) <= 120, 'Sega: zmena v jadru je v obraze prohlizece do 2-3 snimku (median %.0f ms, snimek = 16,7 ms)' % med)
        r = sega_presne(page)
        over(r == 0, 'Sega: po zmenach obraz zase presne (%d ruznych bodu)' % r)

        # ================= PS1 =================
        tisk('\n--- PS1 (/ps1.stream, JPEG jen zmenenych casti v puvodnim rozliseni) ---')
        odp = LIB['ps1'].ps1_start(png.encode(), prac.encode()).decode()
        tisk('ps1BootDoMonitoru: %s, snimek %dx%d' % (odp, LIB['ps1'].ps1_rozmer() >> 16, LIB['ps1'].ps1_rozmer() & 0xFFFF))
        over(odp == 'PS1_BIOS_OK', 'PS1: skutecny start pres JNI (vlakno emulace, OpenSL zvuk)')
        time.sleep(0.5)
        NA_OBRAZOVCE[0] = 'ps1'          # uzivatel prepnul ze Segy na PS1
        try:
            page.wait_for_function("napAp.on&&napAp.druh==='ps1'&&napAp.jpg>10", timeout=8000)
            prep = True
        except Exception:
            prep = False
        st = page.evaluate("({W:napAp.W,H:napAp.H,ver:napAp.ver,druh:napAp.druh})")
        tisk('po prepnuti: %s' % st)
        over(prep and st['W'] == 320 and st['H'] == 240 and st['ver'] == 3, 'Sega -> PS1: stranka sama prepnula proud (bez obnoveni stranky), canvas 320x240, verze 3')
        time.sleep(1.0)
        ps = ps1_psnr(page)
        tisk('canvas proti snimku z jadra PS1: PSNR %.1f dB' % ps)
        over(ps >= 30, 'PS1: obraz v prohlizeci odpovida snimku jadra (PSNR %.1f dB >= 30)' % ps)
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,jzah:napAp.jzah})')
        b0 = PROUD_BAJTU['ps1']
        time.sleep(4.0)
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,jzah:napAp.jzah,chyb:napAp.chyb,cil:napAp.cil})')
        b1 = PROUD_BAJTU['ps1']
        fps = (a1['pak'] - a0['pak']) / 4.0
        jps = (a1['jpg'] - a0['jpg']) / 4.0
        zv = (a1['zv'] - a0['zv']) / 4.0
        tisk('za 4 s: %.1f paketu/s, %.1f JPEG/s nakresleno (prekrytych zahozeno %d), zvuk %.0f vzorku/s, polstar %.0f ms, chyb %d, %.2f Mbit/s'
             % (fps, jps, a1['jzah'] - a0['jzah'], zv, a1['cil'] * 1000, a1['chyb'], (b1 - b0) * 8 / 4.0 / 1e6))
        over(fps >= 40 and 27 <= jps <= 32 and a1['chyb'] == 0,
             'PS1: hra na 30 snimku/s = vsech 30 obrazku/s v prohlizeci, mezi nimi pakety jen se zvukem (duplikat), zadna chyba')
        over(44100 * 0.9 < zv < 44100 * 1.1, 'PS1: zvuk z OpenSL jde proudem a hraje ve WebAudio (44100 vzorku/s)')
        lat = zpozdeni(page, 'ps1', 130, 90)
        tisk('zpozdeni zmena v jadru PS1 -> canvas (bez Wi-Fi): %s ms' % ', '.join('%.0f' % x for x in lat))
        med = statistics.median(lat)
        over(len(lat) == 6 and med <= 80 and max(lat) <= 150, 'PS1: zmena v jadru je v obraze prohlizece do 2-3 snimku hry (median %.0f ms)' % med)
        # obraz stoji -> po chvili zjemneni (kvalita 94)
        ps_a = ps1_psnr(page)
        LIB['ps1'].ps1_stuj(1)
        time.sleep(1.2)
        ps_b = ps1_psnr(page)
        tisk('obraz v pohybu PSNR %.1f dB -> stoji 1,2 s: PSNR %.1f dB' % (ps_a, ps_b))
        over(ps_b > ps_a + 3.0 and ps_b >= 40, 'PS1: kdyz obraz stoji (menu, text), prijde jeste jednou ve vysoke kvalite (PSNR %.1f -> %.1f dB)' % (ps_a, ps_b))
        LIB['ps1'].ps1_stuj(0)
        # spadle spojeni
        with PZ:
            pr0 = DOTAZY.get('/ps1.stream', 0)
        ZAVRIT_PROUD.set()
        time.sleep(2.5)
        with PZ:
            pr1 = DOTAZY.get('/ps1.stream', 0)
        ps = ps1_psnr(page)
        on = page.evaluate('napAp.on&&napAp.druh')
        tisk('spojeni zavreno serverem: pozadavku na /ps1.stream %d -> %d, PSNR %.1f dB, proud %s' % (pr0, pr1, ps, on))
        over(pr1 == pr0 + 1 and ps >= 30 and on == 'ps1', 'PS1: spadle spojeni - prohlizec se pripojil znovu sam, obraz zase spravne')
        # statistika z prohlizece do logu appky (kazdych 30 s)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 33:
            with PZ:
                if any(m.startswith('B305 PROHLIZEC PS1:') for m in CLIENTLOG):
                    break
            time.sleep(0.5)
        with PZ:
            cl = list(CLIENTLOG)
        tisk('z prohlizece do logu appky: %s' % [m for m in cl if m.startswith('B305')])
        over(any(m == 'B305 SEGA PROUD start' for m in cl) and any(m == 'B305 PS1 PROUD start' for m in cl)
             and any(m.startswith('B305 PROHLIZEC PS1:') and 'JPEG' in m and 'hraje' in m for m in cl),
             'prohlizec hlasi do logu appky start proudu Segy i PS1 a kazdych 30 s snimky/s, JPEG/s a zvuk')
        mj1 = stare_cesty()
        over(mj1 == mj0, 'behem proudu (i pri prepnuti Sega -> PS1 a znovupripojeni) zadne dotazy na JPEG / MJPEG / H.264 / stary zvuk (%d -> %d)' % (mj0, mj1))

        # PS1 pryc z obrazovky -> stranka zpet postaru
        NA_OBRAZOVCE[0] = ''
        try:
            page.wait_for_function("!napAp.chce&&document.getElementById('v').getAttribute('src')", timeout=6000)
            zpet = True
        except Exception:
            zpet = False
        st2 = page.evaluate("""(function(){var a=napAp,v=document.getElementById('v');return {on:a.on,cv:a.cv?a.cv.style.display:'',
            img:v.style.display,src:(v.getAttribute('src')||'').split('?')[0]};})()""")
        tisk('PS1 pryc z obrazovky: %s' % st2)
        over(zpet and st2['cv'] == 'none' and st2['img'] != 'none' and st2['src'] in ('/stream.mjpg', '/frame.jpg'),
             'PS1 pryc z obrazovky -> proud konci, stranka zpet na MJPEG / JPEG (postaru)')
        # a zpet Sega
        NA_OBRAZOVCE[0] = 'sega'
        try:
            page.wait_for_function("napAp.on&&napAp.druh==='sega'&&napAp.ver===2", timeout=6000)
            znovu = True
        except Exception:
            znovu = False
        time.sleep(0.8)
        r = sega_presne(page)
        over(znovu and r == 0, 'Sega zpet na obrazovce -> proud z C++ zase bezi, obraz presne (%d ruznych bodu)' % r)
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2],))
        br.close()
    srv.shutdown()
    time.sleep(0.5)
    zaloguj()
    with PZ:
        cely = ''.join(LOG)
    radky = [l for l in cely.split('\n') if 'B305' in l]
    tisk('log appky (B305):\n  ' + '\n  '.join(radky[:30]))
    over(any('TV PROUD SEGA: prohlizec' in l and 'pripojen' in l for l in radky) and any('TV PROUD PS1: prohlizec' in l and 'odpojen po' in l and 'JPEG kvalita' in l for l in radky),
         'log appky: prohlizec pripojen / odpojen (pakety, MB, kvalita JPEG, doba kodovani)')
    zap = ZAPIS_MS['ps1']
    if zap:
        tisk('doba zapisu paketu PS1 do site (localhost): median %d ms, max %d ms' % (statistics.median(zap), max(zap)))
    LIB['ps1'].ps1_stop()
    LIB['sega'].sega_stop()
    tisk('\nVYSLEDEK: %d kontrol, %d chyb' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b305_prohlizec.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
