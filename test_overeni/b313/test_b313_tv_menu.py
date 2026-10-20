#!/usr/bin/env python3
"""B313: HLAVNI MENU NA TV / PC OSTRE (konec-koncu, Chromium) - navazuje na B311.

Rene: "Grafika hlavniho menu je rozmazana na TV ci PC prenosu LAN." (B312: 854x480,
JPEG 76, nikdy nezjemneno - v menu se porad neco hybe.)

B313: obraz menu pro TV podle kvality v OPTIONS (VYSOKA = 1280x720), na TV v klidu stoji
ozdoby, stojici casti se zjemni na kvalitu 94 i kdyz se konzole na karte toci (kodek: vic
obdelniku = vic paketu na snimek s priznakem 8). Tady SKUTECNA stranka z appky dostava
SKUTECNY proud a porovna se, co ma v canvasu, s obrazem, ze ktereho proud jde.

  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java),
  - Python server = to, co v appce dela Java: /status (proud=menu, menu=1), /menu.stream
    (bajty z proudDalsiNative), /klavesa a /klavesy (napTvWebJednaKlavesa: klavesa -> menu),
  - libmenu = SKUTECNY nap_menu_gl.cpp (tv_menu_lib.cpp, Mesa) - menu jako v telefonu.

Kontroluje: canvas s proudem menu je navrchu a obraz bezi (1280x720, neni cerny), v klidu je
obraz v prohlizeci OSTRY (PSNR napisu nahore i dole proti obrazu menu >= 38 dB - kvalita 94),
zmena kvality TV prepne velikost obrazu za chodu (854x480 a zpet), popis klaves MENU, sipky /
A D / Home / End listuji, Enter spusti kartu, na zamcene SEGE nic, zadna chyba JavaScriptu.

Pouziti: python3 test_b313_tv_menu.py ADRESAR_KNIHOVNY PRACOVNI   (libmenu_b313.so, viz JAK_SPUSTIT.txt)
"""
import base64
import ctypes
import http.server
import io
import os
import re
import socket
import sys
import threading
import time
import urllib.parse

os.environ.setdefault('EGL_PLATFORM', 'surfaceless')
ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
LIBDIR = sys.argv[1]
PRAC = os.path.abspath(sys.argv[2])
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


def menu_klavesa_z_javy(src):
    """napTvMenuKlavesa z MainActivity.java: kod klavesy prohlizece -> klavesa menu (jmeno konstanty)."""
    telo = src[src.index('private static int napTvMenuKlavesa(String code)'):]
    telo = telo[:telo.index('default: return 0;')]
    mapa = {}
    for radek in re.findall(r'((?:case "\w+": )+)return NativeMenuBridge\.(KL_\w+);', telo):
        for kod in re.findall(r'case "(\w+)":', radek[0]):
            mapa[kod] = radek[1]
    return mapa


