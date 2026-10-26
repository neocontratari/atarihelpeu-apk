#!/usr/bin/env python3
"""B315: ZVUK NA TV / PC HRAJE I PO PRVNI HRE.

Rene (log B314, uz od B313): "Po spusteni LAN streamu bezel zvuk pouze jednou v Atari - pak uz ne -
ani v Atari, ani v Sega, ani v PS1. Na mobilu zvuk funguje, na TV nebo PC uz ne."

Stranka WEB TV ma JEDEN zesilovac (GainNode g), pres ktery jde zvuk obou cest:
  - proud z C++ (/atari.stream, /sega.stream, /ps1.stream - apZvuk),
  - stara cesta /audio.raw (pollAudio - MP3 prehravac, YouTube a mezery mezi proudy).
Stara cesta pri kazdem navazani (cutover) ztlumi g na 0 a naplanuje navrat na 1 za AVD
("zpozdeni zvuku", klavesy sipka nahoru / dolu, ulozene v prohlizeci). Kdyz je AVD 0
(sipka dolu 6x - od B313 jsou sipky v menu na TV listovani kartami, na TV je posila
i ovladac), navrat na 1 se naplanuje PRED ztlumenim -> g zustane na 0 NAPORAD a s nim
cely zvuk stranky. Poprve stara cesta navaze az v mezere po prvni hre (Atari bezi bez
proudu -> Java plni /audio.raw) - proto hral jen prvni Atari.

Test: stranka PRESNE z appky (vytahne se z MainActivity.java) v Chromiu, Python server
dela, co Java (/status, proudy menu / atari / sega / ps1 s 409 + X-Nap-Proud, /audio.raw
s kruhem plnenym jen kdyz proud nema prohlizece, rate -> seq 0), postup jako v logu Reneho:
menu, kliknuti, Atari, NET_HRY (proud 0), Atari znovu, menu, Sega, menu, PS1, MP3 (stara cesta).
Mereni: co OPRAVDU prijde do reproduktoru (AnalyserNode zapojeny pred ac.destination).
Scenare: vychozi zpozdeni, 6x sipka dolu v menu (AVD 0), ulozene AVD 0, AVD 2 s (nejvic).
Stejny postup i se strankou B314 (git 8629955) - musi ukazat Reneho chybu (jinak je rozbor spatne).

Pouziti: python3 test_b315_zvuk_tv.py   -> VYSTUP_test_b315_zvuk_tv.txt
"""
import http.server
import math
import os
import re
import struct
import subprocess
import sys
import threading
import time
import urllib.parse
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
B314_COMMIT = '8629955'
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
    """HTML prohlizece presne tak, jak ho sklada MainActivity.napTvWebWriteHtml (jako test B313)."""
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


# ---------------- ton: 1 s predpocitane (cela cisla Hz -> navazuje bez lupnuti) ----------------
class Ton:
    def __init__(self, f, rate, amp=0.3):
        self.rate = rate
        a = amp * 32767
        self.data = b''.join(struct.pack('<hh', int(a * math.sin(2 * math.pi * f * i / rate)),
                                         int(a * math.sin(2 * math.pi * f * i / rate))) for i in range(rate))
        self.pos = 0

    def kus(self, n):
        out = bytearray()
        while n > 0:
            k = min(n, self.rate - self.pos)
            out += self.data[self.pos * 4:(self.pos + k) * 4]
            self.pos = (self.pos + k) % self.rate
            n -= k
        return bytes(out)


# ---------------- pakety proudu z C++ (format jako apPaket1 / apPaket23 stranky) ----------------
def paket1(pcm, n):
    """Atari 130XE (verze 1): obraz 336x240 bez zmen, zvuk 44100 Hz stereo"""
    p = b'NA' + bytes([1, 0]) + struct.pack('<HH', 336, 240) + b'\0' * 4
    p += struct.pack('<HH', 0, 0) + struct.pack('<H', 0)
    return p + struct.pack('<HB', n, 2) + pcm


