#!/usr/bin/env python3
"""B308: KLAVESNICE PC V TV PROHLIZECI -> PS1 (konec-koncu).

  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java,
    napTvWebWriteHtml),
  - tenhle Python server = to, co v appce dela Java: /status (ps1=1, kdyz je
    v appce otevrena stranka PS1), /klavesa a /klavesy -> pro PS1 rovnou do C++
    (napTvWebJednaKlavesa -> NativePs1CoreBridge.pcKlavesaSafe),
  - C++ = SKUTECNY nap_ps1_native.cpp (knihovna z ps1_harness.cpp): JNI
    ps1PcKlavesa, vlakno emulace s hlidacem klavesnice, jadro cte tlacitka pres
    retro_input_state presne jako PCSX.

Pouziti: python3 test_b308_klavesnice.py KNIHOVNA.so PRACOVNI_ADRESAR
"""
import ctypes
import http.server
import os
import re
import shutil
import sys
import threading
import time
import urllib.parse

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
LIB = ctypes.CDLL(sys.argv[1] if len(sys.argv) > 1 else 'libps1_b308.so')
PRAC = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else 'prac_klavesnice')
LIB.h_start.restype = ctypes.c_char_p
LIB.h_start.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
LIB.h_stop.restype = ctypes.c_char_p
LIB.h_klavesa.restype = ctypes.c_char_p
LIB.h_klavesa.argtypes = [ctypes.c_char_p, ctypes.c_int]
LIB.h_bity_jadra.restype = ctypes.c_uint
LIB.h_denik.restype = ctypes.c_char_p


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


JAVA = open(os.path.join(KOREN, JAVA_CESTA), encoding='utf-8').read()
HTML = stranka_z_javy(JAVA).encode('utf-8')
PZ = threading.Lock()
KLAVESY = []          # (code, dolu, odpoved)
VYPADEK = threading.Event()      # "spadla Wi-Fi": /klavesa(y) neodpovida
PS1_STRANKA = [True]


def jedna(code, dolu):
    # MainActivity.napTvWebJednaKlavesa: stranka PS1 v appce -> rovnou C++
    if PS1_STRANKA[0]:
        r = LIB.h_klavesa(code.encode('utf-8'), 1 if dolu else 0).decode()
    else:
        r = 'NENI_NA_STRANCE'
    with PZ:
        KLAVESY.append((code, dolu, r))
    return r


class Server(http.server.BaseHTTPRequestHandler):
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
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        g = lambda k, d='': q.get(k, [d])[0]
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            # Java: " ps1=" + (isPs1OwnerUrl(curUrl3) ? 1 : 0)
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=0 sega=0 ps1=%d seq=0 '
                                           'mirror=APP h264Seq=0\n' % (1 if PS1_STRANKA[0] else 0)).encode())
        elif u.path in ('/klavesa', '/klavesy'):
            if VYPADEK.is_set():                      # Wi-Fi spadla - pozadavek nedojde
                time.sleep(5)
                return
            if u.path == '/klavesa':
                r = jedna(g('code'), int(g('dolu', '1') or 1) != 0)
            else:
                d = ''
                for par in u.query.split('&'):
                    if par.startswith('d='):
                        d = par[2:]
                        break
                odp = []
                for kus in d.split(';')[:64]:
                    f = kus.split(',')
                    if len(f) < 5:
                        continue
                    odp.append(jedna(urllib.parse.unquote_plus(f[0]), f[2] != '0'))
                r = '\n'.join(odp)
            self.posli(200, 'text/plain', r.encode())
        else:
            self.posli(404, 'text/plain', b'nic')


