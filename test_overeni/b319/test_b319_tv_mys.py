#!/usr/bin/env python3
"""B319: MYS POCITACE -> MYS PLAYSTATION V PROHLIZECI NA TV / PC - JEDNIM SPOJENIM, NEJVYS ~30 ZPRAV / S
(+ vse z B318: stara cesta az podle /status).

Rene (log B318): "je to mnohem mnohem lepsi - ale mam pocit, ze doslo ke zpomaleni pri prenosu LAN".
Mys z PC posilala ~25 zprav za sekundu a kazda otevrela nove spojeni (navic pakety pres Wi-Fi vedle
obrazu). B319: zpravy nejvys po 33 ms (tlacitka hned) a appka spojeni drzi (keep-alive - overuje
test_b319_spojeni.py se skutecnym kodem z MainActivity). Stranka WEB TV ma navic kopii a JPEG v
polovicnim rozliseni (test_b319_prohlizec.py).

  - stranka = PRESNE ta z appky (vytahne se z MainActivity.napTvWebWriteHtml), anglicky s
    prekladacem a slovnikem presne jako Java (napTvJazykStranka),
  - Python server = co dela Java: /status (ps1=1 ps1mys=1 ...), /ps1.stream (pakety verze 3 jako
    C++ proud PS1, obraz 368x240 jako Dune 2000), /mys (zapise, co prislo), /stream.mjpg, /frame.jpg,
    /clientlog,
  - zamek ukazatele (requestPointerLock) a cela obrazovka jsou v Chromiu bez okna nahrazene
    (pocitaji volani, posilaji pointerlockchange) - vse ostatni je skutecna stranka.
Kontroly (B318 beze zmeny + B319):
  1) s proudem z C++ stranka starou cestu (MJPEG / JPEG) vubec neotevre - B317 ji otevrela hned,
  2) PS1 s mysi: napoveda "klikni do obrazu", klik = zamek ukazatele (ne prepnuti cele obrazovky),
     pohyb mysi -> /mys v 1/100000 sirky obrazu (soucet presne), leve / prave tlacitko, "jsem tu"
     pri drzenem tlacitku (C++ hlida 2,5 s), panel OBRAZ se pri hre neukazuje, Esc = pusteni (b=0),
     mys na telefonu vypnuta -> zamek se sam uvolni, bez mysi klik zase prepina celou obrazovku,
  2b) B319: stale pohyb mysi 1,5 s (jako hraní): nejvys ~30 zprav / s, soucet presne, tlacitko hned,
  3) obrazovka bez proudu (DJ, MP3 ...): stara cesta se pusti hned po /status; /status neodpovida:
     po 2,5 s jako drive,
  4) cesky = stranka B318 + jen zmeny B319; anglicky bez cestiny v napovedach mysi,
  5) zadna chyba JavaScriptu.
Pouziti: python3 test_b319_tv_mys.py        -> VYSTUP_test_b319_tv_mys.txt
"""
import http.server
import os
import struct
import subprocess
import sys
import threading
import time
import urllib.parse

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
ASSETS = os.path.join(KOREN, 'app/src/main/assets')
B317 = '2f8fff4'
B318 = 'e465b07'
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
    """HTML prohlizece presne tak, jak ho sklada MainActivity.napTvWebWriteHtml (String body = ...;)."""
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


def anglicky_jako_java(body):
    """MainActivity.napTvJazykStranka: za </title> NAP_JAZYK='en' + nap_jazyk.js + en_tv.js"""
    js = open(os.path.join(ASSETS, 'spolecne/nap_jazyk.js'), encoding='utf-8').read() + '\n' + \
        open(os.path.join(ASSETS, 'jazyk/en_tv.js'), encoding='utf-8').read()
    i = body.index('</title>')
    vlozit = "<script>window.NAP_JAZYK='en';</script><script>" + js.replace('</script', '<\\/script') + '</script>'
    return body[:i + 8] + vlozit + body[i + 8:]