def paket23(ver, rate, W, H, pcm, n):
    """Sega (verze 2: paleta + LZ), menu a PS1 (verze 3: JPEG po castech) - bez zmeny obrazu, se zvukem"""
    p = b'NA' + bytes([ver, 0]) + struct.pack('<HH', W, H) + b'\0' * 4
    p += struct.pack('<HHHH', 0, 0, W, H) + bytes([4, 3, 0, 0]) + struct.pack('<I', rate)
    if ver == 2:
        p += struct.pack('<HI', 0, 0)
    else:
        p += struct.pack('<HHHHBI', 0, 0, 0, 0, 0, 0)
    return p + struct.pack('<HB', n, 2) + pcm


PROUDY = {   # druh: (verze, Hz, snimku/s, sirka, vyska, ton Hz) ; menu je tiche jako u Reneho ("0->48000 Hz")
    'menu': (3, 44100, 25, 1280, 720, 0),
    'atari': (1, 44100, 50, 336, 240, 440),
    'sega': (2, 48000, 60, 320, 224, 660),
    'ps1': (3, 44100, 60, 640, 480, 550),
}
# /audio.raw ma JINY ton nez proud tehoz jadra (Atari 880 / 440 Hz): pri prechodu stare cesty na
# proud jde zmerit, jak dlouho stara cesta hraje pres proud (ozvena)
RING_TON = {'atari': (880, 44100), 'sega': (990, 48000), 'ps1': (1100, 44100), 'mp3': (330, 44100)}

# zaznam toho, co opravdu jde do reproduktoru (pred skripty stranky): RMS (okno 1024 vzorku) a spektrum
# (4096 vzorku, bez vyhlazovani) - uroven tonu proudu Atari 440 Hz a stare cesty Atari 880 Hz [dB]
MERENI_JS = r"""
(function(){
  window._ctxs=[];window._rms=[];window._gains=[];
  var C=window.AudioContext||window.webkitAudioContext;
  if(C){var N=function(o){var c=new C(o);window._ctxs.push(c);return c;};N.prototype=C.prototype;window.AudioContext=N;window.webkitAudioContext=N;}
  var cg=BaseAudioContext.prototype.createGain;
  BaseAudioContext.prototype.createGain=function(){var x=cg.apply(this,arguments);window._gains.push(x);return x;};
  var oc=AudioNode.prototype.connect;
  AudioNode.prototype.connect=function(d){
    try{if(d&&(d instanceof AudioDestinationNode)){var c=this.context;if(!c._mer){c._mer=c.createAnalyser();c._mer.fftSize=1024;
      c._spe=c.createAnalyser();c._spe.fftSize=4096;c._spe.smoothingTimeConstant=0;}oc.call(this,c._mer);oc.call(this,c._spe);}}catch(e){}
    return oc.apply(this,arguments);};
  var b=new Float32Array(1024),f=new Float32Array(2048);
  function pas(c,hz){var k=Math.round(hz*4096/c.sampleRate),m=-1e9;for(var i=k-2;i<=k+2;i++)if(f[i]>m)m=f[i];return Math.round(m);}
  setInterval(function(){try{var c=window._ctxs[window._ctxs.length-1];if(!c||!c._mer)return;c._mer.getFloatTimeDomainData(b);
    var s=0;for(var i=0;i<b.length;i++)s+=b[i]*b[i];c._spe.getFloatFrequencyData(f);
    window._rms.push([Date.now(),Math.sqrt(s/b.length),c.state,window._gains.map(function(g){return Math.round(g.gain.value*1000)/1000;}).join('/'),pas(c,440),pas(c,880)]);
    if(window._rms.length>40000)window._rms.splice(0,20000);}catch(e){}},25);
})();
"""
SPEKTRUM_HRAJE = -50.0   # dB v pasmu tonu (ton 0,3 ~ -16 dB, ticho pod -100 dB)