def main():
    vystup = []

    def tisk(s):
        print(s, flush=True)
        vystup.append(s)

    chyb = [0]
    kontrol = [0]

    def over(ok, co):
        kontrol[0] += 1
        if not ok:
            chyb[0] += 1
        tisk(('OK     ' if ok else 'CHYBA  ') + co)

    if os.path.exists(PRAC):
        shutil.rmtree(PRAC)
    os.makedirs(PRAC)
    cue = os.path.join(PRAC, 'hra.cue')
    with open(cue, 'w') as f:
        f.write('FILE "hra.bin" BINARY\n  TRACK 01 MODE2/2352\n')
    tisk('B308: klavesnice PC v TV prohlizeci (Chromium) -> server jako Java -> SKUTECNY nap_ps1_native.cpp')
    tisk('stranka z MainActivity.java: %d bajtu' % len(HTML))
    over('ps1=' in JAVA and 'isPs1OwnerUrl(napTvWebCurrentUrl)' in JAVA and 'pcKlavesaSafe(code, dolu)' in JAVA,
         'Java: /status hlasi ps1=..., klavesa na strance PS1 jde do NativePs1CoreBridge.pcKlavesaSafe')
    over(LIB.h_start(PRAC.encode(), cue.encode()).decode() == 'PS1_HRA_OK', 'PS1 bezi (vlakno emulace cte tlacitka 60x za sekundu)')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    def bity(cekej=0.15):
        time.sleep(cekej)
        return LIB.h_bity_jadra()

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        args = ['--autoplay-policy=no-user-gesture-required']
        try:
            br = p.chromium.launch(args=args)
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome', args=args)
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        page = ctx.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        page.goto(url)
        try:
            page.wait_for_function('window.napVPs1===true', timeout=8000)
            zap = True
        except Exception:
            zap = False
        over(zap, 'stranka z /status (ps1=1) poznala PS1 - klavesnice se zapnula sama')
        page.wait_for_timeout(300)
        popis = page.evaluate("(function(){var d=[].slice.call(document.querySelectorAll('div')).filter(function(e){"
                              "return /^PS1 - /.test(e.textContent||'');});return d.length?d[0].textContent:'';})()")
        tisk('   napoveda v rohu: ' + popis)
        over(popis.startswith('PS1 - sipky/WASD smer, K/mezernik KRIZEK, L KOLECKO, J CTVEREC, I TROJUHELNIK'),
             'v rohu obrazovky je napoveda klaves pro PS1')
        page.mouse.click(640, 360)        # zamereni do okna (jako klepnuti uzivatele)

        # 1) jednotlive klavesy -> co jadro cte
        mapa = [('k', 0, 'KRIZEK'), ('l', 8, 'KOLECKO'), ('j', 1, 'CTVEREC'), ('i', 9, 'TROJUHELNIK'),
                ('ArrowUp', 4, 'NAHORU'), ('w', 4, 'NAHORU'), ('ArrowDown', 5, 'DOLU'), ('s', 5, 'DOLU'),
                ('ArrowLeft', 6, 'VLEVO'), ('a', 6, 'VLEVO'), ('ArrowRight', 7, 'VPRAVO'), ('d', 7, 'VPRAVO'),
                ('q', 10, 'L1'), ('e', 11, 'R1'), ('z', 12, 'L2'), ('c', 13, 'R2'), ('Enter', 3, 'START'),
                ('p', 3, 'START'), ('Backspace', 2, 'SELECT'), ('o', 2, 'SELECT'), (' ', 0, 'KRIZEK')]
        spatne = []
        for kl, bit, jm in mapa:
            page.keyboard.down(kl)
            b = bity(0.18)
            posl = page.evaluate('window.napPosledni||""')
            page.keyboard.up(kl)
            b2 = bity(0.15)
            if b != (1 << bit) or b2 != 0 or posl != jm:
                spatne.append('%s:%x/%x/%s' % (kl, b, b2, posl))
        over(not spatne, 'vsech %d klaves z prohlizece dojde do jadra PS1 a po pusteni se pusti; prohlizec ukaze, co klavesa udelala %s'
             % (len(mapa), spatne))

        # 2) vic klaves najednou (diagonala + strelba)
        page.keyboard.down('w')
        page.keyboard.down('d')
        page.keyboard.down('k')
        over(bity(0.2) == (1 << 4) | (1 << 7) | (1 << 0), 'tri klavesy drzene najednou (NAHORU + VPRAVO + KRIZEK)')
        page.keyboard.up('w')
        page.keyboard.up('d')
        page.keyboard.up('k')
        over(bity(0.2) == 0, 'vse pusteno')

        # 3) drzena klavesa: prohlizec posila "Zije" -> C++ ji nepusti
        page.keyboard.down('ArrowRight')
        bity(0.25)                          # stisk dojde do jadra
        t0 = time.time()
        drzi = True
        while time.time() - t0 < 4.0:
            if LIB.h_bity_jadra() != (1 << 7):
                drzi = False
            time.sleep(0.1)
        over(drzi, 'klavesa drzena 4 s - jadro ji celou dobu vidi (prohlizec posila "Zije" kazdych 0,8 s)')
        with PZ:
            zije = sum(1 for k in KLAVESY if k[0] == 'Zije')
        over(zije >= 3, 'behem drzeni prisel "Zije" %dx' % zije)
        page.keyboard.up('ArrowRight')
        over(bity(0.2) == 0, 'pusteno')

        # 4) okno ztratilo zamereni -> prohlizec posle PustVse
        page.keyboard.down('l')
        page.keyboard.down('a')
        b = bity(0.2)
        page.evaluate("window.dispatchEvent(new Event('blur'))")
        b2 = bity(0.3)
        over(b == (1 << 8) | (1 << 6) and b2 == 0, 'okno prohlizece ztratilo zamereni -> PustVse -> C++ vse pustilo')
        page.keyboard.up('l')
        page.keyboard.up('a')

        # 5) spadla Wi-Fi s drzenou klavesou -> C++ ji pusti samo
        page.mouse.click(640, 360)
        page.keyboard.down('KeyK' if False else 'k')
        b = bity(0.25)
        VYPADEK.set()
        t0 = time.time()
        while LIB.h_bity_jadra() and time.time() - t0 < 6:
            time.sleep(0.05)
        dt = time.time() - t0
        over(b == 1 and LIB.h_bity_jadra() == 0 and dt < 3.6,
             'spadla Wi-Fi s drzenym KRIZKEM -> C++ ho pustilo samo za %.1f s (postava nebezi do nekonecna)' % dt)
        page.keyboard.up('k')
        VYPADEK.clear()
        page.wait_for_timeout(5500)          # prohlizec zjisti chybu spojeni a posle PustVse
        page.keyboard.down('i')
        b = bity(0.3)
        page.keyboard.up('i')
        over(b == (1 << 9) and bity(0.3) == 0, 'po obnoveni spojeni klavesnice zase funguje')

        # 6) rychle psani (fronta /klavesy v jednom pozadavku)
        page.keyboard.type('wasdkljiqe', delay=5)
        page.wait_for_timeout(800)
        over(LIB.h_bity_jadra() == 0, 'rychla sekvence 10 klaves - na konci nic nezustalo drzene')

        # 7) mimo stranku PS1 (appka prepnula jinam) -> klavesnice pro PS1 se vypne
        PS1_STRANKA[0] = False
        try:
            page.wait_for_function('window.napVPs1===false', timeout=5000)
            vyp = True
        except Exception:
            vyp = False
        over(vyp, 'appka uz neni na strance PS1 (ps1=0) -> prohlizec klavesy pro PS1 neposila')

        denik = LIB.h_denik().decode('utf-8', 'replace')
        over('TV KLAVESNICE: KeyK dolu -> KRIZEK' in denik and 'prohlizec pustil vse' in denik
             and 'prohlizec se 2,5 s neozval' in denik, 'denik PS1: stisky z TV, PustVse i hlidac')
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % chyby_js[:3])
        br.close()
    LIB.h_stop()
    tisk('\nVYSLEDEK: %d kontrol, %d CHYB' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b308_klavesnice.txt'), 'w') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
