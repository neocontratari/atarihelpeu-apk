#!/usr/bin/env python3
# B302: KONEC-KONCU test klavesnice pocitace pro Atari 130XE (C++) ve web
# prohlizeci appky (TV / PC).
#
#  - stranka prohlizece = PRESNE ta z appky: vytahne se z MainActivity.java
#    (napTvWebWriteHtml), zadna kopie,
#  - Chromium (Playwright) = prohlizec na PC: skutecne udalosti klavesnice
#    (e.code, e.key, Shift, opakovani, CapsLock, F-klavesy, ztrata zamereni),
#  - tenhle Python server = to, co v appce dela Java: /status (atari=1
#    atariCpp=1 klavRezim=...), /klavesa a /klavesy (code, znak, dolu, ctrl,
#    mod -> devPcKlavesaNative),
#  - libpcklav.so = skutecne jadro Atari + skutecna nap_atari_pc_klavesnice.h,
#    vlakno 50 snimku/s jako emulacni vlakno v appce.
#
#   g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp
#   python3 test_b302_prohlizec.py          -> VYSTUP_test_b302_prohlizec.txt
#   python3 test_b302_prohlizec.py stary    -> jen "pomala Wi-Fi" se strankou z B301
#                                              (VYSTUP_test_b302_prohlizec_B301.txt)
import ctypes
import http.server
import os
import re
import subprocess
import sys
import threading
import time
import urllib.parse

ZDE = os.path.dirname(os.path.abspath(__file__))
JAVA_CESTA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
B301 = '1f077e9'      # commit B301 (stranka prohlizece pred B302)


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


STARY = len(sys.argv) > 1 and sys.argv[1] == 'stary'
if STARY:
    JAVA_SRC = subprocess.run(['git', 'show', B301 + ':' + JAVA_CESTA], cwd=os.path.join(ZDE, '../..'),
                              capture_output=True, text=True, check=True).stdout
else:
    JAVA_SRC = open(os.path.join(ZDE, '../..', JAVA_CESTA), encoding='utf-8').read()
HTML = stranka_z_javy(JAVA_SRC).encode('utf-8')

lib = ctypes.CDLL(os.path.join(ZDE, 'libpcklav.so'))
lib.pk_klavesa.restype = ctypes.c_char_p
lib.pk_klavesa.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
lib.pk_obrazovka.restype = ctypes.c_char_p
lib.pk_log.restype = ctypes.c_char_p

POZADAVKY = []          # vsechny klavesy (jak prisly)
DAVKY = []              # kolik klaves prislo v jednom pozadavku
PZ = threading.Lock()
ZPOZDENI = [0.0]        # "pomala Wi-Fi": tolik sekund trva kazdy pozadavek s klavesami
POSLEDNI = [0.0]        # cas posledniho pozadavku s klavesami


def jedna(code, znak, dolu, ctrl, mod):
    """Java napTvWebJednaKlavesa (vetev B302): mod, u stareho prohlizece jen ctrl."""
    if mod < 0:
        mod = 2 if ctrl else 0
    r = lib.pk_klavesa(code.encode('utf-8'), znak.encode('utf-8'), mod & 0xFF, 1 if dolu else 0).decode()
    with PZ:
        POZADAVKY.append((code, znak, dolu, mod, r))
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

    def do_GET(self):
        u = urllib.parse.urlsplit(self.path)
        q = urllib.parse.parse_qs(u.query, keep_blank_values=True)
        g = lambda k, d='': q.get(k, [d])[0]
        if u.path in ('/klavesa', '/klavesy'):
            if ZPOZDENI[0]:
                time.sleep(ZPOZDENI[0])
            POSLEDNI[0] = time.monotonic()
        if u.path == '/':
            self.posli(200, 'text/html; charset=utf-8', HTML)
        elif u.path == '/status':
            # jako Java /status: Atari 130XE (C++) je na obrazovce
            rezim = lib.pk_klavesa(b'STAV', b'', 0, 1).decode()
            self.posli(200, 'text/plain', ('running=true atari=1 atariCpp=1 klavRezim=%s sega=0 seq=0 h264Seq=0 url=file:///android_asset/emu_atari_cpp/index.html\n' % rezim).encode())
        elif u.path == '/klavesa':
            r = jedna(g('code'), g('znak'), int(g('dolu', '1') or 1) != 0, int(g('ctrl', '0') or 0) != 0, int(g('mod', '-1') or -1))
            with PZ:
                DAVKY.append(1)
            self.posli(200, 'text/plain', r.encode())
        elif u.path == '/klavesy':
            # jako Java napTvWebKlavesy: d=code,znak,dolu,ctrl,mod;... (pole zvlast URL-kodovana)
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
            with PZ:
                DAVKY.append(len(odp))
            self.posli(200, 'text/plain', '\n'.join(odp).encode())
        else:
            self.posli(404, 'text/plain', b'nic')


