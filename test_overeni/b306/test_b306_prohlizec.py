#!/usr/bin/env python3
# B306: KONEC-KONCU test prenosu Sega, PS1 a uvodniho dema na TV / PC (cele v C++).
#
#  - stranka prohlizece = PRESNE ta z appky (vytahne se z MainActivity.java),
#  - Chromium (Playwright) BEZ prepinace --autoplay-policy: prohlizec se chova jako
#    Chrome u Reneho (zvuk stranky az po kliknuti / klavese, po F5 znovu),
#  - Python server = to, co v appce dela Java (/status proud=..., /sega.stream,
#    /ps1.stream, /intro.stream, /clientlog); umi pridat zadrhavani site (Wi-Fi),
#  - libsega_b306.so = SKUTECNY nap_sega_native_proof.cpp (tv_sega6.cpp),
#    libps1_b306.so = SKUTECNY nap_ps1_native.cpp (tv_ps1_6.cpp),
#    libintro_b306.so = SKUTECNY nap_intro_gl.cpp (tv_intro.cpp, Mesa bez okna).
#
#   python3 test_b306_prohlizec.py ADRESAR_PNG_320x240              -> VYSTUP_test_b306_prohlizec.txt
#   python3 test_b306_prohlizec.py ADRESAR_PNG_320x240 --srovnani   -> VYSTUP_srovnani_b305_b306.txt
#     (stejna mereni se strankou a C++ z B305 (commit 34c5fa3) a z B306 - zvuk za obrazem,
#      zadrhavajici sit, a Reneho "zvuk naskocil az po case / po F5 zase ne")
import base64
import ctypes
import http.server
import math
import os
import random
import re
import socket
import statistics
import struct
import subprocess
import sys
import threading
import time
import urllib.parse

import numpy as np

os.environ.setdefault('EGL_PLATFORM', 'surfaceless')
ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
SROVNANI = '--srovnani' in sys.argv


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


HTML_B306 = stranka_z_javy(open(os.path.join(KOREN, JAVA_CESTA), encoding='utf-8').read()).encode('utf-8')
B305_COMMIT = os.environ.get('REF', '34c5fa3')     # commit B305 (stranka prohlizece z B305)
HTML_B305 = stranka_z_javy(subprocess.run(['git', 'show', B305_COMMIT + ':' + JAVA_CESTA], cwd=KOREN, capture_output=True,
                                          text=True, check=True).stdout).encode('utf-8') if SROVNANI else b''
HTML = [HTML_B306]


def nacti(cesta, jm):
    lib = ctypes.CDLL(cesta)
    getattr(lib, jm + '_dalsi').argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    getattr(lib, jm + '_log').restype = ctypes.c_char_p
    if hasattr(lib, jm + '_znacka_seq'):
        getattr(lib, jm + '_znacka_seq').restype = ctypes.c_longlong
    if jm != 'intro':
        getattr(lib, jm + '_ocekavany').argtypes = [ctypes.c_uint, ctypes.c_void_p]
    if jm == 'ps1':
        lib.ps1_start.restype = ctypes.c_char_p
        lib.ps1_start.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    if jm == 'intro':
        lib.intro_start.argtypes = [ctypes.c_char_p]
        for f in ('intro_prehrano', 'intro_zacatek', 'intro_snimku'):
            getattr(lib, f).restype = ctypes.c_longlong
        lib.intro_reference.argtypes = [ctypes.c_void_p, ctypes.c_int]
    return lib


LIB = {}
if SROVNANI:
    LIB['sega'] = nacti(os.path.join(ZDE, 'libsega_b305srovnani.so'), 'sega')
    LIB['ps1'] = nacti(os.path.join(ZDE, 'libps1_b305srovnani.so'), 'ps1')
    LIB6 = {'sega': nacti(os.path.join(ZDE, 'libsega_b306.so'), 'sega'), 'ps1': nacti(os.path.join(ZDE, 'libps1_b306.so'), 'ps1')}
else:
    LIB['sega'] = nacti(os.path.join(ZDE, 'libsega_b306.so'), 'sega')
    LIB['ps1'] = nacti(os.path.join(ZDE, 'libps1_b306.so'), 'ps1')
    LIB['intro'] = nacti(os.path.join(ZDE, 'libintro_b306.so'), 'intro')

PZ = threading.Lock()
DOTAZY = {}
CLIENTLOG = []
NA_OBRAZOVCE = ['']            # 'sega' / 'ps1' / 'intro' / ''
ZAVRIT_PROUD = threading.Event()
PROUD_BAJTU = {'sega': 0, 'ps1': 0, 'intro': 0}
ZADRHAVANI = {'on': False, 'max_ms': 25.0, 'stall_kazdych_s': 3.0, 'stall_ms': 150.0, 'posl': 0.0}
PAKETY = {'sega': [], 'ps1': [], 'intro': []}     # (cas, seq, ver, vzorku, index znacky ve zvuku nebo -1)
INTRO_ZVUK = bytearray()                            # vsechen zvuk dema z paketu (int16 stereo)
INTRO_CAS = []                                      # (zvuk v paketu konci v case dema [ms], cas dema v okamziku paketu [ms])
LOG = []


def zaloguj():
    for jm, lib in LIB.items():
        s = getattr(lib, jm + '_log')().decode('utf-8', 'replace')
        if s:
            with PZ:
                LOG.append(s)


