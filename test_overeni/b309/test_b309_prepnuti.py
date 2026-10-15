#!/usr/bin/env python3
"""B309: TV / PC - PREPNUTI HRY CI EMULATORU BEZ F5 (konec-koncu, Chromium).

Rene: "pokazdy pri prehozeni hry ci emulatoru musim refresovat stranku F5 - aby se chytil
novej obraz". Z jeho logu (Tv_prenos_-_a_log.txt, B308): kdyz na TV bezel H.264 (menu appky,
nebo mezera pri prepnuti hry) a pak nabehl proud z C++ (PS1 / Sega / demo), stranka H.264
zastavila, ale zrusene cteni H.264 dobehlo AZ POTOM ("stream ended, resetting") a ukazalo zpet
obrazek #v - cerny, pres celou obrazovku, v DOM nad canvasem s novym obrazem.

V testech B305-B308 se to neukazalo: prohlizec v testu nemel internet, takze se JMuxer
(prehravac H.264 z cdn.jsdelivr.net) nenacetl a H.264 cesta nikdy nebezela. Tady JMuxer
nahradi maly falesny (stejne rozhrani), server posila /stream.h264 jako appka a scenar je
presne podle Reneho logu:

  1) menu appky -> na TV H.264
  2) start PS1 (BIOS) -> proud z C++
  3) prepnuti hry (jadro se ~1,5 s restartuje, proud chvili neni)
  4) PS1 -> menu (5 s) -> Sega
  5) Sega -> PS1 rovnou
  6) F5 behem hry
  7) konec hry -> zpet H.264

  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java; --ref COMMIT = starsi),
  - Python server = to, co v appce dela Java (/status proud=..., /ps1.stream, /sega.stream,
    /stream.h264, /stream.mjpg, /clientlog),
  - libps1 = SKUTECNY nap_ps1_native.cpp (../b306/tv_ps1_6.cpp), libsega = SKUTECNY
    nap_sega_native_proof.cpp (../b306/tv_sega6.cpp) - proud z C++ jako v telefonu.

Pouziti: python3 test_b309_prepnuti.py PNG_320 ADRESAR_KNIHOVEN PRACOVNI [--ref COMMIT]
   (knihovny libps1_b309.so a libsega_b309.so, viz JAK_SPUSTIT.txt)
"""
import ctypes
import http.server
import io
import os
import re
import socket
import subprocess
import sys
import threading
import time
import urllib.parse

from PIL import Image

os.environ.setdefault('EGL_PLATFORM', 'surfaceless')
ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
ARG = [a for a in sys.argv[1:] if not a.startswith('--')]
REF = sys.argv[sys.argv.index('--ref') + 1] if '--ref' in sys.argv else None
if REF in ARG:
    ARG.remove(REF)
PNG = ARG[0]
LIBDIR = ARG[1]
PRAC = os.path.abspath(ARG[2])


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


if REF:
    JAVA = subprocess.run(['git', 'show', REF + ':' + JAVA_CESTA], cwd=KOREN, capture_output=True, text=True, check=True).stdout
else:
    JAVA = open(os.path.join(KOREN, JAVA_CESTA), encoding='utf-8').read()
HTML = stranka_z_javy(JAVA).encode('utf-8')


def nacti(jm):
    lib = ctypes.CDLL(os.path.join(LIBDIR, 'lib%s_b309.so' % jm))
    getattr(lib, jm + '_dalsi').argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    getattr(lib, jm + '_log').restype = ctypes.c_char_p
    if jm == 'ps1':
        lib.ps1_start.restype = ctypes.c_char_p
        lib.ps1_start.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    return lib


LIB = {'ps1': nacti('ps1'), 'sega': nacti('sega')}
PZ = threading.Lock()
NA_OBRAZOVCE = ['']            # 'ps1' / 'sega' / '' (menu appky -> H.264)
CLIENTLOG = []
H264 = {'spojeni': 0, 'otevrenych': 0}
PROUD = {'ps1': 0, 'sega': 0}  # kolikrat prohlizec otevrel proud z C++


def jpeg(barva):
    b = io.BytesIO()
    Image.new('RGB', (64, 36), barva).save(b, 'JPEG')
    return b.getvalue()


JPEG_TELEFON = jpeg((90, 90, 90))      # obraz "z Javy" (MJPEG / JPEG) - sedy