JAVA = open(os.path.join(KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
CZ = stranka_z_javy(JAVA)
EN = anglicky_jako_java(CZ)
STARA = stranka_z_javy(subprocess.run(['git', '-C', KOREN, 'show', B317 + ':app/src/main/java/eu/atarihelp/emu10/MainActivity.java'],
                                      capture_output=True, text=True, check=True).stdout)
STARA318 = stranka_z_javy(subprocess.run(['git', '-C', KOREN, 'show', B318 + ':app/src/main/java/eu/atarihelp/emu10/MainActivity.java'],
                                         capture_output=True, text=True, check=True).stdout)

# zamek ukazatele a cela obrazovka: v Chromiu bez okna nahrazene - pocitaji se volani
NAHRADY = r"""
(function(){
  var zamek=null;
  Object.defineProperty(Document.prototype,'pointerLockElement',{get:function(){return zamek;},configurable:true});
  Element.prototype.requestPointerLock=function(){window.__zamekZadost=(window.__zamekZadost||0)+1;var el=this;
    if(window.__zamekOdmitni){setTimeout(function(){document.dispatchEvent(new Event('pointerlockerror'));},5);return Promise.reject(new Error('odmitnuto'));}
    setTimeout(function(){zamek=el;document.dispatchEvent(new Event('pointerlockchange'));},10);return Promise.resolve();};
  Document.prototype.exitPointerLock=function(){window.__zamekKonec=(window.__zamekKonec||0)+1;
    setTimeout(function(){zamek=null;document.dispatchEvent(new Event('pointerlockchange'));},10);};
  Element.prototype.requestFullscreen=function(){window.__fsZadost=(window.__fsZadost||0)+1;return Promise.resolve();};
  Document.prototype.exitFullscreen=function(){window.__fsKonec=(window.__fsKonec||0)+1;return Promise.resolve();};
})();
"""


def paket3(W, H):
    """PS1 (verze 3: JPEG po castech) - bez zmeny obrazu, bez zvuku (jako apPaket23 stranky)"""
    p = b'NA' + bytes([3, 0]) + struct.pack('<HH', W, H) + b'\0' * 4
    p += struct.pack('<HHHH', 0, 0, W, H) + bytes([4, 3, 0, 0]) + struct.pack('<I', 44100)
    p += struct.pack('<HHHHBI', 0, 0, 0, 0, 0, 0)
    return p + struct.pack('<HB', 0, 2)


def server(html, stav, dotazy, mys, log):
    T0 = time.time()

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
            dotazy.append((round(time.time() - T0, 2), p))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', html.encode('utf-8'))
            if p == '/status':
                if stav['status'] == 'visi':     # stary / zaseknuty server: neodpovi vubec
                    time.sleep(6)
                    return
                if not stav['status']:
                    return self.posli(500, 'text/plain', b'chyba')
                pr = stav['proud']
                t = ('running=true atari=0 atariCpp=0 atariProud=0 proud=%s sega=0 ps1=%s ps1mys=%s menu=0 seq=1 h264Seq=0 url=x\n'
                     % (pr, stav['ps1'], stav['ps1mys']))
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                log.append((round(time.time() - T0, 2), q.get('m', [''])[0]))
                return self.posli(200, 'text/plain', b'ok')
            if p == '/mys':
                mys.append((round(time.time() - T0, 3), int(q['dx'][0]), int(q['dy'][0]), int(q['b'][0])))
                return self.posli(200, 'text/plain', b'PS1:MYS' if stav['ps1mys'] == '1' else b'PS1:MYS_VYP')
            if p in ('/quality',):
                return self.posli(200, 'text/plain', b'tier=2')
            if p in ('/klavesa', '/klavesy', '/audio.raw'):
                return self.posli(200, 'text/plain', b'')
            if p == '/frame.jpg':
                return self.posli(503, 'text/plain', b'frame not ready\n')
            if p == '/stream.mjpg':
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=napframe\r\n\r\n')
                    self.wfile.flush()
                    while not stav['konec']:
                        time.sleep(0.1)
                except Exception:
                    pass
                return
            if p == '/ps1.stream':
                if stav['proud'] != 'ps1':
                    return self.posli(409, 'text/plain', b'proud=0\n', {'X-Nap-Proud': stav['proud']})
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                    self.wfile.flush()
                    while not stav['konec'] and stav['proud'] == 'ps1':
                        pk = paket3(368, 240)
                        self.wfile.write(struct.pack('<I', len(pk)) + pk)
                        self.wfile.flush()
                        time.sleep(1 / 60)
                except Exception:
                    pass
                return
            return self.posli(404, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def otevri(b, html, stav, nahrady=True):
    dotazy, mys, log, chybyJs = [], [], [], []
    srv = server(html, stav, dotazy, mys, log)
    ctx = b.new_context(viewport={'width': 1280, 'height': 720})
    if nahrady:
        ctx.add_init_script(NAHRADY)
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chybyJs.append(str(e)))
    pg.goto('http://127.0.0.1:%d/' % srv.server_address[1])
    return srv, ctx, pg, dotazy, mys, log, chybyJs


def stare_cesty(dotazy, do_s=None):
    return [d for d in dotazy if d[1] in ('/stream.mjpg', '/frame.jpg', '/stream.h264') and (do_s is None or d[0] <= do_s)]


def napoveda(pg):
    return pg.evaluate("(function(){var h=document.getElementById('pmh');return h&&h.style.display!=='none'?h.textContent:'';})()")


def pohyb(pg, dx, dy):
    pg.evaluate("([x,y])=>document.documentElement.dispatchEvent(new MouseEvent('mousemove',{bubbles:true,movementX:x,movementY:y}))", [dx, dy])


def tlacitko(pg, typ, b):
    pg.evaluate("([t,b])=>document.documentElement.dispatchEvent(new MouseEvent(t,{bubbles:true,button:b}))", [typ, b])


def main():
    tisk('B319 - mys pocitace -> mys PlayStation v prohlizeci (TV / PC): nejvys ~30 zprav / s (+ B318: stara cesta az podle /status)')
    tisk('stranka WEB TV z MainActivity.java (%d znaku), B317 z %s (%d znaku)' % (len(CZ), B317, len(STARA)))
    with sync_playwright() as p:
        b = p.chromium.launch()

        # ===== 1) proud z C++: stara cesta vubec ne (B317 hned MJPEG) =====
        stav = {'proud': 'ps1', 'ps1': '1', 'ps1mys': '1', 'status': True, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, CZ, stav)
        time.sleep(3.2)
        st = stare_cesty(dotazy)
        t_str = [d[0] for d in dotazy if d[1] == '/'][0]
        proud = [d[0] - t_str for d in dotazy if d[1] == '/ps1.stream']
        audio = [d for d in dotazy if d[1] == '/audio.raw']
        over(not st and proud and proud[0] < 0.6 and not audio,
             '1) PS1 s proudem z C++: prohlizec bere /ps1.stream (%.2f s po nacteni) a starou cestu (MJPEG / JPEG / zvuk /audio.raw z Javy) vubec neotevrel'
             % (proud[0] if proud else -1))
        sirka = pg.evaluate("parseFloat(window.napAp.cv.style.width)")
        over(sirka > 300, '   obraz hry (368x240) v canvasu siroky %.0f bodu stranky' % sirka)

        # ===== 2) napoveda, klik = zamek ukazatele =====
        h = napoveda(pg)
        over(h == 'MYS PS1: KLIKNI DO OBRAZU - KURZOR HRY OVLADA MYS POCITACE (ESC = PUSTIT)',
             '2) /status ps1mys=1: napoveda nahore "%s"' % h)
        over(any('B318 MYS PC: na telefonu je PS1 s mysi PlayStation' in m for _, m in log), '   do logu appky: "B318 MYS PC: na telefonu je PS1 s mysi PlayStation ..."')
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        z = pg.evaluate("[window.__zamekZadost||0, window.__fsZadost||0, window.__fsKonec||0, window.napPm.lock]")
        over(z[0] == 1 and z[1] == 1 and z[2] == 0 and z[3] is True,
             '   klik do obrazu: zamek ukazatele (%d), cela obrazovka zapnuta (%d), ne vypnuta (%d) - mys zachycena' % (z[0], z[1], z[2]))
        h = napoveda(pg)
        over(h.startswith('MYS PS1 ZACHYCENA'), '   napoveda: "%s"' % h)
        n0 = len(mys)
        time.sleep(0.2)
        over(len(mys) > n0 - 1 and any(m[3] == 0 for m in mys), '   po zachyceni hned zprava /mys (tlacitka pustena) - appka vi, ze prohlizec zije')

        # pohyb: 10 x (+30, -12) bodu stranky
        mys.clear()
        for _ in range(10):
            pohyb(pg, 30, -12)
            time.sleep(0.008)
        time.sleep(0.4)
        sx, sy = sum(m[1] for m in mys), sum(m[2] for m in mys)
        ocek_x, ocek_y = 300 * 100000 / sirka, -120 * 100000 / sirka
        over(abs(sx - ocek_x) <= 2 and abs(sy - ocek_y) <= 2 and len(mys) <= 10,
             '   pohyb mysi 300 / -120 bodu stranky -> /mys %d / %d (1/100000 sirky obrazu, vypocet %.0f / %.0f) v %d zpravach'
             % (sx, sy, ocek_x, ocek_y, len(mys)))
        over(sx * 320 / 100000 > 0 and abs(sx * 320 / 100000 - 300 / sirka * 320) < 0.1,
             '   v C++ = %.1f bodu mysi PS1 pri 100 %% (cela sirka obrazu = 320 jako prst na telefonu)' % (sx * 320 / 100000))

        # tlacitka
        mys.clear()
        tlacitko(pg, 'mousedown', 0)
        time.sleep(0.15)
        tlacitko(pg, 'mouseup', 0)
        time.sleep(0.15)
        tlacitko(pg, 'mousedown', 2)
        time.sleep(0.15)
        tlacitko(pg, 'mouseup', 2)
        time.sleep(0.2)
        bb = [m[3] for m in mys]
        over(bb[:4] == [1, 0, 2, 0], '   leve dolu / nahoru, prave dolu / nahoru -> /mys b = %s' % bb)
        fs = pg.evaluate("[window.__fsZadost||0, window.__fsKonec||0]")
        over(fs == [1, 0], '   kliky do hry nevypinaji ani nezapinaji celou obrazovku (zadost %d, konec %d)' % tuple(fs))

        # B319: stale pohyb jako pri hrani (mys 125 Hz = udalost po 8 ms, 1,5 s): nejvys ~30 zprav / s
        mys.clear()
        t0 = time.time()
        n_ud = 0
        while time.time() - t0 < 1.5:
            pohyb(pg, 4, 1)
            n_ud += 1
            time.sleep(0.008)
        time.sleep(0.3)
        sx, sy = sum(m[1] for m in mys), sum(m[2] for m in mys)
        ocek_x, ocek_y = 4 * n_ud * 100000 / sirka, 1 * n_ud * 100000 / sirka
        mez = [round((mys[i + 1][0] - mys[i][0]) * 1000) for i in range(len(mys) - 1)]
        za_s = len(mys) / 1.5
        over(abs(sx - ocek_x) <= 2 and abs(sy - ocek_y) <= 2 and za_s <= 34 and min(mez or [99]) >= 25,
             '2b) B319: stale pohyb 1,5 s (%d udalosti mysi): %d zprav /mys = %.0f / s (B318 az 60 / s, kazda nove spojeni), '
             'nejkratsi mezera %d ms, soucet presne (%d / %d)' % (n_ud, len(mys), za_s, min(mez or [0]), sx, sy))
        mys.clear()
        for _ in range(6):
            pohyb(pg, 5, 0)
            time.sleep(0.004)
        tlacitko(pg, 'mousedown', 0)
        t_dolu = time.time()
        za = None
        while time.time() - t_dolu < 0.5:
            if any(m[3] == 1 for m in mys):
                za = time.time() - t_dolu
                break
            time.sleep(0.002)
        tlacitko(pg, 'mouseup', 0)
        time.sleep(0.2)
        over(za is not None and za < 0.06,
             '2b) tlacitko uprostred pohybu jde hned (zprava s b=1 za %.0f ms, nemusi cekat na dalsich 33 ms)' % ((za or 9) * 1000))
        # drzene tlacitko bez pohybu: "jsem tu"
        mys.clear()
        tlacitko(pg, 'mousedown', 0)
        time.sleep(3.2)
        drzi = [m for m in mys if m[3] == 1]
        tlacitko(pg, 'mouseup', 0)
        time.sleep(0.2)
        mezery = [round(drzi[i + 1][0] - drzi[i][0], 2) for i in range(len(drzi) - 1)]
        over(len(drzi) >= 3 and max(mezery or [9]) < 1.5,
             '   leve drzene 3,2 s bez pohybu: %d zprav s b=1 (mezery %s s) - C++ (hlida 2,5 s) tlacitko nepusti' % (len(drzi), mezery))

        # panel OBRAZ se pri hre neukazuje
        time.sleep(2.8)
        for _ in range(5):
            pohyb(pg, 3, 3)
            time.sleep(0.02)
        op = pg.evaluate("document.getElementById('pan').style.opacity")
        over(op == '0', '   pohyb mysi pri hre: panel OBRAZ (jas, kontrast) se neukazuje (opacity %s)' % op)

        # Esc = pusteni
        mys.clear()
        tlacitko(pg, 'mousedown', 0)
        time.sleep(0.1)
        pg.evaluate("document.exitPointerLock()")
        time.sleep(0.3)
        over(pg.evaluate("window.napPm.lock") is False and mys and mys[-1][3] == 0,
             '   Esc (pusteni zamku) pri drzenem tlacitku: /mys b=0 (tlacitko ve hre pusteno), mys pustena')
        h = napoveda(pg)
        over(h.startswith('MYS PS1 PUSTENA'), '   napoveda: "%s"' % h)
        over(any('B318 MYS PC: mys pustena' in m for _, m in log) and any('B318 MYS PC: mys zachycena' in m for _, m in log),
             '   do logu appky: "B318 MYS PC: mys zachycena (zamek ukazatele)" a "... mys pustena"')

        # znovu zachytit, pak mys na telefonu vypnuta -> zamek se uvolni sam
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        stav['ps1mys'] = '0'
        time.sleep(1.6)
        z = pg.evaluate("[window.napPm.lock, window.__zamekKonec||0, window.napPs1Mys]")
        over(z == [False, 2, False], '   mys na telefonu vypnuta (/status ps1mys=0): zamek ukazatele uvolnen sam (%s)' % z)
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        z = pg.evaluate("[window.__zamekZadost||0, window.__fsZadost||0]")
        over(z == [2, 3], '   bez mysi PS1: klik zase prepina celou obrazovku (zadosti o celou obrazovku %d), mys se nezachyti (%d)' % (z[1], z[0]))
        pohyb(pg, 50, 50)
        time.sleep(0.2)
        over(not napoveda(pg) or 'KLIKNI' not in napoveda(pg), '   bez mysi PS1 zadna napoveda mysi')
        over(not stare_cesty(dotazy), '   za celou dobu ani jednou stara cesta (MJPEG / JPEG / H.264) - Java nemusela nic snimat')
        over(not chybyJs, '   zadna chyba JavaScriptu (%s)' % chybyJs[:2])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # ===== 1b) B317: stara cesta hned po nacteni (proto "TV Java") =====
        stav = {'proud': 'ps1', 'ps1': '1', 'ps1mys': '1', 'status': True, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, STARA, stav)
        time.sleep(1.5)
        st = stare_cesty(dotazy)
        over(st and st[0][0] < 0.5, '1b) stranka B317: MJPEG z Javy otevrena hned po nacteni (za %.2f s) - odtud "TV Java" v logu menu'
             % (st[0][0] if st else -1))
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        z = pg.evaluate("[window.__zamekZadost||0, window.__fsZadost||0]")
        over(z == [0, 1], '1b) B317: klik jen prepnul celou obrazovku, mys pocitace nesla do hry vubec (zamek %d)' % z[0])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # ===== 3) obrazovka bez proudu (DJ, MP3): stara cesta hned po /status =====
        stav = {'proud': '0', 'ps1': '0', 'ps1mys': '0', 'status': True, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, CZ, stav)
        time.sleep(2.0)
        st = stare_cesty(dotazy)
        stp = [d for d in dotazy if d[1] == '/status']
        over(st and stp and st[0][0] < 0.8 and st[0][0] >= stp[0][0],
             '3) obrazovka bez proudu z C++: MJPEG az po prvnim /status (status %.2f s, MJPEG %.2f s)' % (stp[0][0] if stp else -1, st[0][0] if st else -1))
        over(not chybyJs, '   zadna chyba JavaScriptu (%s)' % chybyJs[:2])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()
        stav = {'proud': '0', 'ps1': '0', 'ps1mys': '0', 'status': False, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, CZ, stav)
        time.sleep(1.2)
        st = stare_cesty(dotazy)
        t_str = [d[0] for d in dotazy if d[1] == '/'][0]
        over(st and st[0][0] - t_str < 0.6, '3) /status vrati chybu (500): hned MJPEG (%.2f s po nacteni)' % (st[0][0] - t_str if st else -1))
        stav['konec'] = True
        ctx.close()
        srv.shutdown()
        stav = {'proud': '0', 'ps1': '0', 'ps1mys': '0', 'status': 'visi', 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, CZ, stav)
        time.sleep(3.5)
        st = stare_cesty(dotazy)
        t_str = [d[0] for d in dotazy if d[1] == '/'][0]
        over(st and 2.3 <= st[0][0] - t_str <= 3.0, '3) /status neodpovida vubec (zaseknuty server): MJPEG po 2,5 s jako drive (%.2f s po nacteni)'
             % (st[0][0] - t_str if st else -1))
        over(not chybyJs, '   zadna chyba JavaScriptu (%s)' % chybyJs[:2])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # prohlizec zamek odmitne
        stav = {'proud': 'ps1', 'ps1': '1', 'ps1mys': '1', 'status': True, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, CZ, stav)
        time.sleep(1.5)
        pg.evaluate("window.__zamekOdmitni=1")
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        h = napoveda(pg)
        over(h.startswith('PROHLIZEC NEDOVOLIL ZACHYTIT MYS') and pg.evaluate("window.napPm.lock") is False and not mys,
             '2) prohlizec zamek odmitne: "%s", do hry nic' % h)
        over(not chybyJs, '   zadna chyba JavaScriptu (%s)' % chybyJs[:2])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # ===== 4) anglicky =====
        stav = {'proud': 'ps1', 'ps1': '1', 'ps1mys': '1', 'status': True, 'konec': False}
        srv, ctx, pg, dotazy, mys, log, chybyJs = otevri(b, EN, stav)
        time.sleep(1.8)
        h1 = napoveda(pg)
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        h2 = napoveda(pg)
        pg.evaluate("document.exitPointerLock()")
        time.sleep(0.3)
        h3 = napoveda(pg)
        pg.evaluate("window.__zamekOdmitni=1")
        pg.mouse.click(640, 360)
        time.sleep(0.3)
        h4 = napoveda(pg)
        tisk('   anglicky: "%s" | "%s" | "%s" | "%s"' % (h1, h2, h3, h4))
        vse = ['MYS PS1: KLIKNI DO OBRAZU - KURZOR HRY OVLADA MYS POCITACE (ESC = PUSTIT)',
               'MYS PS1 ZACHYCENA - POHYB A TLACITKA JDOU DO HRY (ESC = PUSTIT)', 'MYS PS1 PUSTENA - KLIKNI DO OBRAZU A ZASE OVLADAS HRU',
               'MYS PS1 JE NA TELEFONU VYPNUTA - ZAPNI JI TLACITKEM MYS', 'PROHLIZEC NEDOVOLIL ZACHYTIT MYS - KLIKNI DO OBRAZU ZNOVU',
               'TENHLE PROHLIZEC NEUMI ZACHYTIT MYS']
        nalez = [s for s in vse if s in CZ]
        prel = pg.evaluate("(v)=>v.map(function(s){return NapJazyk.tr(s);})", vse)
        over(len(nalez) == len(vse) and all(a != c and not any(w in c for w in ('KLIKNI', 'OBRAZU', 'PUSTIT', 'ZACHYT', 'TLACITK')) for a, c in zip(vse, prel)),
             '4) anglicky: vsech %d hlasek mysi ze stranky ve slovniku en_tv.js (napr. "%s")' % (len(nalez), prel[3]))
        cz_slova = ('KLIKNI', 'OBRAZU', 'MYS ', 'PUSTIT', 'ZACHYCENA', 'PROHLIZEC', 'NEDOVOLIL')
        over(h1.startswith('PS1 MOUSE: CLICK INTO THE PICTURE') and h2.startswith('PS1 MOUSE CAPTURED') and
             h3.startswith('PS1 MOUSE RELEASED') and h4.startswith('THE BROWSER DID NOT ALLOW') and
             not any(s in h1 + h2 + h3 + h4 for s in cz_slova),
             '4) anglicky: napovedy mysi prelozene, zadna cestina')
        over(not chybyJs, '   zadna chyba JavaScriptu (%s)' % chybyJs[:2])
        stav['konec'] = True
        ctx.close()
        srv.shutdown()
        b.close()

    # ===== 4) cesky = B318 + jen B319 =====
    zmeny = [
        ("false,jpg:0,jzah:0,jl:0,jzl:0,nF:0", "false,jpg:0,jzah:0,jl:0,jzl:0,kop:0,kopl:0,pol:0,poll:0,kc:null,nF:0"),
        ("fetch('/'+d+'.stream?'+Date.now()", "fetch('/'+d+'.stream?v=319&t='+Date.now()"),
        ("h=dv.getUint16(o+6,true),dl=dv.getUint32(o+9,true);o+=13;if(o+dl>p.length)return -1;if(w>0&&dl>0)apJpeg(p.slice(o,o+dl),x,y,w,h);return o+dl;}",
         "h=dv.getUint16(o+6,true),kv=p[o+8],dl=dv.getUint32(o+9,true);o+=13;if(o+dl>p.length)return -1;"
         "if(kv===255){if(dl!==4||x+w>ap.W||y+h>ap.H)return -1;var sx=dv.getUint16(o,true),sy=dv.getUint16(o+2,true);if(sx+w>ap.W||sy+h>ap.H)return -1;"
         "ap.jq.push({k:1,sx:sx,sy:sy,x:x,y:y,w:w,h:h});apJdalsi();}"
         "else if(w>0&&dl>0)apJpeg(p.slice(o,o+dl),x,y,w,h,(kv&128)?1:0);return o+dl;}"),
        ("function apJpeg(j,x,y,w,h){var q=ap.jq;for(var i=q.length-1;i>=0;i--){var f=q[i];if(f.x>=x",
         "function apJpeg(j,x,y,w,h,pol){var q=ap.jq;for(var i=q.length-1;i>=0;i--){var f=q[i];if(f.k)break;if(f.x>=x"),
        ("q.push({j:j,x:x,y:y,w:w,h:h});apJdalsi();}function apJdalsi(){if(ap.jbusy||!ap.jq.length)return;",
         "q.push({j:j,x:x,y:y,w:w,h:h,p:pol});apJdalsi();}"
         "function apKopie(c){try{var t=ap.kc;if(!t)t=ap.kc=document.createElement('canvas');if(t.width<c.w)t.width=c.w;if(t.height<c.h)t.height=c.h;"
         "var tx=t.getContext('2d');tx.clearRect(0,0,c.w,c.h);tx.drawImage(ap.cv,c.sx,c.sy,c.w,c.h,0,0,c.w,c.h);"
         "ap.cx.drawImage(t,0,0,c.w,c.h,c.x,c.y,c.w,c.h);ap.kop++;}catch(e){ap.chyb++;}}"
         "function apJdalsi(){while(!ap.jbusy&&ap.jq.length&&ap.jq[0].k)apKopie(ap.jq.shift());if(ap.jbusy||!ap.jq.length)return;"),
        ("if(img){try{ap.cx.drawImage(img,f.x,f.y);}catch(e){}",
         "if(img){try{if(f.p){var cx=ap.cx;cx.imageSmoothingEnabled=false;cx.webkitImageSmoothingEnabled=false;cx.mozImageSmoothingEnabled=false;"
         "cx.drawImage(img,f.x,f.y,f.w,f.h);ap.pol++;}else ap.cx.drawImage(img,f.x,f.y);}catch(e){}"),
        ("(prekrytych zahozeno '+(ap.jzah-ap.jzl)+')':'')",
         "(prekrytych zahozeno '+(ap.jzah-ap.jzl)+')'+(ap.kop>ap.kopl||ap.pol>ap.poll?', B319: kopii (posun obrazu) '"
         "+(Math.round((ap.kop-ap.kopl)/s*10)/10)+'/s, JPEG v polovicnim rozliseni '+(ap.pol-ap.poll):''):'')"),
        ("ap.jl=ap.jpg;ap.jzl=ap.jzah;ap.zahSl=", "ap.jl=ap.jpg;ap.jzl=ap.jzah;ap.kopl=ap.kop;ap.poll=ap.pol;ap.zahSl="),
        ("leti:false,znovu:false,posl:0,", "leti:false,znovu:false,tl:false,posl:0,"),
        ("window.napPm=pm;var NAPOVEDA=", "window.napPm=pm;var MEZERA=33;var NAPOVEDA="),
        ("function posli(hned){if(pm.leti){pm.znovu=true;return;}", "function posli(hned){if(pm.leti){pm.znovu=true;if(hned)pm.tl=true;return;}"),
        ("if(!dx&&!dy&&!hned)return;pm.dx-=dx;", "if(!dx&&!dy&&!hned)return;if(!hned&&Date.now()-pm.posl<MEZERA){planuj();return;}pm.dx-=dx;"),
        ("if(pm.znovu){pm.znovu=false;posli(true);}};", "if(pm.znovu){pm.znovu=false;var t=pm.tl;pm.tl=false;if(t)posli(true);else planuj();}};"),
        ("x.onerror=x.ontimeout=function(){pm.leti=false;if(pm.znovu){pm.znovu=false;setTimeout(",
         "x.onerror=x.ontimeout=function(){pm.leti=false;if(pm.znovu){pm.znovu=false;pm.tl=false;setTimeout("),
        ("function planuj(){if(!pm.t)pm.t=setTimeout(function(){pm.t=0;posli(false);},16);}",
         "function planuj(){if(pm.t)return;var d=MEZERA-(Date.now()-pm.posl);pm.t=setTimeout(function(){pm.t=0;posli(false);},d>1?d:1);}"),
    ]
    ocek = STARA318
    nalez = 0
    for a, c in zmeny:
        if ocek.count(a) == 1:
            nalez += 1
        else:
            tisk('   (zmena nenalezena presne jednou: %s)' % a[:80])
        ocek = ocek.replace(a, c, 1)
    over(nalez == len(zmeny) and ocek == CZ,
         '4) cesky: stranka WEB TV = B318 + jen B319 (%d zmen: v=319 v adrese proudu, kopie a JPEG v polovicnim rozliseni, log, mys nejvys po 33 ms)'
         % len(zmeny))

    tisk('VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb))
    with open(os.path.join(ZDE, 'VYSTUP_test_b319_tv_mys.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