def rozeber(jm, p):
    """paket verze 2 / 3: hlavicka, preskocit obraz, zvuk; vraci (seq, ver, vzorku, index znacky, pcm)"""
    ver = p[2]
    seq = struct.unpack_from('<I', p, 8)[0]
    o = 28
    if ver == 2:
        pz = struct.unpack_from('<H', p, o)[0]
        o += 2 + 4 * pz
        dl = struct.unpack_from('<I', p, o)[0]
        o += 4 + dl
    elif ver == 3:
        dl = struct.unpack_from('<I', p, o + 9)[0]
        o += 13 + dl
    else:
        return seq, ver, 0, -1, b''
    an = struct.unpack_from('<H', p, o)[0]
    kan = p[o + 2]
    o += 3
    pcm = p[o:o + an * 2 * kan]
    a = np.frombuffer(pcm, dtype='<i2')
    L = a if kan == 1 else a[0::2]
    idx = np.nonzero(np.abs(L.astype(np.int32)) > 11000)[0]
    if kan == 1:
        pcm = np.repeat(a, 2).astype('<i2').tobytes()
    return seq, ver, an, int(idx[0]) if len(idx) else -1, pcm


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
        kap = 4 * 1024 * 1024
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
                data = b.raw[:n]
                seq, ver, an, zn, pcm = rozeber(jm, data[4:])
                with PZ:
                    PAKETY[jm].append((time.monotonic(), seq, ver, an, zn))
                    if len(PAKETY[jm]) > 20000:
                        del PAKETY[jm][:10000]
                    if jm == 'intro':
                        INTRO_ZVUK.extend(pcm)
                        zac = LIB['intro'].intro_zacatek()
                        if zac >= 0:
                            INTRO_CAS.append((len(INTRO_ZVUK) // 4, LIB['intro'].intro_cas_ms(), LIB['intro'].intro_prehrano()))
                # zadrhavajici sit (Wi-Fi): kolisani + obcas delsi vypadek
                if ZADRHAVANI['on']:
                    t = time.monotonic()
                    if t - ZADRHAVANI['posl'] > ZADRHAVANI['stall_kazdych_s']:
                        ZADRHAVANI['posl'] = t
                        time.sleep(ZADRHAVANI['stall_ms'] / 1000.0)
                    else:
                        time.sleep(random.random() * ZADRHAVANI['max_ms'] / 1000.0)
                t0 = time.monotonic()
                self.wfile.write(data)
                self.wfile.flush()
                zapis = int(min(1000, (time.monotonic() - t0) * 1000))
                PROUD_BAJTU[jm] += n
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
            self.posli(200, 'text/html; charset=utf-8', HTML[0])
        elif u.path == '/status':
            d = NA_OBRAZOVCE[0]
            proud = d if d and d in LIB and (getattr(LIB[d], d + '_stav')() & 1) else '0'
            self.posli(200, 'text/plain', ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=%d seq=0 h264Seq=0\n'
                                           % (proud, 1 if d == 'sega' else 0)).encode())
        elif u.path in ('/sega.stream', '/ps1.stream', '/intro.stream'):
            self.proud(u.path[1:].split('.')[0])
        elif u.path == '/zmen':
            getattr(LIB[g('d')], g('d') + '_ctverec')(int(g('z', '0')))
            self.posli(200, 'text/plain', b'ok')
        elif u.path == '/clientlog':
            with PZ:
                CLIENTLOG.append(g('m'))
            self.posli(200, 'text/plain', b'ok')
        else:
            self.posli(404, 'text/plain', b'nic')


# zaznam zvuku, ktery stranka opravdu zaradi do WebAudio (pred skripty stranky).
# Stav AudioContextu a vyzvy hlasi stranka sama do konzole: Playwright page.evaluate bezi
# jako GESTO uzivatele (userGesture) - kdyby se stav cetl pres evaluate, prohlizec by zvuk
# "odemkl" sam a test autoplay by nic nedokazal.
ZAZNAM_JS = r"""
(function(){
  setInterval(function(){try{var c=window._ctxs&&_ctxs.length?_ctxs[_ctxs.length-1]:null,z=document.getElementById('zv');
    console.log('ZVSTAV '+(c?c.state:'zadny')+' '+(z?getComputedStyle(z).display:'?')+' '+(window._zz?_zz.length:0));}catch(e){}},200);
  window._zz=[];window._zzData=false;window._ctxs=[];
  var C=window.AudioContext||window.webkitAudioContext;
  if(C){var N=function(o){var c=new C(o);window._ctxs.push(c);return c;};N.prototype=C.prototype;window.AudioContext=N;window.webkitAudioContext=N;}
  var orig=AudioBufferSourceNode.prototype.start;
  AudioBufferSourceNode.prototype.start=function(when){
    try{var b=this.buffer,ctx=this.context;
      if(b){var ts=ctx.getOutputTimestamp?ctx.getOutputTimestamp():null,L=b.getChannelData(0),m=-1,i;
        for(i=0;i<L.length;i++){if(L[i]>0.33||L[i]<-0.33){m=i;break;}}
        var r={w:when||0,n:b.length,fs:b.sampleRate,pr:this.playbackRate.value,ct:ctx.currentTime,
               tc:ts?ts.contextTime:0,tp:ts?ts.performanceTime:0,now:performance.now(),m:m};
        if(window._zzData){r.L=Array.prototype.slice.call(L);}
        window._zz.push(r);if(window._zz.length>30000)window._zz.splice(0,15000);}
    }catch(e){}
    return orig.apply(this,arguments);};
})();
"""


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

    png = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('--') else os.path.join(ZDE, 'ps1_320')
    prac = os.path.join(ZDE, 'prac_b306')
    os.makedirs(prac, exist_ok=True)
    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    # ---------------- pomocne funkce nad strankou ----------------
    def canvas(page):
        r = page.evaluate("""(function(){var a=window.napAp;if(!a||!a.cx||!a.W)return null;
            var d=a.cx.getImageData(0,0,a.W,a.H).data,s='';
            for(var i=0;i<d.length;i+=32768){s+=String.fromCharCode.apply(null,d.subarray(i,Math.min(d.length,i+32768)));}
            return {W:a.W,H:a.H,b:btoa(s)};})()""")
        if not r:
            return None
        return r['W'], r['H'], base64.b64decode(r['b'])

    def sega_presne(page, pokusu=8):
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

    def stav_zvuku(page):
        return page.evaluate("""(function(){var c=window._ctxs&&_ctxs.length?_ctxs[_ctxs.length-1]:null,z=document.getElementById('zv');
            return {stav:c?c.state:'zadny',vyzva:z?getComputedStyle(z).display!=='none':null,zv:napAp.zv,zahS:napAp.zahS||0,
            pod:napAp.pod,zah:napAp.zah,zdroju:_zz.length,fs:c?c.sampleRate:0};})()""")

    def av_mereni(page, druh, x, y, kolik=6):
        """ctverec v jadru (obraz) + znacka ve zvuku tehoz kroku jadra: kdy se ctverec objevi
        v canvasu a kdy prohlizec znacku PREHRAJE (cas vystupu zvukove karty) - rozdil = zvuk za obrazem"""
        return page.evaluate("""async function(a){var res=[];
            for(var k=0;k<a.kolik;k++){
              await fetch('/zmen?d='+a.d+'&z=0');await new Promise(function(r){setTimeout(r,450);});
              var od=_zz.length,t0=performance.now(),tv=0;
              await fetch('/zmen?d='+a.d+'&z=1');
              while(performance.now()-t0<2000){var p=napAp.cx.getImageData(a.x,a.y,1,1).data;
                if(p[0]>240&&p[1]>240&&p[2]>240){tv=performance.now();break;}await new Promise(function(r){setTimeout(r,4);});}
              var ta=0,rz=null;var t1=performance.now();
              while(performance.now()-t1<2500&&!ta){for(var i=od;i<_zz.length;i++){var r=_zz[i];if(r.m>=0){
                ta=r.tp+(r.w+r.m/(r.fs*r.pr)-r.tc)*1000;rz=r;break;}}if(!ta)await new Promise(function(r){setTimeout(r,10);});}
              res.push({obraz:tv?tv-t0:-1,zvuk:ta?ta-t0:-1,rozdil:(tv&&ta)?ta-tv:null,
                prisel:rz?rz.now-t0:-1,rezerva:rz?(rz.w-rz.ct)*1000:-1,vnitrek:rz?rz.m/(rz.fs*rz.pr)*1000:-1,
                vystup:rz?(rz.ct-rz.tc)*1000-(rz.now-rz.tp):-1});
            }
            await fetch('/zmen?d='+a.d+'&z=0');return res;}""", {'d': druh, 'x': x, 'y': y, 'kolik': kolik})

    def paket_znacky(jm):
        """C++: je znacka zvuku ve STEJNEM paketu jako prvni snimek se ctvercem? vraci o kolik paketu pozdeji"""
        zs = getattr(LIB[jm], jm + '_znacka_seq')()
        with PZ:
            pk = list(PAKETY[jm])
        # prvni paket se snimkem znacky nebo pozdejsim (rozbocovac posila vzdy posledni snimek -
        # kdyz jadro da dva snimky tesne po sobe, prvni se preskoci; zvuk od minula je v nem cely)
        i0 = next((i for i, p in enumerate(pk) if p[1] >= (zs & 0xFFFFFFFF) and p[1] - (zs & 0xFFFFFFFF) < 1000), None)
        if i0 is None:
            return None, zs
        for j in range(i0, min(len(pk), i0 + 60)):
            if pk[j][4] >= 0:
                return j - i0, zs
        return 99, zs

    def analyza_zvuku(zz, fs_ctx, frekv_max=1000.0):
        """zaznam zdroju (data L) -> navaznost (mezery / prekryvy ve vzorcich) a lupance
        (druha diference nad mez cisteho sinusu), mimo prechody znacky"""
        zz = [r for r in zz if 'L' in r]
        if len(zz) < 10:
            return None
        zz.sort(key=lambda r: r['w'])
        mezer = 0
        useky = []
        akt = []
        for k, r in enumerate(zz):
            zac = r['w'] * r['fs']
            if k and abs(zac - (zz[k - 1]['w'] * zz[k - 1]['fs'] + zz[k - 1]['n'] / zz[k - 1]['pr'])) > 0.01:
                mezer += 1
                useky.append(akt)
                akt = []
            akt.extend(r['L'])
        useky.append(akt)
        lupancu = 0
        vzorku = 0
        max_d2 = 0.0
        for u in useky:
            x = np.array(u, dtype=np.float64)
            if len(x) < 10:
                continue
            d2 = np.abs(x[2:] - 2 * x[1:-1] + x[:-2])
            hlas = np.abs(x) > 0.25                  # znacka (2000 Hz nahlas) a jeji okoli vynechat
            maska = np.convolve(hlas.astype(np.float64), np.ones(200), mode='same') > 0
            d2[maska[1:-1]] = 0
            vzorku += len(x)
            max_d2 = max(max_d2, float(d2.max()) if len(d2) else 0.0)
            lupancu += int(np.sum(d2 > 0.004))
        lb = sorted((r['w'] - r['ct']) * 1000 for r in zz)
        pr = [r['now'] for r in zz]
        mez = sorted(b - a for a, b in zip(pr, pr[1:]))
        return {'zdroju': len(zz), 'mezer': mezer, 'useku': len(useky), 'vzorku': vzorku, 'lupancu': lupancu,
                'max_d2': round(max_d2, 5), 'fs': zz[0]['fs'],
                'rezerva_ms': '%.0f/%.0f/%.0f/%.0f' % (lb[0], lb[len(lb) // 10], lb[len(lb) // 2], lb[-1]),
                'mezi_pakety_ms': '%.0f/%.0f/%.0f' % (mez[len(mez) // 2], mez[len(mez) * 99 // 100], mez[-1]) if mez else '-'}

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        args = []          # B306: SCHVALNE bez --autoplay-policy - jako Chrome u Reneho
        try:
            br = p.chromium.launch(args=args)
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome', args=args)
        ctx = br.new_context(viewport={'width': 1280, 'height': 720})
        ctx.add_init_script(ZAZNAM_JS)
        page = ctx.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))

        if SROVNANI:
            srovnani(page, url, tisk, over, av_mereni, paket_znacky, stav_zvuku, analyza_zvuku, png, prac)
            br.close()
            srv.shutdown()
            fin(tisk, kontrol, chyb, vystup, 'VYSTUP_srovnani_b305_b306.txt')
            return 1 if chyb[0] else 0

        tisk('B306: prohlizec (Chromium BEZ --autoplay-policy, jako Chrome u Reneho) <- server jako Java <- SKUTECNE C++ appky')
        tisk('stranka z MainActivity.java: %d bajtu' % len(HTML_B306))
        over(LIB['sega'].sega_start() == 1, 'Sega: jadro bezi (krok jako vlakno emulace: radky, zvuk FM/PSG, nap_tv_po_kroku)')
        NA_OBRAZOVCE[0] = 'sega'

        # ================= 1) ZVUK A AUTOPLAY =================
        # (zadne page.evaluate / wait_for_function, dokud se nekliklo - to by bylo gesto;
        #  cekani pres wait_for_timeout, at Playwright doruci hlaseni z konzole)
        tisk('\n--- 1) ZVUK: prohlizec ho pusti az po kliknuti (autoplay) ---')
        ZVSTAV = {}

        def sleduj(pg, klic):
            def h(msg):
                t = msg.text
                if t.startswith('ZVSTAV '):
                    c = t.split(' ')
                    ZVSTAV[klic] = {'stav': c[1], 'vyzva': c[2] != 'none', 'zdroju': int(c[3])}
            pg.on('console', h)

        sleduj(page, 'p1')
        page.goto(url)
        page.wait_for_timeout(4000)
        s0 = dict(ZVSTAV.get('p1', {}))
        with PZ:
            cl = list(CLIENTLOG)
            pak0 = len(PAKETY['sega'])
        tisk('po nacteni (bez kliknuti): %s, paketu Segy %d' % (s0, pak0))
        over(s0.get('stav') == 'suspended' and s0.get('vyzva') is True and s0.get('zdroju') == 0 and pak0 > 100,
             'bez kliknuti prohlizec zvuk nepusti (AudioContext suspended) - stranka to vi: VYZVA na obrazovce, obraz bezi, zvuk se nehromadi')
        over(any(m.startswith('B306 ZVUK: prohlizec zvuk nepustil') for m in cl), 'do logu appky: "B306 ZVUK: prohlizec zvuk nepustil (autoplay ...)"')
        page.mouse.click(640, 360)
        page.wait_for_timeout(1000)
        s1 = dict(ZVSTAV.get('p1', {}))
        tisk('po jednom kliknuti: %s' % s1)
        over(s1.get('stav') == 'running' and s1.get('vyzva') is False and s1.get('zdroju', 0) > 20,
             'jedno kliknuti kamkoli -> zvuk HNED hraje (running), vyzva zmizela')
        page.wait_for_timeout(600)
        with PZ:
            cl = list(CLIENTLOG)
        over(any(m.startswith('B306 ZVUK: hraje - prohlizec ho pustil po') for m in cl), 'do logu appky: "B306 ZVUK: hraje - prohlizec ho pustil po X s"')
        # F5: nektery Chrome si kliknuti pamatuje i po obnoveni stranky (stejny server), jiny ne
        # (Rene: "po refresh stranky zase se zvuk odmlcel") - stranka musi zvladnout oboje
        ZVSTAV.pop('p1', None)
        page.reload()
        page.wait_for_timeout(2500)
        s2 = dict(ZVSTAV.get('p1', {}))
        tisk('po F5: %s' % s2)
        if s2.get('stav') == 'running':
            over(s2.get('vyzva') is False and s2.get('zdroju', 0) > 20, 'po F5: tenhle Chromium si kliknuti pamatoval - zvuk hraje hned, vyzva neni')
        else:
            over(s2.get('vyzva') is True, 'po F5 prohlizec zvuk zastavil - VYZVA je zpet (driv ticho bez vysvetleni)')
            page.keyboard.press('KeyA')
            page.wait_for_timeout(1000)
            s2 = dict(ZVSTAV.get('p1', {}))
            over(s2.get('stav') == 'running' and s2.get('vyzva') is False, 'po F5 + klavese zvuk hraje')
        # prohlizec, ktery jeste nikdy nekliknul (TV / PC poprve): klavesa staci
        p2 = br.new_page(viewport={'width': 1280, 'height': 720})      # novy kontext = bez historie kliknuti
        p2.add_init_script(ZAZNAM_JS)
        p2.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        sleduj(p2, 'p2')
        p2.goto(url)
        p2.wait_for_timeout(2500)
        s3a = dict(ZVSTAV.get('p2', {}))
        p2.keyboard.press('KeyA')
        p2.wait_for_timeout(1000)
        s3 = dict(ZVSTAV.get('p2', {}))
        tisk('nove okno prohlizece: pred klavesou %s, po klavese %s' % (s3a, s3))
        over(s3a.get('stav') == 'suspended' and s3a.get('vyzva') is True and s3.get('stav') == 'running' and s3.get('vyzva') is False and s3.get('zdroju', 0) > 20,
             'nove okno: vyzva, pak klavesa (napr. hra z klavesnice PC) zvuk pusti take')
        p2.close()
        time.sleep(0.5)
        try:
            page.wait_for_function("window.napAp&&napAp.on&&napAp.druh==='sega'&&napAp.pak>30", timeout=10000)
        except Exception:
            pass
        s1 = stav_zvuku(page)

        # ================= 2) SEGA =================
        tisk('\n--- 2) SEGA (/sega.stream) ---')
        r = sega_presne(page)
        over(r == 0, 'Sega: obraz v prohlizeci PRESNE snimek jadra (%d ruznych bodu z 71680)' % r)
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah})')
        b0 = PROUD_BAJTU['sega']
        time.sleep(4.0)
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,pod:napAp.pod,zah:napAp.zah,cil:napAp.cil,chyb:napAp.chyb,r0:napAp.rMin,r1:napAp.rMax})')
        b1 = PROUD_BAJTU['sega']
        fps = (a1['pak'] - a0['pak']) / 4.0
        zv = (a1['zv'] - a0['zv']) / 4.0
        tisk('za 4 s: %.1f snimku/s, zvuk %.0f vzorku/s, podteceni %d, zahozeno %d, chyb %d, %.2f Mbit/s'
             % (fps, zv, a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], a1['chyb'], (b1 - b0) * 8 / 4.0 / 1e6))
        over(57 <= fps <= 62 and a1['chyb'] == 0, 'Sega: 60 snimku/s, zadna chyba paketu')
        over(48000 * 0.95 < zv < 48000 * 1.05 and a1['pod'] - a0['pod'] <= 1 and a1['zah'] == a0['zah'],
             'Sega: zvuk 48000 vzorku/s, zadne zahozeni, nejvys 1 podteceni (testovaci stroj ma 2 jadra: Chromium + emulatory + server)')
        # zvuk v paketech = presne smes FM + PSG z jadra (omezovac jako pro reproduktor)
        # A/V v C++: znacka zvuku ve STEJNEM paketu jako prvni snimek se ctvercem
        page.evaluate('window._zzData=true;window._zz=[];')
        av = av_mereni(page, 'sega', 120, 80)
        page.evaluate('window._zzData=false;')
        zz = page.evaluate('_zz')
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        tisk('A/V Sega: %s' % ', '.join('obraz %.0f / zvuk %.0f ms' % (x['obraz'], x['zvuk']) for x in av))
        tisk('  rizeni rezervy: %s' % page.evaluate("({r:napAp.r,mins:napAp.mins.map(function(x){return Math.round(x*1000)}),pod:napAp.pod,zah:napAp.zah,uber:napAp.uber})"))
        tisk('  rozpis (paket se zvukem prisel / rezerva pred nim / znacka v kusu / vystup prohlizece): %s'
             % ', '.join('%.0f/%.0f/%.0f/%.0f' % (x['prisel'], x['rezerva'], x['vnitrek'], x['vystup']) for x in av))
        posun, zs = paket_znacky('sega')
        tisk('C++: prvni snimek se ctvercem = paket %d, znacka zvuku o %s paketu pozdeji' % (zs, posun))
        over(posun == 0, 'Sega C++: zvuk tehoz kroku jadra je ve STEJNEM paketu jako obraz (B305: az za frontou reproduktoru)')
        med = statistics.median(roz) if roz else 999
        tisk('zvuk za obrazem v prohlizeci (vcetne vystupu zvukove karty): median %.0f ms, %s' % (med, ', '.join('%.0f' % x for x in roz)))
        over(len(roz) == 6 and med <= 200, 'Sega: zvuk za obrazem median %.0f ms <= 200 ms (z toho ~35 ms vystup zvuku prohlizece; rezerva se drzi nad nejhorsim zakolisanim poslednich 10 s; B305 se S8 frontou 254-348 ms, viz --srovnani)' % med)
        an = analyza_zvuku(zz, 0)
        tisk('zvuk v prohlizeci: %s' % an)
        over(an and an['lupancu'] <= an['mezer'] <= 1 and an['fs'] == s1['fs'],
             'Sega: kusy zvuku navazuji vzorek na vzorek, zadny lupanec (mezer %s, lupancu %s; prevzorkovano 48000 -> %d Hz)' % (an and an['mezer'], an and an['lupancu'], s1['fs']))
        lat = [x['obraz'] for x in av if x['obraz'] > 0]
        tisk('zpozdeni zmena v jadru -> canvas: %s ms' % ', '.join('%.0f' % x for x in lat))
        over(len(lat) == 6 and statistics.median(lat) <= 50, 'Sega: zmena v jadru je v obraze do 1-2 snimku (median %.0f ms)' % statistics.median(lat))

        # ================= 3) ZADRHAVAJICI SIT =================
        tisk('\n--- 3) SEGA pres zadrhavajici sit (kolisani 0-25 ms, kazde 3 s vypadek 150 ms) ---')
        a0 = page.evaluate('({pod:napAp.pod,zah:napAp.zah,zv:napAp.zv})')
        ZADRHAVANI['on'] = True
        ZADRHAVANI['posl'] = time.monotonic()
        time.sleep(4.0)
        page.evaluate('window._zzData=true;window._zz=[];')
        av = av_mereni(page, 'sega', 120, 80, kolik=8)
        page.evaluate('window._zzData=false;')
        zz = page.evaluate('_zz')
        ZADRHAVANI['on'] = False
        a1 = page.evaluate('({pod:napAp.pod,zah:napAp.zah,zv:napAp.zv,lsum:napAp.lsum,lcnt:napAp.lcnt,cil:napAp.cil})')
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        an = analyza_zvuku(zz, 0)
        tisk('zadrhavani: podteceni %d, zahozeno %d, zvuk za obrazem median %.0f ms (%s), zvuk: %s'
             % (a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], statistics.median(roz) if roz else -1,
                ', '.join('%.0f' % x for x in roz), an))
        over(an and an['lupancu'] <= 2 * (an['mezer'] + 1) and a1['pod'] - a0['pod'] <= 6,
             'Sega pres zadrhavajici sit: zvuk se srovna sam (podteceni %d za ~20 s, lupance jen u nich)' % (a1['pod'] - a0['pod']))
        over(roz and statistics.median(roz) <= 220, 'Sega pres zadrhavajici sit: zvuk za obrazem median %.0f ms (rezerva se prizpusobi siti)' % (statistics.median(roz) if roz else -1))
        time.sleep(1.0)

        # ================= 4) PS1 =================
        tisk('\n--- 4) PS1 (/ps1.stream) ---')
        LIB['sega'].sega_stop()          # jako v telefonu: bezi vzdy jen jedno jadro (testovaci stroj ma jen 2 jadra)
        odp = LIB['ps1'].ps1_start(png.encode(), prac.encode()).decode()
        over(odp == 'PS1_BIOS_OK', 'PS1: skutecny start pres JNI (vlakno emulace, OpenSL zvuk) - %s' % odp)
        time.sleep(0.5)
        NA_OBRAZOVCE[0] = 'ps1'
        try:
            page.wait_for_function("napAp.on&&napAp.druh==='ps1'&&napAp.jpg>10", timeout=8000)
            prep = True
        except Exception:
            prep = False
        st = page.evaluate("({W:napAp.W,H:napAp.H,ver:napAp.ver,druh:napAp.druh})")
        over(prep and st['W'] == 320 and st['H'] == 240 and st['ver'] == 3, 'Sega -> PS1: stranka sama prepnula proud (bez obnoveni stranky) %s' % st)
        # prubeh rezervy zvuku po prepnuti (kazdych 0,5 s): cas ms, rezerva ms, minimum posledni vteriny, rychlost, k vynechani ms, podteceni, zahozeno
        page.evaluate("""window._prubeh=[];var t0=performance.now();window._prubehT=setInterval(function(){var c=_ctxs[_ctxs.length-1];
            _prubeh.push([Math.round(performance.now()-t0),Math.round((napAp.nF/c.sampleRate-c.currentTime)*1000),
            napAp.mins.length?Math.round(napAp.mins[napAp.mins.length-1]*1000):null,Math.round(napAp.r*10000)/10000,
            Math.round((napAp.uber||0)*1000),napAp.pod,napAp.zah]);},500);""")
        time.sleep(1.0)
        ps = ps1_psnr(page)
        over(ps >= 30, 'PS1: obraz odpovida snimku jadra (PSNR %.1f dB)' % ps)
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod,zah:napAp.zah})')
        time.sleep(4.0)
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod,zah:napAp.zah,chyb:napAp.chyb})')
        fps = (a1['pak'] - a0['pak']) / 4.0
        jps = (a1['jpg'] - a0['jpg']) / 4.0
        zv = (a1['zv'] - a0['zv']) / 4.0
        tisk('za 4 s: %.1f paketu/s, %.1f JPEG/s, zvuk %.0f vzorku/s, podteceni %d, zahozeno %d'
             % (fps, jps, zv, a1['pod'] - a0['pod'], a1['zah'] - a0['zah']))
        over(fps >= 40 and 27 <= jps <= 33 and a1['chyb'] == 0, 'PS1: hra na 30 snimku/s = 30 obrazku/s, zadna chyba')
        over(44100 * 0.95 < zv < 44100 * 1.05 and a1['pod'] - a0['pod'] <= 1 and a1['zah'] == a0['zah'],
             'PS1: zvuk 44100 vzorku/s (uz pri vyrobe v retro_run), zadne zahozeni, nejvys 1 podteceni')
        # rezerva zvuku se po startu proudu dorovnava ~10-15 s (pamatuje si 10 s nejhorsiho prichodu) - merit ustaleny stav
        time.sleep(8.0)
        page.evaluate('window._zzData=true;window._zz=[];')
        av = av_mereni(page, 'ps1', 130, 90)
        page.evaluate('window._zzData=false;')
        zz = page.evaluate('_zz')
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        tisk('A/V PS1: %s' % ', '.join('obraz %.0f / zvuk %.0f ms' % (x['obraz'], x['zvuk']) for x in av))
        pr = page.evaluate('clearInterval(window._prubehT),_prubeh')
        tisk('  prubeh rezervy od prepnuti [ms, rezerva, min/s, rychlost, vynechat, podteceni, zahozeno]: %s' % pr[:40])
        tisk('  rizeni rezervy: %s' % page.evaluate("({r:napAp.r,mins:napAp.mins.map(function(x){return Math.round(x*1000)}),pod:napAp.pod,zah:napAp.zah,uber:napAp.uber})"))
        tisk('  rozpis (paket se zvukem prisel / rezerva pred nim / znacka v kusu / vystup prohlizece): %s'
             % ', '.join('%.0f/%.0f/%.0f/%.0f' % (x['prisel'], x['rezerva'], x['vnitrek'], x['vystup']) for x in av))
        posun, zs = paket_znacky('ps1')
        tisk('C++: prvni snimek se ctvercem = paket %d, znacka zvuku o %s paketu pozdeji' % (zs, posun))
        over(posun == 0, 'PS1 C++: zvuk z retro_run je ve STEJNEM paketu jako jeho snimek (B305: az z OpenSL callbacku)')
        med = statistics.median(roz) if roz else 999
        tisk('zvuk za obrazem v prohlizeci: median %.0f ms, %s' % (med, ', '.join('%.0f' % x for x in roz)))
        over(len(roz) == 6 and med <= 200, 'PS1: zvuk za obrazem median %.0f ms <= 200 ms (B305 236-257 ms, viz --srovnani)' % med)
        an = analyza_zvuku(zz, 0)
        tisk('zvuk v prohlizeci: %s' % an)
        over(an and an['lupancu'] <= an['mezer'] <= 1, 'PS1: kusy zvuku navazuji vzorek na vzorek, zadny lupanec (mezer %s, lupancu %s)' % (an and an['mezer'], an and an['lupancu']))
        # spadle spojeni
        with PZ:
            pr0 = DOTAZY.get('/ps1.stream', 0)
        ZAVRIT_PROUD.set()
        time.sleep(2.5)
        with PZ:
            pr1 = DOTAZY.get('/ps1.stream', 0)
        ps = ps1_psnr(page)
        over(pr1 == pr0 + 1 and ps >= 30, 'PS1: spadle spojeni - prohlizec se pripojil znovu sam (PSNR %.1f dB)' % ps)

        # ================= 5) UVODNI DEMO =================
        tisk('\n--- 5) UVODNI DEMO (/intro.stream, C++: OpenSL ES + proud) ---')
        LIB['ps1'].ps1_stop()            # demo bezi v telefonu samo
        nat = LIB['intro'].intro_start(os.path.join(KOREN, 'app/src/main/assets/').encode())
        over(nat == 1, 'demo: zvuk prehrava C++ (OpenSL ES) - Java AudioTrack jen zaloha')
        t0 = time.monotonic()
        while not LIB['intro'].intro_pripraveno() and time.monotonic() - t0 < 20:
            time.sleep(0.05)
        with PZ:
            INTRO_ZVUK.clear()
            INTRO_CAS.clear()
        NA_OBRAZOVCE[0] = 'intro'
        try:
            page.wait_for_function("napAp.on&&napAp.druh==='intro'&&napAp.W===854", timeout=12000)
            prep = True
        except Exception:
            prep = False
        st = page.evaluate("({W:napAp.W,H:napAp.H,ver:napAp.ver,druh:napAp.druh,px:napAp.px,py:napAp.py,ow:napAp.ow,oh:napAp.oh})")
        over(prep and st['W'] == 854 and st['H'] == 480 and st['ver'] == 3 and st['px'] == 16 and st['py'] == 9 and st['ow'] == 854,
             'PS1 -> demo: stranka sama prepnula na /intro.stream - 854x480 16:9 bez orezu %s' % st)
        a0 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod})')
        time.sleep(6.0)
        a1 = page.evaluate('({pak:napAp.pak,zv:napAp.zv,jpg:napAp.jpg,pod:napAp.pod,chyb:napAp.chyb})')
        jps = (a1['jpg'] - a0['jpg']) / 6.0
        zv = (a1['zv'] - a0['zv']) / 6.0
        c = canvas(page)
        jas = 0
        if c:
            d = np.frombuffer(c[2], dtype=np.uint8).reshape(-1, 4)
            jas = float(d[:, :3].mean())
        tisk('demo za 6 s: %.1f obrazku/s (Mesa bez GPU kresli pomalu), zvuk %.0f vzorku/s, podteceni %d, jas canvasu %.1f, cas dema %d ms'
             % (jps, zv, a1['pod'] - a0['pod'], jas, LIB['intro'].intro_cas_ms()))
        over(jps >= 5 and a1['chyb'] == 0 and jas > 3, 'demo: obraz z C++ chodi do prohlizece (JPEG 854x480)')
        over(44100 * 0.93 < zv < 44100 * 1.05, 'demo: zvuk 44100 vzorku/s jde proudem i kdyz obraz kresli pomalu (pakety jen se zvukem)')
        # zvuk v paketech = presne stopa dema (to, co "reproduktor" prave dohral)
        ref = (ctypes.c_int16 * (2 * 44100 * 50))()
        nref = LIB['intro'].intro_reference(ref, 44100 * 50)
        refa = np.frombuffer(ref, dtype=np.int16)[:nref * 2].reshape(-1, 2)
        with PZ:
            pr = np.frombuffer(bytes(INTRO_ZVUK), dtype='<i2').reshape(-1, 2)
            cas = list(INTRO_CAS)
        zac = LIB['intro'].intro_zacatek()
        shoda_ok = False
        info = 'nic'
        nz = np.nonzero(np.abs(pr[:, 0].astype(np.int32)) > 50)[0]
        if len(nz) and len(pr) > 44100:
            i0 = int(nz[0])
            vzor = pr[i0:i0 + 256, 0]
            kand = np.nonzero(refa[:, 0] == vzor[0])[0]
            for k in kand:
                if k + 256 <= len(refa) and np.array_equal(refa[k:k + 256, 0], vzor):
                    off = int(k) - i0          # index v ref = index v paketech + off
                    a = max(0, -off)
                    b = min(len(pr), len(refa) - off)
                    ruzne = int(np.sum(np.any(pr[a:b] != refa[a + off:b + off], axis=1))) if b > a else -1
                    info = 'porovnano %d ramcu (%.1f s), rozdilnych %d' % (b - a, (b - a) / 44100.0, ruzne)
                    shoda_ok = ruzne == 0 and (b - a) > 44100 * 3
                    break
        tisk('zvuk dema v paketech proti stope (bit po bitu): %s' % info)
        over(shoda_ok, 'demo: zvuk na TV = PRESNE stopa dema, vzorek po vzorku bez mezer')
        # zvuk v paketu konci tam, kde je obraz (cas dema se ridi tim, co reproduktor prehral)
        rozd = []
        if len(nz) and shoda_ok:
            for kon, cms, preh in cas:
                ref_idx = kon + off        # index ve stope, kde konci zvuk paketu
                rozd.append(ref_idx / 44.1 - cms)
        if rozd:
            tisk('zvuk paketu konci proti casu dema (obraz): median %.0f ms, min %.0f, max %.0f (%d paketu)'
                 % (statistics.median(rozd), min(rozd), max(rozd), len(rozd)))
        over(rozd and abs(statistics.median(rozd)) <= 60, 'demo: v paketu je zvuk prave prehraneho okamziku dema (obraz a zvuk spolu)')
        LIB['intro'].intro_stop()
        over(LIB['intro'].intro_hracu() == 101 and LIB['intro'].intro_okno_refs() == 1, 'demo konec: prehravac OpenSL zniceny, okno uvolnene')

        # ================= 6) ZPET POSTARU =================
        NA_OBRAZOVCE[0] = ''
        try:
            page.wait_for_function("!napAp.chce&&document.getElementById('v').getAttribute('src')", timeout=6000)
            zpet = True
        except Exception:
            zpet = False
        over(zpet, 'demo pryc z obrazovky -> proud konci, stranka zpet postaru (MJPEG / JPEG)')
        t0 = time.monotonic()
        while time.monotonic() - t0 < 33:
            with PZ:
                if any(m.startswith('B306 PROHLIZEC') for m in CLIENTLOG):
                    break
            time.sleep(0.5)
        NA_OBRAZOVCE[0] = 'sega'
        time.sleep(3.0)
        with PZ:
            cl = list(CLIENTLOG)
        tisk('z prohlizece do logu appky: %s' % [m for m in cl if m.startswith('B306')][:12])
        over(any(m.startswith('B306 PROHLIZEC') and 'zvuk za obrazem' in m and 'rychlost' in m for m in cl),
             'prohlizec hlasi do logu appky kazdych 30 s i zvuk: napred, zvuk za obrazem, rychlost, vzorkovani')
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2],))
        br.close()
    srv.shutdown()
    time.sleep(0.5)
    zaloguj()
    with PZ:
        cely = ''.join(LOG)
    radky = [l for l in cely.split('\n') if 'B306' in l]
    tisk('log appky (B306):\n  ' + '\n  '.join(radky[:24]))
    over(any('TV PROUD INTRO: prohlizec' in l and 'odpojen po' in l and 'jen se zvukem' in l for l in radky)
         and any('TV PROUD SEGA: prohlizec' in l for l in radky), 'log appky: Sega, PS1 i demo - pripojen / odpojen / statistika')
    LIB['ps1'].ps1_stop()
    LIB['sega'].sega_stop()
    fin(tisk, kontrol, chyb, vystup, 'VYSTUP_test_b306_prohlizec.txt')
    return 1 if chyb[0] else 0