def emulace(stop):
    dalsi = time.monotonic()
    while not stop.is_set():
        lib.pk_snimek()
        dalsi += 0.02
        t = dalsi - time.monotonic()
        if t > 0:
            time.sleep(t)
        elif t < -0.2:
            dalsi = time.monotonic()


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

    obr = lambda: lib.pk_obrazovka().decode()
    tisk('B302: prohlizec (Chromium) -> server jako Java -> C++ PcKlavesnice/PcPrehravac -> jadro Atari 130XE')
    tisk('stranka z MainActivity.java%s: %d bajtu, skriptu %d' % (' (B301, ' + B301 + ')' if STARY else '', len(HTML), HTML.count(b'<script>')))
    over(lib.pk_init() == 1, 'Atari nastartovalo do READY')

    stop = threading.Event()
    threading.Thread(target=emulace, args=(stop,), daemon=True).start()
    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Server)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = 'http://127.0.0.1:%d/' % srv.server_address[1]

    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        try:
            br = p.chromium.launch()
        except Exception:
            br = p.chromium.launch(executable_path='/opt/pw-browsers/chromium/chrome-linux/chrome')
        page = br.new_page()
        page.route(re.compile(r'^https?://(?!127\.0\.0\.1).*'), lambda r: r.abort())   # zadny internet (JMuxer z CDN)
        chyby_js = []
        page.on('pageerror', lambda e: chyby_js.append(str(e)))
        page.goto(url)
        page.wait_for_function('window.napVAtari===true', timeout=8000)
        time.sleep(1.0)

        def napis(t, delay=25):
            page.keyboard.type(t, delay=delay)

        def cekej_klavesy(navic=0.3):
            # az prohlizec nic neposila (fronta v prohlizeci prazdna) a C++ vse predalo stroji
            for _ in range(600):
                if time.monotonic() - POSLEDNI[0] > 0.5 and lib.pk_fronta() == 0:
                    break
                time.sleep(0.02)
            time.sleep(navic)

        def popis_klaves():
            return page.evaluate("(function(){var d=[].slice.call(document.querySelectorAll('div')).filter(function(x){"
                                 "return /F9/.test(x.textContent||'')&&x.style.display!=='none'});return d.length?d[0].textContent:'';})()")

        def pomala_wifi():
            # Wi-Fi telefonu v uspornem rezimu: kazdy pozadavek 150 ms, pisar ~16 znaku/s
            ZPOZDENI[0] = 0.15
            radek = '70 REM POMALA WIFI 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ'
            t0 = time.monotonic()
            napis(radek, 30)
            page.keyboard.press('Enter')
            cekej_klavesy(0.5)
            tisk('pomala Wi-Fi (150 ms na pozadavek): %d znaku napsano za %.1f s, Atari je mel za %.1f s'
                 % (len(radek), 30 * len(radek) / 1000.0, time.monotonic() - t0))
            ZPOZDENI[0] = 0.0
            napis('LIST 70')
            page.keyboard.press('Enter')
            cekej_klavesy(0.8)
            # radek programu delsi nez 38 znaku pokracuje na dalsim radku obrazovky
            rad = obr().split('\n')
            vypis = None
            for k in range(len(rad) - 1, -1, -1):
                if rad[k].startswith('  70 REM'):
                    vypis = rad[k][2:]
                    j = k
                    while len(rad[j]) == 40 and j + 1 < len(rad):
                        j += 1
                        vypis += rad[j][2:]
                    break
            tisk('obrazovka:\n' + '\n'.join(r for r in rad if r.strip()))
            tisk('na Atari: ' + (vypis if vypis is not None else '(radek 70 neni)'))
            over(vypis == radek, 'pomala Wi-Fi: rychle psani - do Atari doslo vsechno (nic se nezahodilo)')

        if STARY:
            pomala_wifi()
        else:
            over(page.evaluate('window.napVAtariCpp===true'), 'stranka z appky: /status atari=1 atariCpp=1 -> klavesnice zapnuta sama (napVAtari, napVAtariCpp)')
            popis = popis_klaves()
            tisk('popis klaves: ' + popis)
            over(popis.startswith('ATARI - pis normalne') and 'F6 HELP' in popis, 'popis klaves pro Atari 130XE (C++) v rohu obrazovky')
            # 1) psani programu: velka/mala, uvozovky, ?, strednik, dvojice 00 a LL
            napis('10 PRINT "Ahoj";100;:? 2*3')
            page.keyboard.press('Enter')
            # 2) CapsLock = CAPS na Atari (mala pismena), pak zpet
            page.keyboard.press('CapsLock')
            napis('20 ? "male"')
            page.keyboard.press('Enter')
            page.keyboard.press('CapsLock')
            # 3) Backspace
            napis('30 ? 7X')
            page.keyboard.press('Backspace')
            page.keyboard.press('Enter')
            # 4) rychle psani bez prodlevy (vse jde po siti naraz)
            page.keyboard.type('40 REM BALL 1001 ZZZ', delay=0)
            page.keyboard.press('Enter')
            cekej_klavesy(0.5)
            napis('LIST')
            page.keyboard.press('Enter')
            cekej_klavesy(1.5)
            s = obr()
            tisk('--- LIST ---\n' + s + '---')
            over('10 PRINT "AHOJ";100;:? 2*3' in s, 'radek 10 z prohlizece presne (Shift, uvozovky, 00)')
            over('20 ? "male"' in s, 'CapsLock z prohlizece = CAPS na Atari (mala pismena)')
            over('30 ? 7' in s and '7X' not in s, 'Backspace z prohlizece')
            over('40 REM BALL 1001 ZZZ' in s, 'rychle psani bez prodlevy (LL, 00, ZZZ) - nic neztraceno')
            over('ERROR' not in s, 'zadna chyba syntaxe')
            # 5) sipky nemeni zpozdeni zvuku (driv sipky a 0 menily "ZVUK ms")
            page.keyboard.press('ArrowUp')
            page.keyboard.press('ArrowDown')
            time.sleep(0.3)
            avd = page.evaluate("(function(){var o=document.getElementById('avdmsg');return o?o.style.display:'zadny';})()")
            over(avd in ('zadny', 'none'), 'sipky v Atari = kurzor, ne zpozdeni zvuku (avdmsg: %s)' % avd)
            # 6) drzena klavesa: Atari ji opakuje samo (OS), opakovani od prohlizece nic nepridava
            napis('50 REM ')
            cekej_klavesy(0.2)
            with PZ:
                pred_x = len(POZADAVKY)
            page.keyboard.down('x')
            for _ in range(10):
                page.keyboard.down('x')          # Playwright: dalsi down = opakovani (e.repeat)
                time.sleep(0.15)
            page.keyboard.up('x')
            page.keyboard.press('Enter')
            cekej_klavesy(0.5)
            with PZ:
                rep = [r for r in POZADAVKY[pred_x:] if r[0] == 'KeyX' and r[2]]
            over(len(rep) == 11 and sum(1 for r in rep if r[3] & 16) == 10 and all(r[4] == 'DRZENO' for r in rep[1:]),
                 'opakovani z prohlizece (e.repeat, mod 16) -> DRZENO (%d stisku, %d opakovani)' % (len(rep), sum(1 for r in rep if r[3] & 16)))
            napis('LIST 50')
            page.keyboard.press('Enter')
            cekej_klavesy(0.8)
            m = re.search(r'50 REM (X+)', obr())
            tisk('drzene X 1,5 s -> %d znaku X' % (len(m.group(1)) if m else 0))
            over(m is not None and len(m.group(1)) >= 5, 'drzena klavesa se opakuje jako na skutecnem Atari (OS)')
            # 7) RUN a BREAK (F2)
            napis('60 GOTO 60')
            page.keyboard.press('Enter')
            napis('RUN')
            page.keyboard.press('Enter')
            cekej_klavesy(0.8)
            page.keyboard.press('F2')
            cekej_klavesy(0.8)
            s = obr()
            over('AHOJ1006' in s and 'STOPPED' in s and 'AT LINE 60' in s, 'RUN (AHOJ1006) a F2 = BREAK (STOPPED AT LINE 60)')
            # 8) pomala Wi-Fi telefonu
            pomala_wifi()
            # 9) F9 = HRANI: WASD + L z prohlizece -> joystick 1
            page.keyboard.press('F9')
            time.sleep(1.6)
            popis = popis_klaves()
            over(popis.startswith('ATARI HRANI'), 'F9: popis prepnut na HRANI (%s)' % popis[:40])
            page.keyboard.down('w')
            page.keyboard.down('l')
            time.sleep(0.25)
            j1 = lib.pk_joy()
            page.keyboard.up('w')
            time.sleep(0.25)
            j2 = lib.pk_joy()
            page.keyboard.up('l')
            page.keyboard.down('ArrowLeft')
            page.keyboard.down('ArrowDown')
            time.sleep(0.25)
            j3 = lib.pk_joy()
            page.keyboard.up('ArrowLeft')
            page.keyboard.up('ArrowDown')
            time.sleep(0.25)
            j4 = lib.pk_joy()
            tisk('joystick: W+L %d, jen L %d, vlevo+dolu %d, nic %d' % (j1, j2, j3, j4))
            over(j1 == 17 and j2 == 16 and j3 == 6 and j4 == 0, 'HRANI: W = nahoru, L = FIRE, sipky = smer, po pusteni klid')
            # ztrata zamereni okna (prepnuti jinam) = vse pustit
            page.keyboard.down('d')
            time.sleep(0.25)
            jd = lib.pk_joy()
            page.evaluate("window.dispatchEvent(new Event('blur'))")
            time.sleep(0.4)
            over(jd == 8 and lib.pk_joy() == 0, 'okno ztratilo zamereni (blur) -> PustVse: joystick pusten')
            page.keyboard.up('d')
            # F1 = START drzeny
            page.keyboard.down('F1')
            time.sleep(0.25)
            k1 = lib.pk_kon()
            page.keyboard.up('F1')
            time.sleep(0.25)
            over(k1 == 1 and lib.pk_kon() == 0, 'F1 = START drzeny / pusteny')
            # hlidani spojeni: drzena klavesa bez opakovani (Playwright neopakuje) - prohlizec
            # posila kazdych 0,8 s "Zije", C++ ji drzi
            page.keyboard.down('a')
            time.sleep(4.0)
            ja = lib.pk_joy()
            over(ja == 4 and lib.pk_ticho() == 0, 'drzena klavesa 4 s bez opakovani: "Zije" ji drzi (joystick vlevo = %d)' % ja)
            # spojeni spadlo (Wi-Fi): pozadavky s klavesami nedojdou - C++ po 2,5 s vse pusti
            spadlo = re.compile(r'.*/klaves[ay]\?.*')
            page.route(spadlo, lambda r: r.abort())
            time.sleep(3.5)
            over(lib.pk_joy() == 0 and lib.pk_ticho() == 1, 'spojeni spadlo: C++ po 2,5 s pustil drzenou klavesu (joystick v klidu)')
            page.unroute(spadlo)
            page.keyboard.up('a')
            time.sleep(1.0)
            page.keyboard.press('F9')
            time.sleep(0.3)
            with PZ:
                pz = list(POZADAVKY)
                dv = list(DAVKY)
            over(any(r[0] == 'F9' and r[4] == 'PSANI' for r in pz), 'F9 = zpet PSANI')
            over(any(r[0] == 'PustVse' for r in pz), 'blur poslal PustVse')
            neznam = [r for r in pz if r[4].startswith('NEZNAMA')]
            over(not neznam, 'zadna klavesa z testu neskoncila jako NEZNAMA %s' % (neznam[:3],))
            tisk('klaves %d v %d pozadavcich (nejvic %d v jednom)' % (len(pz), len(dv), max(dv) if dv else 0))
        over(not chyby_js, 'zadna chyba JavaScriptu na strance %s' % (chyby_js[:2],))
        br.close()
    stop.set()
    srv.shutdown()
    tisk('\nVYSLEDEK: %d kontrol, %d chyb' % (kontrol[0], chyb[0]))
    jm = 'VYSTUP_test_b302_prohlizec_B301.txt' if STARY else 'VYSTUP_test_b302_prohlizec.txt'
    with open(os.path.join(ZDE, jm), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb[0] else 0


if __name__ == '__main__':
    sys.exit(main())