class Server(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *a):
        pass

    def posli(self, kod, typ, data, hlavicky=None):
        self.send_response(kod)
        self.send_header('Content-Type', typ)
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        for k, v in (hlavicky or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def proud(self, jm):
        """proud z C++ - presne jako Java napTvWebJadroProud: bajty z proudDalsiNative"""
        lib = LIB[jm]
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n'
                         b'Cache-Control: no-store\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        self.close_connection = True
        if NA_OBRAZOVCE[0] != jm:
            return
        with PZ:
            PROUD[jm] += 1
        cid = getattr(lib, jm + '_otevri')()
        kap = 4 * 1024 * 1024
        b = ctypes.create_string_buffer(kap)
        posledni = time.monotonic()
        try:
            while NA_OBRAZOVCE[0] == jm:
                n = getattr(lib, jm + '_dalsi')(cid, b, kap, 250, 0)
                if n == -2 ** 31:
                    break
                if n < 0:
                    kap = -n + 65536
                    b = ctypes.create_string_buffer(kap)
                    continue
                if n == 0:
                    if (getattr(lib, jm + '_stav')() & 1) == 0 and time.monotonic() - posledni > 1:
                        break
                    continue
                posledni = time.monotonic()
                self.wfile.write(b.raw[:n])
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            getattr(lib, jm + '_zavri')(cid)

    def h264(self):
        """jako Java /stream.h264: dlouhe spojeni, kousky dat (obsah tu nehraje roli - falesny JMuxer)"""
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: video/h264\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        self.close_connection = True
        with PZ:
            H264['spojeni'] += 1
            H264['otevrenych'] += 1
        try:
            while True:
                self.wfile.write(b'\x00\x00\x00\x01' + bytes(60))
                self.wfile.flush()
                time.sleep(0.04)
        except (BrokenPipeError, ConnectionResetError, OSError):
            pass
        finally:
            with PZ:
                H264['otevrenych'] -= 1

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            d = NA_OBRAZOVCE[0]
            proud = d if d and (getattr(LIB[d], d + '_stav')() & 1) else '0'
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=%d ps1=%d seq=1 h264Seq=1\n'
                                           % (proud, 1 if d == 'sega' else 0, 1 if d == 'ps1' else 0)).encode())
        elif u.path in ('/ps1.stream', '/sega.stream'):
            self.proud(u.path[1:].split('.')[0])
        elif u.path == '/stream.h264':
            self.h264()
        elif u.path in ('/stream.mjpg', '/frame.jpg'):
            self.posli(200, 'image/jpeg', JPEG_TELEFON)
        elif u.path == '/audio.raw':
            self.posli(200, 'application/octet-stream', b'', {'x-nap-audio-seq': '0', 'x-nap-audio-start': '0', 'x-nap-audio-rate': '44100'})
        elif u.path == '/clientlog':
            with PZ:
                CLIENTLOG.append(q.get('m', [''])[0])
            self.posli(200, 'text/plain', b'ok')
        elif u.path == '/quality':
            self.posli(200, 'text/plain', b'tier=2')
        elif u.path == '/vyhlazeni':
            self.posli(200, 'text/plain', b'1')
        else:
            self.posli(404, 'text/plain', b'nic')


# falesny JMuxer (stejne rozhrani jako jmuxer@2: new JMuxer({node,...}), feed({video,duration}))
FALESNY_JMUXER = b"""
window.JMuxer=function(o){this.o=o;window._jmuxeru=(window._jmuxeru||0)+1;};
window.JMuxer.prototype.feed=function(d){window._feeds=(window._feeds||0)+1;};
window.JMuxer.prototype.destroy=function(){};
"""

STAV_JS = """(function(){var c=document.getElementById('ap'),v=document.getElementById('v'),h=document.getElementById('h264v');
 var e=document.elementFromPoint(innerWidth/2,innerHeight/2),a=window.napAp||{};
 return {nahore:e?(e.id||e.tagName):'',cv:c?getComputedStyle(c).display:'-',v:getComputedStyle(v).display,h:getComputedStyle(h).display,
   chce:!!a.chce,on:!!a.on,druh:a.druh||'',ver:a.ver||0,W:a.W||0,pak:a.pak||0,feeds:window._feeds||0};})()"""