# postup jako v logu Reneho (B314): cas od nacteni stranky [s] -> co je na obrazovce telefonu.
# Mezery "nic" trvaji 4 s: stranka po 2,5 s bez proudu prejde na starou cestu (apStop, H.264
# a /audio.raw) - u Reneho 17:32:03.764 "ATARI PROUD konec" a za 1,4 s "ATARI PROUD start".
# Kdyz pak jadro zase bezi a proud jeste nema prohlizece, Java plni /audio.raw (ATARI_CPP,
# SEGA ...) a stranka ho prehraje = navazani stare cesty (cutover).
POSTUP = [
    (0.0, 'menu'),     # stranka nactena, menu z C++ (tiche)
    (5.0, 'atari'),    # MENU_SPUSTIT atari - prvni Atari (u Reneho hralo)
    (12.0, 'nic'),     # NET_HRY: atari_xex_bridge.html - pristroj vypnut, proud=0
    (16.0, 'atari'),   # hra stazena (De Re Pac-Man) - Atari znovu (u Reneho ticho); v 30. s hlaseni PROHLIZEC
    (33.0, 'menu'),
    (35.0, 'nic'),     # stranka Segy se nacita (u Reneho 20 s H.264)
    (39.0, 'sega'),    # Sonic (u Reneho ticho)
    (47.0, 'menu'),
    (50.0, 'ps1'),     # PS1 rovnou z menu (u Reneho ticho); v 60. s hlaseni PROHLIZEC
    (63.0, 'mp3'),     # MP3 prehravac: stara cesta /audio.raw (proud=0)
    (74.0, 'konec'),
]
OKNA = [   # (nazev, od, do) - kde ma zvuk hrat (po rozjezdu)
    ('Atari poprve', 8.0, 12.0),
    ('Atari podruhe (po NET_HRY)', 21.0, 33.0),
    ('Sega', 43.0, 47.0),
    ('PS1', 54.0, 63.0),
    ('MP3 (stara cesta /audio.raw)', 70.5, 74.0),   # stara cesta az po 2,5 s bez proudu (+ zpozdeni AVD, az 2 s)
]
PRIPOJENI = 0.3   # Java: "PROUD start" -> "prohlizec N pripojen" ~300 ms (log Reneho), do te doby plni /audio.raw
STATUS_ZPOZDENI = 0.7   # /status po zmene jeste 0,7 s rika, co bylo (viz /status ve scenar)
OZ_B314 = [None]  # ozvena stranky B314 pri prechodu stare cesty na proud [s]
PRAH = 0.05   # RMS: ton 0.3 = 0.21, ticho 0