def fin(tisk, kontrol, chyb, vystup, soubor):
    tisk('\nVYSLEDEK: %d kontrol, %d chyb' % (kontrol[0], chyb[0]))
    with open(os.path.join(ZDE, soubor), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')


def srovnani(page, url, tisk, over, av_mereni, paket_znacky, stav_zvuku, analyza_zvuku, png, prac):
    """Stejna mereni s B305 (commit 34c5fa3: stranka + C++) a s B306. Az do kliknuti zadne page.evaluate
    (Playwright evaluate = gesto uzivatele) - stav zvuku hlasi stranka do konzole."""
    global LIB
    vysl = {}
    ZV = {}

    def h(msg):
        t = msg.text
        if t.startswith('ZVSTAV '):
            c = t.split(' ')
            ZV['p'] = {'stav': c[1], 'vyzva': c[2] != 'none' if c[2] != '?' else None, 'zdroju': int(c[3])}
    page.on('console', h)
    for verze in ('B305', 'B306'):
        if verze == 'B306':
            for jm in ('sega', 'ps1'):
                getattr(LIB[jm], jm + '_stop')()
            LIB = dict(LIB6)
        HTML[0] = HTML_B305 if verze == 'B305' else HTML_B306
        tisk('\n=============== %s (stranka i C++ z %s) ===============' % (verze, 'B305 ' + B305_COMMIT if verze == 'B305' else 'B306'))
        LIB['sega'].sega_start()
        LIB['sega'].sega_s8(1)          # fronta reproduktoru telefonu jako na S8 (~6000 vzorku)
        NA_OBRAZOVCE[0] = 'sega'
        with PZ:
            CLIENTLOG.clear()
            for k in PAKETY:
                PAKETY[k].clear()
        ZV.clear()
        page.goto(url)
        page.wait_for_timeout(3000)
        s0 = dict(ZV.get('p', {}))
        tisk('%s: po nacteni bez kliknuti: %s' % (verze, s0))
        # Reneho situace: stranka po nacteni, jedno kliknuti (napr. na celou obrazovku)
        page.mouse.click(640, 360)
        page.wait_for_timeout(4000)
        s = dict(ZV.get('p', {}))
        tisk('%s: 4 s po kliknuti do stranky: %s' % (verze, s))
        if verze == 'B305':
            # B305 se o zvuk po nacteni uz nepokusi - jestli po kliknuti zacne hrat, je nahoda (kdyz se
            # tim kliknutim nebo Wi-Fi zadrhne prijem paketu, stranka zvuk znovu zaradi a Chrome ho pusti)
            tisk('B305: %s' % ('po kliknuti zvuk HRAJE - tentokrat nahodou (kliknuti = cela obrazovka = chvilkove zadrhnuti)'
                               if s.get('stav') == 'running' else 'po kliknuti zvuk NEHRAJE (Reneho log "zvuk NEHRAJE")'))
            # Wi-Fi zadrhne -> polstar naroste -> stranka znovu zaradi zvuk -> Chrome ho (po kliknuti) pusti
            ZADRHAVANI.update({'on': True, 'max_ms': 0.0, 'stall_kazdych_s': 1.5, 'stall_ms': 200.0, 'posl': time.monotonic()})
            page.wait_for_timeout(4000)
            ZADRHAVANI['on'] = False
            s = dict(ZV.get('p', {}))
            tisk('B305: po zadrhnuti site (200 ms): %s' % s)
            page.reload()
            page.wait_for_timeout(3000)
            page.mouse.click(640, 360)
            page.wait_for_timeout(3000)
            s1 = dict(ZV.get('p', {}))
            tisk('B305: po F5 a kliknuti: %s' % s1)
            ZADRHAVANI.update({'on': True, 'max_ms': 0.0, 'stall_kazdych_s': 1.5, 'stall_ms': 200.0, 'posl': time.monotonic()})
            page.wait_for_timeout(4000)
            ZADRHAVANI['on'] = False
            s2 = dict(ZV.get('p', {}))
            tisk('B305: po zadrhnuti site: %s' % s2)
            over(s2.get('stav') == 'running', 'B305: zvuk naskocil az po zadrhnuti site - "zvuk nabehl v sega az po urcite dobe"')
        else:
            over(s0.get('stav') == 'suspended' and s0.get('vyzva') is True and s.get('stav') == 'running' and s.get('vyzva') is False,
                 'B306: vyzva, jedno kliknuti - zvuk hraje hned')
        page.wait_for_timeout(10000)        # ustaleny stav (B306 si rezervu dorovna)
        fr = LIB['sega'].sega_fronta()
        tisk('%s: fronta reproduktoru telefonu (jako S8): %d vzorku = %.0f ms' % (verze, fr, fr / 48.0))
        page.evaluate('window._zzData=true;window._zz=[];')
        av = av_mereni(page, 'sega', 120, 80)
        page.evaluate('window._zzData=false;')
        zz = page.evaluate('_zz')
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        posun, zs = paket_znacky('sega')
        an = analyza_zvuku(zz, 0)
        med = statistics.median(roz) if roz else -1
        tisk('%s Sega: znacka zvuku o %s paketu za obrazem (C++), v prohlizeci zvuk za obrazem median %.0f ms (%s); zvuk %s'
             % (verze, posun, med, ', '.join('%.0f' % x for x in roz), an))
        vysl[verze + '_sega'] = med
        vysl[verze + '_sega_pak'] = posun
        # zadrhavajici sit
        a0 = page.evaluate('({pod:napAp.pod,zah:napAp.zah})')
        ZADRHAVANI.update({'on': True, 'max_ms': 25.0, 'stall_kazdych_s': 3.0, 'stall_ms': 150.0, 'posl': time.monotonic()})
        page.wait_for_timeout(4000)
        page.evaluate('window._zzData=true;window._zz=[];')
        av = av_mereni(page, 'sega', 120, 80, kolik=8)
        page.evaluate('window._zzData=false;')
        zz = page.evaluate('_zz')
        ZADRHAVANI['on'] = False
        a1 = page.evaluate('({pod:napAp.pod,zah:napAp.zah})')
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        an = analyza_zvuku(zz, 0)
        med = statistics.median(roz) if roz else -1
        tisk('%s Sega + zadrhavajici sit: podteceni %d, zahozeno %d, zvuk za obrazem median %.0f ms; zvuk %s'
             % (verze, a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], med, an))
        vysl[verze + '_jitter'] = (round(med), a1['pod'] - a0['pod'], a1['zah'] - a0['zah'], an['lupancu'] if an else -1)
        # PS1 (Sega zastavit - v telefonu bezi vzdy jen jedno jadro)
        LIB['sega'].sega_stop()
        LIB['ps1'].ps1_start(png.encode(), prac.encode())
        page.wait_for_timeout(500)
        NA_OBRAZOVCE[0] = 'ps1'
        page.wait_for_function("napAp.on&&napAp.druh==='ps1'&&napAp.W===320", timeout=10000)
        page.wait_for_timeout(10000)
        av = av_mereni(page, 'ps1', 130, 90)
        roz = [x['rozdil'] for x in av if x['rozdil'] is not None]
        posun, zs = paket_znacky('ps1')
        med = statistics.median(roz) if roz else -1
        tisk('%s PS1: znacka zvuku o %s paketu za obrazem (C++), v prohlizeci zvuk za obrazem median %.0f ms (%s)'
             % (verze, posun, med, ', '.join('%.0f' % x for x in roz)))
        vysl[verze + '_ps1'] = med
        vysl[verze + '_ps1_pak'] = posun
        NA_OBRAZOVCE[0] = ''
        page.wait_for_timeout(1000)
        LIB['ps1'].ps1_stop()
    tisk('\n=============== SROVNANI ===============')
    tisk('zvuk za obrazem (Sega, S8 fronta): B305 %.0f ms -> B306 %.0f ms' % (vysl['B305_sega'], vysl['B306_sega']))
    tisk('zvuk za obrazem (PS1):            B305 %.0f ms -> B306 %.0f ms' % (vysl['B305_ps1'], vysl['B306_ps1']))
    tisk('zadrhavajici sit (Sega): B305 %s -> B306 %s  (median ms, podteceni, zahozeno, lupance)' % (vysl['B305_jitter'], vysl['B306_jitter']))
    over(vysl['B306_sega'] < vysl['B305_sega'] - 100 and vysl['B306_ps1'] < vysl['B305_ps1'] - 40,
         'B306 ma zvuk vyrazne blize obrazu nez B305 (Sega i PS1)')
    over(vysl['B306_sega_pak'] == 0 and vysl['B306_ps1_pak'] == 0 and (vysl['B305_sega_pak'] or 0) >= 4,
         'C++: B306 posila zvuk se snimkem tehoz okamziku, B305 o %s (Sega) / %s (PS1) paketu pozdeji' % (vysl['B305_sega_pak'], vysl['B305_ps1_pak']))
    LIB['sega'].sega_stop()


if __name__ == '__main__':
    sys.exit(main())
