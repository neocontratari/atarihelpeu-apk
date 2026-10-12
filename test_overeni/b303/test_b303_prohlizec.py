#!/usr/bin/env python3
# B303: KONEC-KONCU test prenosu Atari 130XE (C++) na TV / PC a vkladani z PC.
#
#  - stranka prohlizece = PRESNE ta z appky: vytahne se z MainActivity.java
#    (napTvWebWriteHtml), zadna kopie,
#  - Chromium (Playwright) = prohlizec na PC u TV,
#  - tenhle Python server = to, co v appce dela Java: /status (atariProud=1),
#    /atari.stream (devTvProudDalsi ve smycce, 4 B delka + paket - napTvWebAtariProud),
#    /vloz (POST -> devVloz - napTvWebVloz), /klavesa(y) (devPcKlavesa), /clientlog,
#  - libtvproud.so = SKUTECNY nap_atari_native.cpp (vlakna emulace a kresleni
#    jako v appce, Android nahrazen stuby).
#
#   g++ -std=c++17 -O2 -shared -fPIC -pthread -I../b291/stub -I$JAVA_HOME/include \
#       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/atari -o libtvproud.so tv_lib.cpp
#   python3 test_b303_prohlizec.py          -> VYSTUP_test_b303_prohlizec.txt
import base64
import ctypes
import http.server
import os
import re
import socket
import statistics
import sys
import threading
import time
import urllib.parse

ZDE = os.path.dirname(os.path.abspath(__file__))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
W, H = 672, 240
HOTOVO = 4        # nap::PsaniProgramu::HOTOVO


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

lib = ctypes.CDLL(os.path.join(ZDE, 'libtvproud.so'))
lib.tv_klavesa.restype = ctypes.c_char_p
lib.tv_klavesa.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
lib.tv_dalsi.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
lib.tv_vloz.restype = ctypes.c_char_p
lib.tv_vloz.argtypes = [ctypes.c_char_p, ctypes.c_int]
lib.tv_obraz.argtypes = [ctypes.c_void_p]
lib.tv_seq.restype = ctypes.c_longlong
lib.tv_obrazovka.restype = ctypes.c_char_p
lib.tv_log.restype = ctypes.c_char_p

PZ = threading.Lock()
KLAVESY = []            # (code, znak, dolu, mod, odpoved) jak prisly
DOTAZY = {}             # cesta -> pocet pozadavku
VLOZ = []               # (bajtu, odpoved)
CLIENTLOG = []
ATARI = [True]          # Atari 130XE (C++) je na obrazovce telefonu (atariCppNaObrazovce)
ZAVRIT_PROUD = threading.Event()
PROUD_BAJTU = [0]
LOG = []


def zaloguj():
    s = lib.tv_log().decode('utf-8', 'replace')
    if s:
        with PZ:
            LOG.append(s)
    return s