HTML = stranka_z_javy(JAVA).encode('utf-8')
MAPA = menu_klavesa_z_javy(JAVA)
KL = dict(re.findall(r'(KL_\w+) = (\d+)', open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java'), encoding='utf-8').read()))
KL = {k: int(v) for k, v in KL.items()}

lib = ctypes.CDLL(os.path.join(LIBDIR, 'libmenu_b313.so'))
lib.menu_nastav_c.argtypes = [ctypes.c_float, ctypes.c_int]
lib.menu_grab_c.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.menu_start.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
lib.menu_dalsi.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib.menu_log_c.restype = ctypes.c_char_p
lib.menu_snimku_c.restype = ctypes.c_longlong

PZ = threading.Lock()
KLAVESY = []      # (kod, vysledek C++)
PROUDU = [0]
CLIENTLOG = []


def jedna_klavesa(code, dolu):
    """jako Java napTvWebJednaKlavesa, kdyz je videt menu (menuPlocha != null)"""
    if not dolu:
        return 'MENU'
    kl = MAPA.get(code)
    if not kl:
        return 'MENU'
    r = lib.menu_klavesa_c(KL[kl])
    with PZ:
        KLAVESY.append((code, r))
    return 'MENU'


class Server(http.server.BaseHTTPRequestHandler):
    # HTTP/1.0 = kazda odpoved zavre spojeni, jako Java server v appce u klaves
    # (napTvWebKlavesa -> napTvWebHeader(..., close = true): Connection: close).

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

    def proud(self):
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        self.close_connection = True
        with PZ:
            PROUDU[0] += 1
        cid = lib.menu_otevri()
        kap = 4 * 1024 * 1024
        b = ctypes.create_string_buffer(kap)
        try:
            while True:
                n = lib.menu_dalsi(cid, b, kap, 250, 0)
                if n == -2 ** 31:
                    break
                if n < 0:
                    kap = -n + 65536
                    b = ctypes.create_string_buffer(kap)
                    continue
                if n == 0:
                    continue
                self.wfile.write(b.raw[:n])
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError, OSError):
            pass
        finally:
            lib.menu_zavri(cid)

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            proud = 'menu' if (lib.menu_stav() & 1) else '0'
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=0 ps1=0 menu=1 seq=1 h264Seq=1\n' % proud).encode())
        elif u.path == '/menu.stream':
            self.proud()
        elif u.path == '/klavesa':
            r = jedna_klavesa(q.get('code', [''])[0], q.get('dolu', ['0'])[0] == '1')
            self.posli(200, 'text/plain', r.encode())
        elif u.path == '/klavesy':
            odp = []
            for kus in q.get('d', [''])[0].split(';'):
                p = kus.split(',')
                if len(p) >= 3:
                    odp.append(jedna_klavesa(urllib.parse.unquote(p[0]), p[2] == '1'))
            self.posli(200, 'text/plain', '\n'.join(odp).encode())
        elif u.path in ('/stream.mjpg', '/frame.jpg'):
            self.posli(404, 'text/plain', b'nic')
        elif u.path == '/stream.h264':
            self.posli(404, 'text/plain', b'nic')
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


FALESNY_JMUXER = b"""
window.JMuxer=function(o){this.o=o;};
window.JMuxer.prototype.feed=function(d){};
window.JMuxer.prototype.destroy=function(){};
"""

STAV_JS = """(function(){var c=document.getElementById('ap'),e=document.elementFromPoint(innerWidth/2,innerHeight/2),a=window.napAp||{};
 var kn=[].slice.call(document.querySelectorAll('div')).filter(function(d){return /^MENU - sipky/.test(d.textContent)&&d.children.length===0;});
 var jas=-1;try{if(a.cv){var x=a.cv.getContext('2d'),d=x.getImageData(0,0,a.cv.width,a.cv.height).data,s=0;for(var i=0;i<d.length;i+=64)s+=d[i]+d[i+1]+d[i+2];jas=s/(d.length/64)/3;}}catch(err){jas=-2;}
 return {nahore:e?(e.id||e.tagName):'',cv:c?getComputedStyle(c).display:'-',druh:a.druh||'',ver:a.ver||0,W:a.W||0,H:a.H||0,pak:a.pak||0,
   menu:!!window.napVMenu,popis:kn.length?kn[0].textContent:'',popisVidet:kn.length?getComputedStyle(kn[0]).display:'none',jas:jas};})()"""


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

    tisk('B313: hlavni menu na TV / PC - stranka z MainActivity.java (%d B), proud ze skutecneho nap_menu_gl.cpp (Mesa)' % len(HTML))
    tisk('klavesy menu z napTvMenuKlavesa: ' + ', '.join('%s=%s' % kv for kv in sorted(MAPA.items())))
    over(MAPA == {'ArrowLeft': 'KL_VLEVO', 'KeyA': 'KL_VLEVO', 'ArrowRight': 'KL_VPRAVO', 'KeyD': 'KL_VPRAVO', 'Enter': 'KL_POTVRD',
                  'NumpadEnter': 'KL_POTVRD', 'Space': 'KL_POTVRD', 'Home': 'KL_ZACATEK', 'End': 'KL_KONEC'},
         'Java: klavesy prohlizece -> menu (sipky / A D, Enter / mezernik, Home / End)')
    n = lib.menu_start(os.path.join(KOREN, 'app/src/main/assets/').encode(), 360, 780)
    lib.menu_nastav_c(60.0, 2)            # jako Java menuNastav: 60 Hz, kvalita TV VYSOKA
    t0 = time.time()
    while not lib.menu_pripraveno_c() and time.time() - t0 < 90:
        time.sleep(0.1)
    over(n == 7 and lib.menu_pripraveno_c() == 1, 'C++ menu bezi (7 karet, pripraveno za %.1f s na Mesa / CPU)' % (time.time() - t0))

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        br = p.chromium.launch()
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        page = ctx.new_page()
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))

        # jen adresy MIMO server appky (JMuxer z internetu -> falesny, jinak nic). Pozadavky na
        # server (klavesy, proud) se NEzachytavaji: zachyceny pozadavek v sync API Playwrightu
        # ceka, az Python zavola dalsi prikaz Playwrightu - klavesy by pak chodily o jednu pozde.
        def cesta(route):
            if 'jmuxer' in route.request.url:
                route.fulfill(status=200, content_type='application/javascript', body=FALESNY_JMUXER)
            else:
                route.abort()
        page.route(re.compile(r'^(?!http://127\.0\.0\.1)'), cesta)

        def stav():
            return page.evaluate(STAV_JS)

        page.goto(url)
        t0 = time.time()
        s = stav()
        while time.time() - t0 < 30 and not (s['druh'] == 'menu' and s['W'] > 0 and s['pak'] > 10):
            time.sleep(0.2)
            s = stav()
        tisk('   ' + str(s))
        over(s['druh'] == 'menu' and s['nahore'] == 'ap' and s['cv'] != 'none' and s['ver'] == 3 and s['W'] == 1280 and s['H'] == 720,
             'prohlizec bere proud menu z C++ (/menu.stream, JPEG po castech v3, 1280x720 - drive 854x480) a canvas je navrchu (za %.1f s)' % (time.time() - t0))
        p0 = s['pak']
        time.sleep(3)
        s = stav()
        over(s['pak'] > p0 + 10 and s['jas'] > 8, 'obraz menu bezi (%d snimku za 3 s; Mesa na CPU) a neni cerny (prumerny jas %.1f)' % (s['pak'] - p0, s['jas']))

        # ---- B313: ostrost v klidu - canvas prohlizece proti obrazu, ze ktereho jde proud ----
        import numpy as np
        from PIL import Image

        def zdroj():
            kap = 1280 * 720
            buf = (ctypes.c_uint32 * kap)()
            for _ in range(200):
                r = lib.menu_grab_c(buf, kap)
                if r > 0:
                    W, H = r >> 16, r & 0xFFFF
                    a = np.frombuffer(buf, dtype=np.uint32, count=W * H).reshape(H, W)
                    return np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255], axis=2).astype(np.float64)
                time.sleep(0.05)
            return None

        def canvas():
            url = page.evaluate("window.napAp.cv.toDataURL('image/png')")
            im = Image.open(io.BytesIO(base64.b64decode(url.split(',', 1)[1]))).convert('RGB')
            return np.asarray(im).astype(np.float64)

        def psnr(a, b):
            mse = ((a - b) ** 2).mean()
            return 99.0 if mse < 1e-9 else 10 * np.log10(255.0 ** 2 / mse)

        time.sleep(5)                      # nastup dobehl, ozdoby stoji, stojici casti se zjemni
        z = zdroj()
        c = canvas()
        Image.fromarray(c.astype(np.uint8)).save(os.path.join(PRAC, 'tv_menu_klid_canvas.png'))
        if z is not None:
            Image.fromarray(z.astype(np.uint8)).save(os.path.join(PRAC, 'tv_menu_klid_zdroj.png'))
        ok = z is not None and z.shape == c.shape
        H = c.shape[0]
        horni = psnr(z[:H // 5], c[:H // 5]) if ok else 0
        spodni = psnr(z[H * 3 // 4:], c[H * 3 // 4:]) if ok else 0
        tisk('   ostrost v klidu (canvas prohlizece proti obrazu menu): horni pas %.1f dB, spodni pas %.1f dB' % (horni, spodni))
        over(ok and horni >= 38.0 and spodni >= 38.0,
             'v klidu je obraz v prohlizeci ostry: napisy nahore %.1f dB a dole %.1f dB (>= 38 = kvalita 94; drive JPEG 76 bez zjemneni)' % (horni, spodni))
        log = (lib.menu_log_c() or b'').decode('utf-8', 'replace')
        tisk('   log proudu: ' + log.strip().replace('\n', ' | ')[:400])

        # ---- B313: zmena kvality TV v OPTIONS za chodu: NIZKA = 854x480, pak zpet VYSOKA ----
        lib.menu_nastav_c(60.0, 0)
        t1 = time.time()
        s = stav()
        while time.time() - t1 < 15 and not (s['W'] == 854 and s['H'] == 480):
            time.sleep(0.2)
            s = stav()
        nizka = (s['W'], s['H'])
        lib.menu_nastav_c(60.0, 2)
        t1 = time.time()
        s = stav()
        while time.time() - t1 < 15 and not (s['W'] == 1280 and s['H'] == 720):
            time.sleep(0.2)
            s = stav()
        over(nizka == (854, 480) and (s['W'], s['H']) == (1280, 720) and s['jas'] > 8,
             'kvalita TV za chodu: NIZKA -> obraz %dx%d, VYSOKA -> %dx%d (prohlizec se prizpusobi sam)' % (nizka[0], nizka[1], s['W'], s['H']))
        page.mouse.move(640, 360)
        page.mouse.click(640, 360)
        time.sleep(1.2)
        s = stav()
        over(s['menu'] and s['popis'].startswith('MENU - sipky / A D listuji, Enter otevre, Home / End') and s['popisVidet'] != 'none',
             'popis klaves vpravo dole: "%s"' % s['popis'][:90])
        page.screenshot(path=os.path.join(PRAC, 'tv_menu_atari.png'))

        def klavesa(k, cekej=1.3):
            page.keyboard.press(k)
            page.wait_for_timeout(int(cekej * 1000))
            return lib.menu_vybrana_c()

        v1 = klavesa('ArrowRight')
        v2 = klavesa('d')
        v3 = klavesa('a')
        v4 = klavesa('End')
        page.screenshot(path=os.path.join(PRAC, 'tv_menu_help.png'))
        v5 = klavesa('Home')
        tisk('   karta po sipce vpravo %d, D %d, A %d, End %d, Home %d' % (v1, v2, v3, v4, v5))
        over((v1, v2, v3, v4, v5) == (1, 2, 1, 6, 0), 'klavesnice PC listuje C++ menu: sipka vpravo, D, A, End (HELP), Home (ATARI)')
        klavesa('ArrowRight')
        pocet = len(KLAVESY)
        klavesa('Enter', 0.6)
        zam = KLAVESY[pocet:]
        over(zam and zam[-1] == ('Enter', 3001), 'Enter na zamcene SEGE: C++ vrati "zamceno" (3001), nic se nespusti %s' % zam)
        klavesa('ArrowLeft')
        pocet = len(KLAVESY)
        klavesa('Enter', 0.6)
        sp = KLAVESY[pocet:]
        over(sp and sp[-1] == ('Enter', 1000), 'Enter na ATARI 130XE: C++ vrati "spustit kartu 0" (1000) -> Java prepne stranku %s' % sp)
        page.screenshot(path=os.path.join(PRAC, 'tv_menu_spusteni.png'))
        over(not chyby_js, 'zadna chyba JavaScriptu' + (': ' + '; '.join(chyby_js) if chyby_js else ''))
        br.close()
    lib.menu_stop()
    tisk('log proudu: ' + (lib.menu_log_c() or b'').decode('utf-8', 'replace')[:300].replace('\n', ' | '))
    vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol[0], chyb[0])
    tisk(vysl)
    open(os.path.join(ZDE, 'VYSTUP_test_b313_tv_menu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb[0] else 0)


if __name__ == '__main__':
    main()