def scenar(html, nazev, avd_ulozene=None, sipky_v_menu=False):
    T = {'0': None}
    STAV = {'faze': 'menu', 'konec': False, 'predtim': 'menu', 'zmena': 0.0}
    KLIENTU = {'menu': 0, 'atari': 0, 'sega': 0, 'ps1': 0}
    RING = {'buf': bytearray(1024 * 1024), 'seq': 0, 'rate': 44100, 'last': 0.0}
    rlock = threading.Lock()
    LOG, DOTAZY = [], []

    def proud_ted():
        f = STAV['faze']
        return f if f in PROUDY else '0'

    def ring_push(pcm, rate):
        with rlock:
            if rate != RING['rate']:      # jako Java napTvWebAudioPush: jina frekvence -> sekvence od 0
                RING['rate'] = rate
                RING['seq'] = 0
            cap = len(RING['buf'])
            s = RING['seq']
            for i in range(0, len(pcm), 65536):
                kus = pcm[i:i + 65536]
                p = (s + i) % cap
                k = min(len(kus), cap - p)
                RING['buf'][p:p + k] = kus[:k]
                if k < len(kus):
                    RING['buf'][0:len(kus) - k] = kus[k:]
            RING['seq'] = s + len(pcm)
            RING['last'] = time.time()

    def plnic():   # Java: jadro hraje a jeho proud nema prohlizece -> zvuk do /audio.ramu (ATARI_CPP / SEGA / PS1 / PLAYER)
        tony = {k: Ton(f, r) for k, (f, r) in RING_TON.items()}
        posl = time.time()
        while not STAV['konec']:
            time.sleep(0.02)
            ted = time.time()
            dt = ted - posl
            posl = ted
            f = STAV['faze']
            if f in RING_TON and (f == 'mp3' or KLIENTU.get(f, 0) == 0):
                t = tony[f]
                ring_push(t.kus(int(round(dt * t.rate))), t.rate)

    def audio_raw(after):   # port MainActivity.napTvWebWriteAudioRaw
        with rlock:
            end = RING['seq'] & ~3
            rate = RING['rate']
            cap = len(RING['buf'])
            oldest = max(0, end - cap)
            if 0 <= after and oldest <= after <= end:
                start = after & ~3
                dis = False
            else:
                pad = max(8192, rate) & ~3
                start = max(oldest, end - pad) & ~3
                dis = True
            mx = min(65536, max(8192, (rate * 4) // 3))
            if end - start > mx:
                start = (end - mx) & ~3
                dis = True
            n = max(0, end - start)
            body = bytes(RING['buf'][(start + i) % cap] for i in range(n)) if n < 4096 else \
                bytes(RING['buf'][start % cap:start % cap + n]) if start % cap + n <= cap else \
                bytes(RING['buf'][start % cap:]) + bytes(RING['buf'][:(start + n) % cap])
            age = int((time.time() - RING['last']) * 1000) if RING['last'] else 999999
        return body, {'X-NAP-AUDIO-START': str(start), 'X-NAP-AUDIO-SEQ': str(end), 'X-NAP-AUDIO-RATE': str(rate),
                      'X-NAP-AUDIO-AGE': str(age), 'X-NAP-AUDIO-DISCONTINUITY': '1' if dis else '0'}

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
            t0 = T['0'] or time.time()
            DOTAZY.append((round(time.time() - t0, 2), p))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', html)
            if p == '/status':
                # stranka se pta kazdou vterinu - u Reneho se po konci proudu dozvedela "proud=0" az od 409
                # (17:31:59.836 konec proudu, 17:32:00.261 pokus o proud -> 409, status az 17:32:00.666):
                # 0,7 s po zmene /status jeste rika, co bylo (stranka pak jde cestou 409 + napOdmit jako u Reneho)
                f = STAV['faze'] if time.time() - STAV['zmena'] >= STATUS_ZPOZDENI else STAV['predtim']
                pr = f if f in PROUDY else '0'
                a = '1' if f == 'atari' else '0'
                t = ('running=true atari=%s atariCpp=%s atariProud=%s proud=%s sega=%s ps1=%s menu=%s seq=1 h264Seq=0 url=x\n'
                     % (a, a, a, pr, '1' if f == 'sega' else '0', '1' if f == 'ps1' else '0', '1' if f == 'menu' else '0'))
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                LOG.append((round(time.time() - t0, 2), q.get('m', [''])[0]))
                return self.posli(200, 'text/plain', b'ok')
            if p == '/audio.raw':
                try:
                    after = int(q.get('after', ['-1'])[0])
                except ValueError:
                    after = -1
                body, hl = audio_raw(after)
                return self.posli(200, 'application/octet-stream', body, hl)
            if p == '/quality':
                return self.posli(200, 'text/plain', b'tier=2')
            if p in ('/klavesa', '/klavesy'):
                return self.posli(200, 'text/plain', b'ok')
            if p.endswith('.stream'):
                druh = p[1:-7]
                je = proud_ted()
                if druh != je or druh not in PROUDY:   # jako Java B313: napTvProudNeniTu
                    return self.posli(409, 'text/plain', ('proud=' + je + '\n').encode(), {'X-Nap-Proud': je})
                ver, rate, fps, W, H, f = PROUDY[druh]
                ton = Ton(f, rate) if f else None
                time.sleep(PRIPOJENI)
                KLIENTU[druh] += 1
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nCache-Control: no-store\r\n\r\n')
                    self.wfile.flush()
                    zac = time.time()
                    k = 0
                    hotovo = 0
                    while STAV['faze'] == druh and not STAV['konec']:
                        cil = zac + k / fps
                        d = cil - time.time()
                        if d > 0:
                            time.sleep(d)
                        n = int(round((k + 1) * rate / fps)) - hotovo
                        hotovo += n
                        pcm = ton.kus(n) if ton else b''
                        if ver == 1:
                            pk = paket1(pcm, n)
                        else:
                            pk = paket23(ver, rate, W, H, pcm, n if ton else 0)
                        self.wfile.write(struct.pack('<I', len(pk)) + pk)
                        self.wfile.flush()
                        k += 1
                except Exception:
                    pass
                finally:
                    KLIENTU[druh] -= 1
                return
            return self.posli(404, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    port = srv.server_address[1]
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    threading.Thread(target=plnic, daemon=True).start()
    chybyJs = []
    with sync_playwright() as pw:
        b = pw.chromium.launch()
        ctx = b.new_context(viewport={'width': 1280, 'height': 720})
        pg = ctx.new_page()
        pg.on('pageerror', lambda e: chybyJs.append(str(e)))
        pg.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())
        pg.add_init_script(MERENI_JS)
        if avd_ulozene is not None:
            pg.add_init_script("try{if(!sessionStorage.getItem('_b315')){localStorage.setItem('napAvd','%s');sessionStorage.setItem('_b315','1');}}catch(e){}" % avd_ulozene)
        T['0'] = time.time()
        pg.goto('http://127.0.0.1:%d/' % port)
        akce = [(1.5, 'klik')] + ([(3.0, 'sipky')] if sipky_v_menu else [])
        krok = 1
        while True:
            ted = time.time() - T['0']
            while krok < len(POSTUP) and ted >= POSTUP[krok][0]:
                STAV['predtim'] = STAV['faze']
                STAV['zmena'] = time.time()
                STAV['faze'] = POSTUP[krok][1]
                krok += 1
            while akce and ted >= akce[0][0]:
                co = akce.pop(0)[1]
                if co == 'klik':
                    pg.mouse.click(640, 300)        # Rene klikl do stranky (zvuk se odemkne)
                elif co == 'sipky':
                    for _ in range(6):              # 6x sipka dolu v menu na TV (listovani kartami)
                        pg.keyboard.press('ArrowDown')
                        pg.wait_for_timeout(120)
            if STAV['faze'] == 'konec':
                break
            pg.wait_for_timeout(100)
        rms = pg.evaluate('window._rms')
        avd = pg.evaluate("(function(){try{return localStorage.getItem('napAvd');}catch(e){return 'x';}})()")
        STAV['konec'] = True
        b.close()
    srv.shutdown()
    t0ms = T['0'] * 1000.0
    vysl = []
    for jm, od, do in OKNA:
        v = [r for r in rms if od <= (r[0] - t0ms) / 1000.0 < do]
        hraje = sum(1 for r in v if r[1] > PRAH)
        podil = hraje / len(v) if v else 0.0
        stred = sorted(r[1] for r in v)[len(v) // 2] if v else 0.0
        zes = sorted(set(r[3] for r in v))[:4]
        vysl.append((jm, podil, stred, len(v), zes))
    # ozvena pri prechodu stare cesty na proud (Atari podruhe): od prvniho tonu proudu (440 Hz) do
    # posledniho tonu stare cesty (880 Hz); pred tim musela stara cesta opravdu hrat
    ozvena = None
    zac = [r for r in rms if 16.0 <= (r[0] - t0ms) / 1000.0 < 33.0 and len(r) > 5]
    prvni = next((r[0] for r in zac if r[4] > SPEKTRUM_HRAJE), None)
    if prvni is not None:
        pred = any(r[5] > SPEKTRUM_HRAJE for r in zac if prvni - 600 <= r[0] < prvni)
        posl = max([r[0] for r in zac if r[5] > SPEKTRUM_HRAJE and r[0] >= prvni] or [prvni])
        # mezera mezi koncem stare cesty a zacatkem proudu (hrala-li stara cesta tesne pred nim):
        # posledni ton stare cesty pred prvnim tonem proudu -> prvni ton proudu. Okno spektra (93 ms)
        # drzi ton jeste ~80 ms po jeho konci, takze plynula predavka = 0, skutecna mezera se zmensi
        # o ~80 ms. Stara cesta nesmi skoncit (rawPredej) driv, nez zacne proud.
        pred_t = [r[0] for r in zac if prvni - 1000 <= r[0] < prvni and r[5] > SPEKTRUM_HRAJE]
        mezera = max(0.0, (prvni - max(pred_t)) / 1000.0 - 0.025) if pred_t else None
        ozvena = (pred, (posl - prvni) / 1000.0, (prvni - t0ms) / 1000.0, mezera)
    return {'okna': vysl, 'log': LOG, 'js': chybyJs, 'avd': avd, 'rms': rms, 't0ms': t0ms, 'dotazy': DOTAZY, 'ozvena': ozvena}


def popis(r):
    return ', '.join('%s %s (%.0f %%)' % (jm, 'HRAJE' if podil >= 0.9 else 'TICHO' if podil < 0.1 else 'kousky', podil * 100)
                     for jm, podil, stred, n, zes in r['okna'])


def hraje_vse(r):
    return all(podil >= 0.9 for jm, podil, stred, n, zes in r['okna'])


def main():
    zdroj_b315 = open(os.path.join(KOREN, JAVA_CESTA), encoding='utf-8').read()
    html_b315 = stranka_z_javy(zdroj_b315).encode('utf-8')
    try:
        zdroj_b314 = subprocess.run(['git', '-C', KOREN, 'show', B314_COMMIT + ':' + JAVA_CESTA],
                                    capture_output=True, text=True, check=True).stdout
        html_b314 = stranka_z_javy(zdroj_b314).encode('utf-8')
    except Exception as e:
        html_b314 = None
        tisk('(stranka B314 z gitu neni k dispozici: %s)' % e)

    tisk('B315 - zvuk na TV / PC po prvni hre (stranka WEB TV z MainActivity.java v Chromiu, mereno pred reproduktorem)')
    tisk('postup jako v logu Reneho: menu, kliknuti, Atari, NET_HRY (proud 0), Atari znovu, menu, Sega, menu, PS1, MP3')
    scen = [
        ('vychozi zpozdeni (AVD 0,3 s)', None, False),
        ('6x sipka dolu v menu na TV (jako listovani kartami)', None, True),
        ('ulozene zpozdeni 0 ms (z drivejska v prohlizeci)', '0', False),
        ('ulozene zpozdeni 2 s (nejvic)', '2', False),
    ]
    if html_b314:
        tisk('')
        tisk('--- stranka B314 (git %s) - musi ukazat chybu Reneho ---' % B314_COMMIT)
        r = scenar(html_b314, 'B314 vychozi', None, False)
        tisk('B314 %s: %s' % (scen[0][0], popis(r)))
        over(hraje_vse(r) and not r['js'], 'B314 s vychozim zpozdenim hraje vsude (chyba neni v proudu z C++ ani v mezere samotne)')
        oz = r['ozvena']
        OZ_B314[0] = oz[1] if oz and oz[0] else None
        tisk('   B314 prechod stare cesty na proud (Atari podruhe, %.1f s): stara cesta hrala pres proud jeste %s, mezera %s'
             % (oz[2] if oz else -1, ('%.2f s (ozvena)' % oz[1]) if oz and oz[0] else '? (pred proudem nehrala)',
                ('%.2f s' % oz[3]) if oz and oz[3] is not None else '-'))
        r = scenar(html_b314, 'B314 sipky', None, True)
        tisk('B314 %s: %s, ulozene AVD=%s' % (scen[1][0], popis(r), r['avd']))
        ok = r['okna'][0][1] >= 0.9 and all(podil < 0.1 for jm, podil, stred, n, zes in r['okna'][1:])
        over(ok and r['avd'] == '0', 'B314 po 6x sipce dolu v menu: prvni Atari hraje, pak TICHO v Atari, Sege, PS1 i MP3 = PRESNE chyba Reneho '
             '(sipky v menu zmenily zpozdeni na 0 a prvni navazani stare cesty nechalo zesilovac na 0)')
        tisk('   zesilovace stranky B314 v tichu (g): %s' % ', '.join(sorted(set(z for o in r['okna'][1:] for z in o[4]))))
        r = scenar(html_b314, 'B314 ulozene 0', '0', False)
        tisk('B314 %s: %s' % (scen[2][0], popis(r)))
        ok = r['okna'][0][1] >= 0.9 and all(podil < 0.1 for jm, podil, stred, n, zes in r['okna'][1:])
        over(ok, 'B314 s ulozenym zpozdenim 0 (zustava v prohlizeci i po F5): stejne - zvuk jen poprve')

    tisk('')
    tisk('--- stranka B315 (z MainActivity.java v pracovnim stromu) ---')
    for nazev, avd, sipky in scen:
        r = scenar(html_b315, nazev, avd, sipky)
        tisk('B315 %s: %s, ulozene AVD=%s' % (nazev, popis(r), r['avd']))
        for jm, podil, stred, n, zes in r['okna']:
            tisk('   %-30s vzorku %4d, hraje %5.1f %%, stredni RMS %.3f, zesilovace %s' % (jm, n, podil * 100, stred, ' '.join(zes)))
        over(hraje_vse(r), 'B315 %s: zvuk hraje v Atari poprve i podruhe, v Sege, PS1 i v MP3 (stara cesta)' % nazev)
        over(not r['js'], 'B315 %s: zadna chyba JavaScriptu%s' % (nazev, (': ' + '; '.join(r['js'][:3])) if r['js'] else ''))
        oz = r['ozvena']
        # pri zpozdeni 2 s stara cesta pred proudem jeste nezacala hrat - pak jen: po startu proudu nehraje
        # zpozdeni 2 s: stara cesta pred proudem jeste nezacala hrat (pak jen: po startu proudu nehraje)
        ok = oz is not None and oz[1] <= 0.15 and (oz[0] or avd == '2') and (oz[3] is None or oz[3] <= 0.15)
        over(ok, 'B315 %s: prechod stare cesty na proud Atari (%.1f s) - stara cesta %s skonci do 0,15 s po zacatku proudu '
             '(%.2f s; B314 %s), mezera mezi nimi %s' % (nazev, oz[2] if oz else -1, 'hrala a' if oz and oz[0] else 'nehrala,',
                                                         oz[1] if oz else -1,
                                                         ('%.2f s ozveny' % OZ_B314[0]) if OZ_B314[0] is not None else '-',
                                                         ('%.2f s' % oz[3]) if oz and oz[3] is not None else '-'))
        if sipky:
            over(r['avd'] in (None, '0.3'), 'B315 sipky v menu listuji kartami a zpozdeni zvuku NEMENI (ulozene AVD=%s)' % r['avd'])
        zpr = [m for t, m in r['log'] if m.startswith('B315')]
        if nazev.startswith('vychozi'):
            tisk('   zpravy B315 do logu appky: %s' % ' | '.join(zpr[:6]))
            over(any('B315 ZVUK' in m for m in zpr), 'B315 stranka hlasi do logu appky stav zvuku (zpozdeni stare cesty, uroven vystupu)')
            rep = [m for t, m in r['log'] if 'PROHLIZEC' in m]
            for m in rep:
                tisk('   hlaseni: %s' % m)
            # ton RMS 0,21 = -13,5 dB; za 30 s okna hralo i tiche menu a mezery -> jen "zvuk v N %" > 0
            over(len(rep) >= 2 and all(re.search(r'vystup max -1[2-5] dB, zvuk v [1-9]\d* % mereni', m) for m in rep),
                 'hlaseni PROHLIZEC (kazdych 30 s, Atari i PS1) uvadi skutecnou uroven vystupu (ton -13 dB, ne jen "zvuk hraje")')
        if avd == '0':
            zp = [m for m in zpr if 'ulozene bylo 0 ms' in m]
            tisk('   %s' % (zp[0] if zp else '(zadna zprava o ulozenem zpozdeni)'))
            over(r['avd'] == '0.3' and bool(zp), 'B315 ulozene zpozdeni 0 (od sipek v menu) se vrati na 300 ms a do logu appky to pise'
                 ' "ulozene bylo 0 ms" (Reneho dalsi log ukaze, jestli to byla tahle pricina)')
        if avd == '2':
            over(r['avd'] == '2', 'B315 rucne nastavene zpozdeni 2 s (nejvic) zustava')

    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    open(os.path.join(ZDE, 'VYSTUP_test_b315_zvuk_tv.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