def jedna(code, znak, dolu, ctrl, mod):
    if mod < 0:
        mod = 2 if ctrl else 0
    r = lib.tv_klavesa(code.encode('utf-8'), znak.encode('utf-8'), mod & 0xFF, 1 if dolu else 0).decode()
    with PZ:
        KLAVESY.append((code, znak, dolu, mod, r))
    return r


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
        # Java napTvWebAtariProud: hlavicka, pak devTvProudDalsi(id, b, 250) dokola
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.wfile.write(b'HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n'
                         b'Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n'
                         b'Pragma: no-cache\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n')
        self.wfile.flush()
        if not ATARI[0]:
            return
        ZAVRIT_PROUD.clear()
        cid = lib.tv_otevri()
        kap = 256 * 1024
        b = ctypes.create_string_buffer(kap)
        try:
            while ATARI[0] and not ZAVRIT_PROUD.is_set():
                n = lib.tv_dalsi(cid, b, kap, 250)
                if n < 0:
                    kap = -n + 65536
                    b = ctypes.create_string_buffer(kap)
                    continue
                if n == 0:
                    continue
                self.wfile.write(b.raw[:n])
                self.wfile.flush()
                PROUD_BAJTU[0] += n
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            lib.tv_zavri(cid)
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
            rezim = lib.tv_klavesa(b'STAV', b'', 0, 1).decode()
            a = 1 if ATARI[0] else 0
            self.posli(200, 'text/plain', ('running=true atari=%d atariCpp=%d atariProud=%d klavRezim=%s sega=0 seq=0 h264Seq=0\n'
                                           % (a, a, a, rezim)).encode())
        elif u.path == '/atari.stream':
            self.proud()
        elif u.path == '/clientlog':
            with PZ:
                CLIENTLOG.append(g('m'))
            self.posli(200, 'text/plain', b'ok')
        elif u.path == '/klavesa':
            r = jedna(g('code'), g('znak'), int(g('dolu', '1') or 1) != 0, int(g('ctrl', '0') or 0) != 0, int(g('mod', '-1') or -1))
            self.posli(200, 'text/plain', r.encode())
        elif u.path == '/klavesy':
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
                uq = urllib.parse.unquote_plus
                odp.append(jedna(uq(f[0]), uq(f[1]), f[2] != '0', f[3] == '1', int(f[4])))
            self.posli(200, 'text/plain', '\n'.join(odp).encode())
        else:
            self.posli(404, 'text/plain', b'nic')     # MJPEG / JPEG / H.264 / zvuk postaru tu nejsou

    def do_POST(self):
        u = urllib.parse.urlsplit(self.path)
        with PZ:
            DOTAZY[u.path] = DOTAZY.get(u.path, 0) + 1
        if u.path != '/vloz':
            self.posli(404, 'text/plain', b'nic')
            return
        n = int(self.headers.get('Content-Length', '0') or 0)
        data = self.rfile.read(min(n, 512 * 1024)) if n > 0 else b''
        r = lib.tv_vloz(data, len(data)).decode('utf-8', 'replace') if data else 'VLOZENI: PRAZDNE'
        with PZ:
            VLOZ.append((len(data), r))
        self.posli(200, 'text/plain; charset=utf-8', r.encode('utf-8'))


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

    obr = lambda: lib.tv_obrazovka().decode()
    tisk('B303: prohlizec (Chromium) <- server jako Java <- SKUTECNY nap_atari_native.cpp (proud z C++, vkladani z PC)')
    tisk('stranka z MainActivity.java: %d bajtu, skriptu %d' % (len(HTML), HTML.count(b'<script>')))
    over(lib.tv_start() == 1, 'Atari 130XE (C++) nastartovalo do READY (vlakna emulace a kresleni jako v appce)')
    zaloguj()

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    def obraz_jadra():
        a = (ctypes.c_int32 * (W * H))()
        wh = lib.tv_obraz(a)
        if wh != ((W << 16) | H):
            return None
        return bytes(memoryview(a).cast('B'))       # ARGB little-endian: B,G,R,A

    def obraz_canvas(page):
        b64 = page.evaluate("""(function(){var a=window.napAp;if(!a||!a.cx||a.W!==672)return '';
            var d=a.cx.getImageData(0,0,a.W,a.H).data,s='',k=0;
            for(var i=0;i<d.length;i+=32768){s+=String.fromCharCode.apply(null,d.subarray(i,Math.min(d.length,i+32768)));}
            return btoa(s);})()""")
        return base64.b64decode(b64) if b64 else None

    def rozdil(page):
        """kolik bodu canvasu (RGBA) se lisi od obrazu z jadra (ARGB)"""
        c = obraz_canvas(page)
        j = obraz_jadra()
        if not c or not j or len(c) != W * H * 4:
            return -1
        r = 0
        for i in range(0, W * H * 4, 4):
            if c[i] != j[i + 2] or c[i + 1] != j[i + 1] or c[i + 2] != j[i] or c[i + 3] != 255:
                r += 1
        return r

    def presne(page, pokusu=6):
        r = -1
        for _ in range(pokusu):
            r = rozdil(page)
            if r == 0:
                break
            time.sleep(0.3)
        return r

    def cekej_klavesy(navic=0.3, maxs=30):
        t0 = time.monotonic()
        while time.monotonic() - t0 < maxs:
            if lib.tv_fronta() == 0 and lib.tv_psani() in (0, HOTOVO, 5):
                break
            time.sleep(0.05)
        time.sleep(navic)

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        args = ['--autoplay-policy=no-user-gesture-required']
        try:
            br = p.chromium.launch(args=args)
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome', args=args)
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        try:
            ctx.grant_permissions(['clipboard-read', 'clipboard-write'], origin=url.rstrip('/'))
        except Exception as e:
            tisk('schranka: povoleni nejde (%s)' % e)
        page = ctx.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())   # zadny internet (JMuxer z CDN)
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        t_start = time.monotonic()
        page.goto(url)

        # 1) stranka sama prepne na proud z C++
        try:
            page.wait_for_function('window.napAp&&napAp.on&&napAp.pak>10', timeout=10000)
            zap = True
        except Exception:
            zap = False
        t_proud = time.monotonic()
        stav = page.evaluate("""(function(){var a=napAp,v=document.getElementById('v'),h=document.getElementById('h264v');
            return {cv:!!a.cv, cvVid:a.cv?getComputedStyle(a.cv).display:'', W:a.W, H:a.H, cw:a.cv?a.cv.getBoundingClientRect().width:0,
            ch:a.cv?a.cv.getBoundingClientRect().height:0, img:getComputedStyle(v).display, imgSrc:v.getAttribute('src')||'',
            h264:getComputedStyle(h).display};})()""")
        tisk('po nacteni: %s' % stav)
        over(zap and stav['cv'] and stav['cvVid'] != 'none' and stav['W'] == W and stav['H'] == H,
             'stranka z appky (/status atariProud=1) sama prepnula na proud z C++: canvas 672x240')
        over(stav['img'] == 'none' and stav['imgSrc'] == '' and stav['h264'] == 'none',
             'MJPEG obrazek i H.264 video vypnute (zadny JPEG ani H.264 z Javy)')
        asp = stav['cw'] / max(1, stav['ch'])
        over(abs(asp - (336 / (240 / 0.88))) < 0.01 and stav['ch'] == 720,
             'canvas roztazeny na okno 1280x720 se spravnym pomerem stran Atari (%.0fx%.0f)' % (stav['cw'], stav['ch']))
        with PZ:
            mj0 = DOTAZY.get('/stream.mjpg', 0) + DOTAZY.get('/frame.jpg', 0) + DOTAZY.get('/stream.h264', 0) + DOTAZY.get('/audio.raw', 0)

        # 2) obraz v canvasu = PRESNE obraz z jadra
        time.sleep(1.0)
        r = presne(page)
        tisk('canvas proti obrazu z jadra (devGrabFrame): %d ruznych bodu ze %d' % (r, W * H))
        over(r == 0, 'obraz v prohlizeci je PRESNE obraz Atari z C++ (vsech 161280 bodu)')

        # 3) 50 snimku/s, zvuk
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah,b:0})')
        b0 = PROUD_BAJTU[0]
        time.sleep(4.0)
        a1 = page.evaluate("""(function(){return {pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah,cil:napAp.cil,
            chyb:napAp.chyb};})()""")
        b1 = PROUD_BAJTU[0]
        fps = (a1['pak'] - a0['pak']) / 4.0
        zv = (a1['zv'] - a0['zv']) / 4.0
        tisk('za 4 s: %.1f snimku/s, zvuk %.0f vzorku/s, podteceni zvuku %d, zahozeno %d, polstar zvuku %.0f ms, chyb %d, %.2f Mbit/s'
             % (fps, zv, a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], a1['cil'] * 1000, a1['chyb'], (b1 - b0) * 8 / 4.0 / 1e6))
        over(47 <= fps <= 52 and a1['chyb'] == 0, 'prohlizec dostava kazdy snimek Atari (50/s), zadna chyba paketu')
        over(zv > 44100 * 0.9 and zv < 44100 * 1.1, 'zvuk Atari jde proudem a hraje ve WebAudio (44100 vzorku/s)')
        over(a1['cil'] <= 0.12, 'polstar zvuku na rychle siti maly (%.0f ms)' % (a1['cil'] * 1000))

        # 4) zpozdeni: klavesa v prohlizeci -> zmena v canvasu
        page.evaluate("""(function(){var a=napAp,c=a.cx,o=c.putImageData;window._lat=[];window._kd=0;
            c.putImageData=function(){if(window._kd){window._lat.push(performance.now()-window._kd);window._kd=0;}return o.apply(c,arguments);};
            document.addEventListener('keydown',function(e){if(/^Key[A-Z]$/.test(e.code))window._kd=performance.now();},true);})()""")
        for ch in 'abcdefghij':
            time.sleep(0.35)
            page.keyboard.press(ch)
        time.sleep(0.6)
        lat = page.evaluate('window._lat')
        cekej_klavesy(0.2)
        for _ in range(10):
            page.keyboard.press('Backspace')
        cekej_klavesy(0.3)
        tisk('zpozdeni klavesa -> obraz v prohlizeci (bez Wi-Fi): %s ms' % ', '.join('%.0f' % x for x in lat))
        med = statistics.median(lat) if lat else 9999
        tisk('  median %.0f ms, nejvic %.0f ms (snimek Atari = 20 ms)' % (med, max(lat) if lat else 9999))
        over(len(lat) == 10 and med <= 60 and max(lat) <= 100, 'klavesa je v obraze prohlizece do 2-3 snimku Atari (median %.0f ms)' % med)

        # 5) vkladani z PC: Ctrl+V program (bez NEW, cestina), text, pretazeny soubor, binarka
        page.keyboard.type('5 REM PRVNI', delay=30)
        page.keyboard.press('Enter')
        cekej_klavesy(0.5)
        with PZ:
            k0 = len(KLAVESY)
            v0 = len(VLOZ)
        program = '10 PRINT "Příliš žluťoučký kůň"\r\n20 ? 6*7\r\n'
        page.evaluate('(t)=>navigator.clipboard.writeText(t)', program)
        page.keyboard.press('Control+v')
        for _ in range(40):
            with PZ:
                if len(VLOZ) > v0:
                    break
            time.sleep(0.05)
        with PZ:
            realny = len(VLOZ) > v0
        if not realny:
            tisk('Ctrl+V v headless Chromiu udalost paste nevyvolal - vkladam udalosti paste primo (stejna obsluha)')
            page.evaluate("""(t)=>{var d=new DataTransfer();d.setData('text/plain',t);
                document.body.dispatchEvent(new ClipboardEvent('paste',{clipboardData:d,bubbles:true,cancelable:true}));}""", program)
        cekej_klavesy(1.0)
        with PZ:
            kv = [k for k in KLAVESY[k0:] if k[0] == 'KeyV']
            vl = list(VLOZ[v0:])
        msg = page.evaluate("(function(){var o=document.getElementById('vlozmsg');return o?o.textContent:'';})()")
        tisk('Ctrl+V (%s): server dostal %s, zprava v okne: %s' % ('skutecne z klavesnice' if realny else 'udalost paste', vl, msg))
        over(realny, 'Ctrl+V v okne prohlizece = udalost paste (schranka -> /vloz)')
        over(not kv, 'Ctrl+V nejde do Atari jako klavesa V (KeyV: %d)' % len(kv))
        over(len(vl) == 1 and vl[0][1].startswith('VLOZENO: 2 RADKU PROGRAMU (BEZ NEW)') and msg.startswith('VLOZENO: 2'),
             'program ze schranky prijat (2 radky, bez NEW), zprava v okne')
        page.keyboard.type('LIST', delay=30)
        page.keyboard.press('Enter')
        cekej_klavesy(1.5)
        s = obr()
        tisk('--- LIST ---\n' + s + '---')
        over('5 REM PRVNI' in s and '10 PRINT "Prilis zlutoucky kun"' in s and '20 ? 6*7' in s and 'ERROR' not in s,
             'LIST: radek 5 zustal (bez NEW), radky 10 a 20 vlozene, cestina bez hacku a carek')
        # obycejny text = klavesy
        with PZ:
            v0 = len(VLOZ)
        if realny:
            page.evaluate('(t)=>navigator.clipboard.writeText(t)', 'RUN\n')
            page.keyboard.press('Control+v')
        else:
            page.evaluate("""(t)=>{var d=new DataTransfer();d.setData('text/plain',t);
                document.body.dispatchEvent(new ClipboardEvent('paste',{clipboardData:d,bubbles:true,cancelable:true}));}""", 'RUN\n')
        time.sleep(0.5)
        cekej_klavesy(1.5)
        s = obr()
        with PZ:
            vl = list(VLOZ[v0:])
        tisk('vlozeny text "RUN": %s\n%s' % (vl, s))
        over(len(vl) == 1 and vl[0][1] == 'NAPSANO: 4 ZNAKU' and 'Prilis zlutoucky kun' in s.split('RUN')[-1] and '42' in s,
             'vlozeny text (ne program) napsany jako klavesy: RUN -> program bezi (Prilis zlutoucky kun, 42)')
        # pretazeny TXT soubor
        with PZ:
            v0 = len(VLOZ)
        page.evaluate("""(t)=>{var d=new DataTransfer();d.items.add(new File([t],'prog.txt',{type:'text/plain'}));
            document.body.dispatchEvent(new DragEvent('dragover',{dataTransfer:d,bubbles:true,cancelable:true}));
            document.body.dispatchEvent(new DragEvent('drop',{dataTransfer:d,bubbles:true,cancelable:true}));}""",
                      '30 ? "PRETAZENO"\n40 END\n')
        time.sleep(0.5)
        cekej_klavesy(1.0)
        page.keyboard.type('LIST 30,40', delay=30)
        page.keyboard.press('Enter')
        cekej_klavesy(1.0)
        s = obr()
        with PZ:
            vl = list(VLOZ[v0:])
        tisk('pretazeny prog.txt: %s' % vl)
        over(len(vl) == 1 and vl[0][1].startswith('VLOZENO: 2 RADKU') and '30 ? "PRETAZENO"' in s and '40 END' in s,
             'pretazeny TXT soubor do okna -> radky 30 a 40 v programu')
        # pretazeny XEX omylem
        with PZ:
            v0 = len(VLOZ)
        page.evaluate("""()=>{var b=new Uint8Array([255,255,0,6,5,6,169,66,141,0,6,96]);var d=new DataTransfer();
            d.items.add(new File([b],'hra.xex'));
            document.body.dispatchEvent(new DragEvent('drop',{dataTransfer:d,bubbles:true,cancelable:true}));}""")
        time.sleep(0.8)
        msg = page.evaluate("(function(){var o=document.getElementById('vlozmsg');return o?o.textContent:'';})()")
        with PZ:
            vl = list(VLOZ[v0:])
        tisk('pretazeny hra.xex: %s, zprava: %s' % (vl, msg))
        over(len(vl) == 1 and 'NENI TEXT' in vl[0][1] and 'NENI TEXT' in msg, 'pretazeny XEX se nepise (zprava TO NENI TEXT)')
        zaloguj()

        # 6) spadle spojeni: prohlizec se pripoji znovu sam, obraz zase presne
        with PZ:
            pr0 = DOTAZY.get('/atari.stream', 0)
        ZAVRIT_PROUD.set()
        time.sleep(2.5)
        with PZ:
            pr1 = DOTAZY.get('/atari.stream', 0)
        r = presne(page)
        pak = page.evaluate('napAp.on&&napAp.pak')
        tisk('spojeni zavreno serverem: pozadavku na proud %d -> %d, rozdil obrazu %d' % (pr0, pr1, r))
        over(pr1 == pr0 + 1 and r == 0 and pak, 'spadle spojeni: prohlizec se pripojil znovu sam a obraz je zase presne')

        # 7) statistika do logu appky (kazdych 30 s)
        while time.monotonic() - t_proud < 32:
            time.sleep(0.5)
        with PZ:
            cl = list(CLIENTLOG)
        stat = [m for m in cl if m.startswith('B303 PROHLIZEC:')]
        tisk('z prohlizece do logu appky: %s' % [m for m in cl if m.startswith('B303')])
        over(any('B303 ATARI PROUD start' == m for m in cl) and stat and 'snimku/s' in stat[-1] and 'hraje' in stat[-1],
             'prohlizec hlasi do logu appky start proudu a kazdych 30 s snimky/s a zvuk')
        with PZ:
            mj1 = DOTAZY.get('/stream.mjpg', 0) + DOTAZY.get('/frame.jpg', 0) + DOTAZY.get('/stream.h264', 0) + DOTAZY.get('/audio.raw', 0)
        if mj1 != mj0:
            with PZ:
                tisk('dotazy: %s' % DOTAZY)
        over(mj1 == mj0, 'behem proudu (i pri znovupripojeni) zadne dotazy na JPEG / MJPEG / H.264 / stary zvuk (%d -> %d)' % (mj0, mj1))

        # 8) Atari zmizi z obrazovky telefonu -> proud konci, stranka zpet na MJPEG
        ATARI[0] = False
        try:
            page.wait_for_function("!napAp.chce&&document.getElementById('v').getAttribute('src')", timeout=5000)
            zpet = True
        except Exception:
            zpet = False
        st2 = page.evaluate("""(function(){var a=napAp,v=document.getElementById('v');return {on:a.on,cv:a.cv?a.cv.style.display:'',
            img:v.style.display,src:(v.getAttribute('src')||'').split('?')[0]};})()""")
        tisk('Atari pryc z obrazovky: %s' % st2)
        # (tenhle server MJPEG nema - obrazek pak prejde na /frame.jpg, i to je postaru)
        over(zpet and st2['cv'] == 'none' and st2['img'] != 'none' and st2['src'] in ('/stream.mjpg', '/frame.jpg'),
             'Atari pryc z obrazovky -> proud konci, stranka zpet na MJPEG / JPEG (postaru)')
        ATARI[0] = True
        try:
            page.wait_for_function('napAp.on&&napAp.pak>5', timeout=6000)
            znovu = True
        except Exception:
            znovu = False
        r = presne(page)
        over(znovu and r == 0, 'Atari zpet na obrazovce -> proud z C++ zase bezi, obraz presne')
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2],))
        br.close()
    srv.shutdown()
    lg = zaloguj()
    with PZ:
        cely = ''.join(LOG)
    radky = [l for l in cely.split('\n') if 'B303' in l]
    tisk('log appky (B303):\n  ' + '\n  '.join(radky[:30]))
    over(any('TV PROUD: prohlizec' in l and 'pripojen' in l for l in radky) and any('odpojen po' in l for l in radky),
         'log appky: prohlizec pripojen / odpojen (snimku, MB, klicovych)')
    lib.tv_stop()
    tisk('\nVYSLEDEK: %d kontrol, %d chyb' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, 'VYSTUP_test_b303_prohlizec.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
