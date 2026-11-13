#!/usr/bin/env python3
"""B320: STRANKA WEB TV SE PO AKTUALIZACI APPKY OBNOVI SAMA (Chromium, stranka presne z MainActivity).

Rene po B319: novou stranku (kopie pri posunu obrazu) mel az po F5 - prohlizec drzel stranku ze stare
appky. Ted: Java do stranky dosadi verzi appky (window.NAP_VERZE = versionCode - misto "0/*NAP_VERZE*/")
a /status hlasi " verze=<versionCode>". Kdyz se lisi (appka se mezitim aktualizovala a server je zase
nahore), stranka se sama obnovi - nejvys jednou za 30 s (sessionStorage), at se nikdy necykli.

Server = co dela Java (MainActivity): /, /status (s verze= jako napTvVerze()), /ps1.stream, /clientlog.
Kontroly:
  1) stejna verze: stranka se neobnovuje
  2) appka aktualizovana (status 363 -> 364): stranka se do ~2 s obnovi sama, v logu appky
     "B320 STRANKA: appka je nove verze (363 -> 364) - stranka se obnovi", po obnoveni NAP_VERZE 364,
     proud PS1 zase bezi (/ps1.stream?v=319), dalsi obnoveni uz ne
  3) server porad dava starou stranku (nemelo by nastat): obnoveni jen jednou za 30 s - zadne cykleni
  4) appka bez verze v /status (starsi) a stranka bez cisla (0 - testy, cteni ze zdrojaku): nic
  5) Java: /status ma " verze=" + napTvVerze(), stranka "0/*NAP_VERZE*/" se nahradi pred odeslanim
  6) zadna chyba JavaScriptu
Pouziti: python3 test_b320_tv_verze.py      -> VYSTUP_test_b320_tv_verze.txt
"""
import http.server
import os
import sys
import threading
import time
import urllib.parse

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b318'))
import test_b318_tv_mys as B318                # noqa: E402  (stranka_z_javy, paket3)