def main():
    vystup = []
    chyb = [0]
    kontrol = [0]

    def tisk(s):
        print(s, flush=True)
        vystup.append(s)

    def over(ok, co):
        kontrol[0] += 1
        if not ok:
            chyb[0] += 1
        tisk(('OK     ' if ok else 'CHYBA  ') + co)

    os.makedirs(PRAC, exist_ok=True)
    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    def ps1_zapni():
        r = LIB['ps1'].ps1_start(PNG.encode(), PRAC.encode()).decode()
        NA_OBRAZOVCE[0] = 'ps1'
        return r

    def ps1_vypni():
        NA_OBRAZOVCE[0] = ''
        LIB['ps1'].ps1_stop()

    def sega_zapni():
        r = LIB['sega'].sega_start()
        NA_OBRAZOVCE[0] = 'sega'
        return r

    def sega_vypni():
        NA_OBRAZOVCE[0] = ''
        LIB['sega'].sega_stop()

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        try:
            br = p.chromium.launch()
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome')
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        page = ctx.new_page()
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))

        def cesta(route):
            u = route.request.url
            if u.startswith('http://127.0.0.1'):
                route.continue_()
            elif 'jmuxer' in u:
                route.fulfill(status=200, content_type='application/javascript', body=FALESNY_JMUXER)
            else:
                route.abort()
        page.route('**/*', cesta)

        def stav():
            return page.evaluate(STAV_JS)

        def sleduj(s, kolik=6, krok=0.5):
            """kolik vzorku po krok s - vraci seznam stavu"""
            r = []
            for _ in range(kolik):
                r.append(stav())
                time.sleep(krok)
            return r

        def popis(s):
            return 'nahore=%s canvas=%s #v=%s h264=%s proud=%s ver=%d snimku=%d' % (
                s['nahore'], s['cv'], s['v'], s['h'], s['druh'] or '-', s['ver'], s['pak'])

        def proud_videt(vz, druh, ver):
            if os.environ.get('LADENI'):
                for x in vz:
                    tisk('   VZOREK ' + str(x))
            return all(s['nahore'] == 'ap' and s['v'] == 'none' and s['h'] == 'none' and s['cv'] != 'none'
                       and s['druh'] == druh and s['ver'] == ver for s in vz) and vz[-1]['pak'] > vz[0]['pak'] + 5

        def cekej_na_proud(druh, ver, limit=10.0):
            """az se v prohlizeci objevi obraz z C++ (canvas navrchu, prvni snimek); vraci cas [s] nebo None"""
            t0 = time.time()
            while time.time() - t0 < limit:
                s = stav()
                if s['nahore'] == 'ap' and s['druh'] == druh and s['ver'] == ver and s['W'] > 0:
                    return time.time() - t0
                time.sleep(0.1)
            return None

        def cekej_na_h264(limit=10.0):
            t0 = time.time()
            f0 = stav()['feeds']
            while time.time() - t0 < limit:
                s = stav()
                if s['nahore'] == 'h264v' and s['feeds'] > f0 + 3:
                    return time.time() - t0
                time.sleep(0.1)
            return None

        tisk('B309: prepnuti hry / emulatoru na TV / PC bez F5 - stranka %s, Chromium, falesny JMuxer (H.264 cesta bezi jako u Reneho)'
             % ('z commitu ' + REF if REF else 'z MainActivity.java (B309)'))
        tisk('stranka: %d bajtu' % len(HTML))

        # ---------------- 1) menu appky -> H.264 ----------------
        tisk('\n--- 1) menu appky: na TV H.264 (obraz telefonu) ---')
        page.goto(url)
        t0 = time.time()
        while time.time() - t0 < 8 and not (stav()['feeds'] > 5 and stav()['h'] != 'none'):
            time.sleep(0.2)
        s = stav()
        tisk('   ' + popis(s) + ', H.264 dat %d' % s['feeds'])
        over(s['nahore'] == 'h264v' and s['feeds'] > 5, 'menu: prohlizec prehrava H.264 (JMuxer nacten, data tecou) a video je navrchu')

        # ---------------- 2) start PS1 (BIOS) -> proud z C++ ----------------
        tisk('\n--- 2) start PS1 (BIOS bez disku) - na TV ma naskocit proud PS1 z C++ ---')
        over(ps1_zapni() == 'PS1_BIOS_OK', 'PS1: skutecny ps1BootDoMonitoru -> PS1_BIOS_OK')
        t = cekej_na_proud('ps1', 3)
        vz = sleduj(page, 8)
        tisk('   obraz PS1 v prohlizeci za %s s po startu jadra' % ('%.1f' % t if t is not None else 'NIKDY'))
        tisk('   ' + popis(vz[0]))
        tisk('   ' + popis(vz[-1]))
        f0 = vz[-1]['feeds']
        time.sleep(1.0)
        over(t is not None and proud_videt(vz, 'ps1', 3),
             'PS1 nabehl: canvas s proudem z C++ je navrchu a ZUSTAVA navrchu (4 s), obraz bezi (Rene videl cerno / stary obraz az do F5)')
        over(stav()['feeds'] == f0 and H264['otevrenych'] == 0, 'H.264 zastaveno (prohlizec uz ho necte, spojeni zavrene)')

        # ---------------- 3) prepnuti hry (jadro se restartuje) ----------------
        tisk('\n--- 3) prepnuti hry: jadro PS1 se ~1,5 s restartuje (proud chvili neni) ---')
        h0 = H264['spojeni']
        p0 = PROUD['ps1']
        ps1_vypni()
        mezera = []
        for _ in range(3):
            time.sleep(0.5)
            mezera.append(stav())
        tisk('   v mezere: ' + popis(mezera[-1]))
        over(ps1_zapni() == 'PS1_BIOS_OK', 'PS1: nova "hra" nastartovana')
        t0 = time.time()
        while time.time() - t0 < 10 and not (stav()['on'] and stav()['W'] > 0 and stav()['pak'] > mezera[-1]['pak'] + 5):
            time.sleep(0.1)
        t = time.time() - t0
        vz = sleduj(page, 8)
        tisk('   novy obraz za %.1f s po startu jadra; ' % t + popis(vz[-1]))
        over(all(s['nahore'] == 'ap' for s in mezera), 'v mezere (1,5 s) zustava posledni obraz hry - zadne cerno, zadne problikavani telefonu')
        over(H264['spojeni'] == h0, 'v kratke mezere se H.264 nezapina (telefon nemusi rozjizdet koder)')
        over(proud_videt(vz, 'ps1', 3) and PROUD['ps1'] > p0, 'nova hra: prohlizec se sam pripojil znovu a novy obraz je videt (bez F5)')

        # ---------------- 4) PS1 -> menu -> Sega ----------------
        tisk('\n--- 4) PS1 -> menu appky (5 s) -> Sega ---')
        ps1_vypni()
        t = cekej_na_h264()
        s = stav()
        tisk('   menu: H.264 za %s s; ' % ('%.1f' % t if t is not None else 'NIKDY') + popis(s) + ', H.264 dat %d' % s['feeds'])
        over(t is not None and t > 2.0 and s['cv'] == 'none', 'menu: po odkladu (2,5 s) zpet H.264 (obraz telefonu), canvas schovany')
        time.sleep(1.0)
        over(sega_zapni() == 1, 'Sega: skutecne vlakno emulace bezi')
        t = cekej_na_proud('sega', 2)
        vz = sleduj(page, 8)
        tisk('   Sega za %s s; ' % ('%.1f' % t if t is not None else 'NIKDY') + popis(vz[-1]))
        over(t is not None and proud_videt(vz, 'sega', 2), 'Sega nabehla: canvas s proudem Segy z C++ je navrchu a zustava, obraz bezi (bez F5)')

        # ---------------- 5) Sega -> PS1 rovnou ----------------
        tisk('\n--- 5) Sega -> PS1 rovnou (prepnuti emulatoru za 1 s) ---')
        sega_vypni()
        time.sleep(1.0)
        over(ps1_zapni() == 'PS1_BIOS_OK', 'PS1 znovu')
        t = cekej_na_proud('ps1', 3)
        vz = sleduj(page, 8)
        tisk('   PS1 za %s s; ' % ('%.1f' % t if t is not None else 'NIKDY') + popis(vz[-1]))
        over(t is not None and proud_videt(vz, 'ps1', 3), 'Sega -> PS1: prohlizec prepnul proud sam, obraz PS1 je videt a zustava (bez F5)')

        # ---------------- 6) F5 behem hry ----------------
        tisk('\n--- 6) F5 behem hry (kdyby nekdo zmackl) ---')
        page.reload()
        t = cekej_na_proud('ps1', 3)
        vz = sleduj(page, 4)
        tisk('   za %s s; ' % ('%.1f' % t if t is not None else 'NIKDY') + popis(vz[-1]))
        over(t is not None and t < 3.0 and proud_videt(vz, 'ps1', 3), 'po F5 je obraz PS1 hned zpet (bez H.264 mezikroku)')

        # ---------------- 7) konec hry -> H.264 ----------------
        tisk('\n--- 7) konec hry: zpet do menu appky ---')
        ps1_vypni()
        t = cekej_na_h264()
        s = stav()
        tisk('   H.264 za %s s; ' % ('%.1f' % t if t is not None else 'NIKDY') + popis(s))
        over(t is not None, 'menu: zpet H.264 (obraz telefonu)')

        with PZ:
            cl = list(CLIENTLOG)
        hl = [m for m in cl if m.startswith('B309 HLIDAC')]
        tisk('\nhlasky prohlizece do logu appky: %d (z toho B309 HLIDAC %d)' % (len(cl), len(hl)))
        for m in hl[:5]:
            tisk('   ' + m)
        if os.environ.get('LADENI'):
            for m in cl:
                tisk('   LOG ' + m)
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2] if chyby_js else ''))
        br.close()
    srv.shutdown()
    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol[0], chyb[0]))
    jm = 'VYSTUP_test_b309_prepnuti%s.txt' % ('_' + REF if REF else '')
    with open(os.path.join(ZDE, jm), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