JAVA = open(os.path.join(B318.KOREN, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'), encoding='utf-8').read()
CZ = B318.stranka_z_javy(JAVA)

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


def stranka(verze):
    """jako Java napTvWebWriteHtml: body.replace("0/*NAP_VERZE*/", versionCode)"""
    return CZ.replace('0/*NAP_VERZE*/', str(verze)) if verze is not None else CZ


def server(stav, dotazy, log):
    T0 = time.time()

    class S(http.server.BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.0'

        def log_message(self, *a):
            pass

        def posli(self, kod, typ, data):
            try:
                self.send_response(kod)
                self.send_header('Content-Type', typ)
                self.send_header('Content-Length', str(len(data)))
                self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
                self.end_headers()
                self.wfile.write(data)
            except Exception:
                pass

        def do_GET(self):
            p = self.path.split('?')[0]
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            dotazy.append((round(time.time() - T0, 2), p, self.path))
            if p in ('/', '/index.html'):
                return self.posli(200, 'text/html; charset=utf-8', stranka(stav['stranka']).encode('utf-8'))
            if p == '/status':
                v = (' verze=%d' % stav['verze']) if stav['verze'] is not None else ''
                t = 'running=true atari=0 atariCpp=0 atariProud=0 proud=ps1 sega=0 ps1=1 ps1mys=0 menu=0 seq=1 h264Seq=0%s url=x\n' % v
                return self.posli(200, 'text/plain', t.encode())
            if p == '/clientlog':
                log.append((round(time.time() - T0, 2), q.get('m', [''])[0]))
                return self.posli(200, 'text/plain', b'ok')
            if p == '/ps1.stream':
                try:
                    self.wfile.write(b'HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n')
                    self.wfile.flush()
                    import struct
                    while not stav['konec']:
                        pk = B318.paket3(368, 240)
                        self.wfile.write(struct.pack('<I', len(pk)) + pk)
                        self.wfile.flush()
                        time.sleep(1 / 30)
                except Exception:
                    pass
                return
            if p in ('/quality',):
                return self.posli(200, 'text/plain', b'tier=2')
            if p in ('/klavesa', '/klavesy', '/audio.raw', '/mys'):
                return self.posli(200, 'text/plain', b'')
            if p == '/frame.jpg':
                return self.posli(503, 'text/plain', b'frame not ready\n')
            return self.posli(404, 'text/plain', b'')

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), S)
    srv.daemon_threads = True
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


def nacteni(dotazy):
    return len([d for d in dotazy if d[1] in ('/', '/index.html')])


def main():
    chybyJs = []
    # 5) Java
    over('+ " verze=" + napTvVerze()' in JAVA and 'body = body.replace("0/*NAP_VERZE*/", String.valueOf(napTvVerze()));' in JAVA
         and "<script>window.NAP_VERZE=0/*NAP_VERZE*/;</script>" in CZ and 'getPackageInfo(getPackageName(), 0).versionCode' in JAVA,
         '5) Java: /status " verze=" + napTvVerze() (versionCode), stranka window.NAP_VERZE=0/*NAP_VERZE*/ - cislo dosadi Java pred odeslanim')
    with sync_playwright() as p:
        b = p.chromium.launch()

        def otevri(stav):
            dotazy, log = [], []
            srv = server(stav, dotazy, log)
            ctx = b.new_context(viewport={'width': 1280, 'height': 720})
            ctx.add_init_script(B318.NAHRADY)
            pg = ctx.new_page()
            pg.on('pageerror', lambda e: chybyJs.append(str(e)[:200]))
            pg.goto('http://127.0.0.1:%d/' % srv.server_address[1])
            return srv, ctx, pg, dotazy, log

        # 1) + 2) stejna verze, pak aktualizace appky
        stav = {'stranka': 363, 'verze': 363, 'konec': False}
        srv, ctx, pg, dotazy, log = otevri(stav)
        pg.wait_for_timeout(4000)
        n1 = nacteni(dotazy)
        v1 = pg.evaluate('() => window.NAP_VERZE')
        over(n1 == 1 and v1 == 363, '1) stejna verze appky (363): stranka se neobnovuje (nacteni %d, NAP_VERZE %s)' % (n1, v1))
        stav['stranka'] = 364                    # nova appka: nova stranka a nova verze v /status
        stav['verze'] = 364
        t0 = time.time()
        obnova = None
        while time.time() - t0 < 6:
            pg.wait_for_timeout(100)
            if nacteni(dotazy) >= 2 and obnova is None:
                obnova = time.time() - t0
        pg.wait_for_timeout(3000)
        v2 = pg.evaluate('() => window.NAP_VERZE')
        zprava = [m for _, m in log if m.startswith('B320 STRANKA')]
        proudy = [d for d in dotazy if d[1] == '/ps1.stream']
        posledniNacteni = max(d[0] for d in dotazy if d[1] in ('/', '/index.html'))
        proudPo = [d for d in proudy if d[0] > posledniNacteni and 'v=319' in d[2]]
        over(obnova is not None and obnova < 3.5 and nacteni(dotazy) == 2 and v2 == 364 and zprava and '(363 -> 364)' in zprava[0] and proudPo,
             '2) appka aktualizovana (/status 363 -> 364): stranka se obnovila sama za %.1f s, v logu appky "%s", NAP_VERZE %s, proud PS1 zase bezi (%s), '
             'dalsi obnoveni ne (nacteni celkem %d)' % (obnova or -1, zprava[0] if zprava else '-', v2, proudPo[0][2].split('&')[0] if proudPo else '-', nacteni(dotazy)))
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # 3) server porad dava starou stranku - zadne cykleni
        stav = {'stranka': 363, 'verze': 364, 'konec': False}
        srv, ctx, pg, dotazy, log = otevri(stav)
        pg.wait_for_timeout(8000)
        n3 = nacteni(dotazy)
        over(n3 == 2, '3) stranka i po obnoveni stara (363, /status 364): obnovi se jen jednou za 30 s, zadne cykleni (nacteni za 8 s: %d)' % n3)
        stav['konec'] = True
        ctx.close()
        srv.shutdown()

        # 4) starsi appka bez verze v /status; stranka bez cisla (0)
        stav = {'stranka': 363, 'verze': None, 'konec': False}
        srv, ctx, pg, dotazy, log = otevri(stav)
        pg.wait_for_timeout(3500)
        n4a = nacteni(dotazy)
        stav['konec'] = True
        ctx.close()
        srv.shutdown()
        stav = {'stranka': None, 'verze': 364, 'konec': False}
        srv, ctx, pg, dotazy, log = otevri(stav)
        pg.wait_for_timeout(3500)
        n4b = nacteni(dotazy)
        v4 = pg.evaluate('() => window.NAP_VERZE')
        stav['konec'] = True
        ctx.close()
        srv.shutdown()
        over(n4a == 1 and n4b == 1 and v4 == 0, '4) /status bez verze (starsi appka) ani stranka bez cisla (NAP_VERZE %s) se neobnovuje' % v4)
        b.close()
    over(not chybyJs, '6) zadna chyba JavaScriptu' + (': ' + '; '.join(chybyJs[:3]) if chybyJs else ''))
    vystup.append('')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYB' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    print(vystup[-1])
    with open(os.path.join(ZDE, 'VYSTUP_test_b320_tv_verze.txt'), 'w', encoding='utf-8') as f:
        f.write('B320 - stranka WEB TV se po aktualizaci appky obnovi sama (Chromium, stranka z MainActivity.java)\n')
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
