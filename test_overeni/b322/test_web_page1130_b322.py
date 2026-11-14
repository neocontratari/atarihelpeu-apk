#!/usr/bin/env python3
"""B322: stranka atarihelp.eu/?page_id=1130 se ZIVYM MENU Z APPKY (Chromium + SwiftShader WebGL).

Stranka (html_kod_www_stranky_B322.txt, u Reneho) nacita web/menu/vystup/nap_menu_web.js (ten samy C++
jako hlavni menu appky, WebAssembly) z GitHubu (raw.githubusercontent.com: text/plain + nosniff jako GitHub,
stranka overi SHA-256 a spusti pres Blob) nebo z kopie na atarihelp.eu (integrity). V testu se obe adresy
obslouzi ze souboru v tomhle repozitari.
  Z1 4 panely WEB / ATARI / SEGA / PS1 kresli zive menu (WebGL), GIFy se nestahuji
  Z2 kazdy panel stoji na sve karte (barva karty v obraze: web tyrkys, atari zlata, sega modra, ps1 fialova)
  Z3 obraz se hybe (3D model se toci) - dva snimky za sebou se lisi
  Z4 tazeni prstem do strany = karusel jede (jina karta), Z5 klepnuti na kartu ATARI = okno kodu (stahnout)
  Z6 klepnuti na kartu WEB = atarihelp.eu
  Z7 GitHub nejde -> kopie na atarihelp.eu/wp-content/uploads/nap/; Z7b GitHub posle jiny soubor -> nespusti se;
  Z7c kopie na webu je jina -> prohlizec ji nespusti (integrity) -> GIFy; Z8 nic nejde -> GIFy
  Z9 prohlizec bez WebGL -> GIFy (soubor se ani nestahuje)
  Z10 odpocet a okno kodu jako v B321 (W1-W6), zadna chyba JavaScriptu, v kodu zadny pevny klic
Pouziti: python3 test_web_page1130_b322.py <html stranky> [nap_menu_web.js]  -> VYSTUP_test_web_page1130_b322.txt
"""
import datetime as dt
import os
import re
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
HTML = open(sys.argv[1], encoding='utf-8').read()
JS = open(sys.argv[2] if len(sys.argv) > 2 else os.path.join(ZDE, '..', '..', 'web', 'menu', 'vystup', 'nap_menu_web.js'), 'rb').read()
GIF = bytes.fromhex('47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b')
CET = dt.timezone(dt.timedelta(hours=1))
vystup = []


def kontrola(nazev, ok, detail=''):
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '   -> ' + str(detail)[:300]))
    print(vystup[-1], flush=True)


ZLY = JS + b'\nwindow.__zlo = 1;\n'


def stranka(prohl, github=True, uploads=True, webgl=True, chyby=None, gify=None, navstivene=None, jazyk='cs-CZ'):
    ctx = prohl.new_context(viewport={'width': 1200, 'height': 1000}, locale=jazyk)

    def obrazek(r):
        if gify is not None and r.request.url.endswith('.gif'):
            gify.append(r.request.url)
        if r.request.url.endswith('/nap_menu_web.js'):
            if uploads:
                r.fulfill(status=200, content_type='application/javascript', body=ZLY if uploads == 'zly' else JS,
                          headers={'Access-Control-Allow-Origin': '*'})
            else:
                r.fulfill(status=404, body='')
            return
        if r.request.resource_type == 'document':
            if '?page_id=1130' in r.request.url:     # stranka sama - jako na webu (https = bezpecny kontext)
                r.fulfill(status=200, content_type='text/html; charset=utf-8', body=('<!doctype html><html><head><meta charset="utf-8"></head><body>'
                                                                                    + HTML + '</body></html>').encode('utf-8'))
                return
            if navstivene is not None:
                navstivene.append(r.request.url)
            r.fulfill(status=200, content_type='text/html', body='<p>atarihelp.eu</p>')
            return
        r.fulfill(status=200, content_type='image/gif', body=GIF)
    ctx.route('https://atarihelp.eu/**', obrazek)
    def gh(r):
        if not github:
            r.fulfill(status=503, body='')
        else:   # jako GitHub: text/plain + nosniff (primo jako <script> by nesel) + CORS
            r.fulfill(status=200, content_type='text/plain; charset=utf-8', body=ZLY if github == 'zly' else JS,
                      headers={'Access-Control-Allow-Origin': '*', 'X-Content-Type-Options': 'nosniff'})
    ctx.route('https://raw.githubusercontent.com/**', gh)
    if not webgl:
        ctx.add_init_script("HTMLCanvasElement.prototype.getContext = (function (g) { return function (t) {"
                            " return /webgl/.test(String(t)) ? null : g.apply(this, arguments); }; })(HTMLCanvasElement.prototype.getContext);")
    s = ctx.new_page()
    if chyby is not None:
        s.on('pageerror', lambda e: chyby.append(str(e)))
    s.goto('https://atarihelp.eu/?page_id=1130', wait_until='load')
    return ctx, s


def zive(s):
    return s.evaluate("Array.prototype.map.call(document.querySelectorAll('.game-box[data-karta]'), function (b) {"
                      " return b.getAttribute('data-karta') + ':' + (b.className.indexOf('zive') >= 0 ? 'zive' : 'gif'); })")


def snimek_panelu(s, i):
    """snimek obrazovky panelu (WebGL bez preserveDrawingBuffer se z JavaScriptu cist neda)"""
    import cv2, numpy as np
    png = s.locator('#napMenu%d' % i).screenshot()
    return cv2.imdecode(np.frombuffer(png, np.uint8), cv2.IMREAD_COLOR)


def barva_panelu(s, i):
    """prumerna barva stredu karty [R, G, B]"""
    img = snimek_panelu(s, i)
    h, w = img.shape[:2]
    b, g, r = img[int(h * 0.25):int(h * 0.65), int(w * 0.2):int(w * 0.8)].reshape(-1, 3).mean(axis=0)
    return [int(r), int(g), int(b)]


def otisk(s, i):
    import cv2
    return cv2.resize(snimek_panelu(s, i), (64, 140)).astype(int)


def cekej_zive(s, n=4, ms=20000):
    for _ in range(ms // 250):
        if sum(1 for z in zive(s) if z.endswith(':zive')) >= n and s.evaluate("!!document.getElementById('napMenu3')"):
            break
        s.wait_for_timeout(250)
    s.wait_for_timeout(1500)


def main():
    chyby = []
    with sync_playwright() as p:
        prohl = p.chromium.launch(args=['--enable-unsafe-swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'])

        gify = []
        ctx, s = stranka(prohl, chyby=chyby, gify=gify)
        cekej_zive(s)
        z = zive(s)
        zdroj = s.evaluate("Array.prototype.map.call(document.querySelectorAll('script[src]'), function (x) { return x.src.slice(0, 5); })")
        kontrola('Z1 4 panely kresli zive menu z appky (WebAssembly + WebGL) z GitHubu (overeno SHA-256, Blob), zadny GIF',
                 z == ['web:zive', 'atari:zive', 'sega:zive', 'ps1:zive'] and not gify and 'blob:' in zdroj and s.evaluate('window.isSecureContext'), (z, gify, zdroj))
        b = [barva_panelu(s, i) for i in range(4)]
        ok2 = b[0] and b[0][1] > b[0][0] and b[1][0] > b[1][2] and b[2][2] > b[2][0] and b[3][2] > b[3][1]
        kontrola('Z2 panely stoji na svych kartach (web tyrkys, atari zlata, sega modra, ps1 fialova)', ok2, b)
        o1 = otisk(s, 1)
        s.wait_for_timeout(700)
        kontrola('Z3 obraz zije (Atari 130XE se toci - dva snimky se lisi)', abs(o1 - otisk(s, 1)).mean() > 0.5, abs(o1 - otisk(s, 1)).mean())
        s.screenshot(path=os.path.join(ZDE, 'snimek_web_b322.png'))   # pro kontrolu (neposila se)

        # Z4 tazeni prstem: panel SEGA doleva -> karusel na PS1
        s.locator('#napMenu2').scroll_into_view_if_needed()
        s.wait_for_timeout(300)
        box = s.locator('#napMenu2').bounding_box()
        x0, y0 = box['x'] + box['width'] * 0.75, box['y'] + box['height'] * 0.45
        pred = otisk(s, 2)
        s.mouse.move(x0, y0)
        s.mouse.down()
        for k in range(1, 11):
            s.mouse.move(x0 - box['width'] * 0.05 * k, y0)
            s.wait_for_timeout(16)
        s.mouse.up()
        s.wait_for_timeout(1200)
        b2 = barva_panelu(s, 2)
        kontrola('Z4 tazeni do strany: karusel jede na dalsi kartu (SEGA -> PS1, fialova)', b2 and b2[0] > b[2][0] and abs(pred - otisk(s, 2)).mean() > 3, (b[2], b2))

        # Z5 klepnuti na kartu ATARI = okno kodu
        s.locator('#napMenu1').scroll_into_view_if_needed()
        s.wait_for_timeout(300)
        box = s.locator('#napMenu1').bounding_box()
        s.mouse.click(box['x'] + box['width'] * 0.5, box['y'] + box['height'] * 0.42)
        s.wait_for_timeout(900)
        kontrola('Z5 klepnuti na kartu ATARI = okno osobniho kodu (stahnout aplikaci)', s.is_visible('#promptModal'), s.is_visible('#promptModal'))
        s.click('text=Zrušit / Cancel')
        ctx.close()

        nav = []
        ctx, s = stranka(prohl, chyby=chyby, navstivene=nav)
        cekej_zive(s)
        s.locator('#napMenu0').scroll_into_view_if_needed()
        s.wait_for_timeout(300)
        box = s.locator('#napMenu0').bounding_box()
        with s.expect_navigation():
            s.mouse.click(box['x'] + box['width'] * 0.5, box['y'] + box['height'] * 0.42)
        kontrola('Z6 klepnuti na kartu ATARIHELP.EU = web atarihelp.eu', nav[-1:] == ['https://atarihelp.eu/'], nav)
        ctx.close()

        ctx, s = stranka(prohl, github=False, chyby=chyby)
        cekej_zive(s)
        zdroj = s.evaluate("Array.prototype.map.call(document.querySelectorAll('script[src]'), function (x) { return x.src; })")
        kontrola('Z7 GitHub nejde -> nacte se kopie z atarihelp.eu/wp-content/uploads/nap/ (integrity)', zive(s) == ['web:zive', 'atari:zive', 'sega:zive', 'ps1:zive']
                 and 'https://atarihelp.eu/wp-content/uploads/nap/nap_menu_web.js' in zdroj, (zive(s), zdroj))
        ctx.close()

        gify = []
        ctx, s = stranka(prohl, github='zly', uploads=False, chyby=chyby, gify=gify)
        s.wait_for_timeout(3000)
        kontrola('Z7b GitHub posle jiny soubor (otisk SHA-256 nesedi) -> nespusti se, GIFy', zive(s) == ['web:gif', 'atari:gif', 'sega:gif', 'ps1:gif']
                 and s.evaluate('window.__zlo === undefined'), (zive(s), s.evaluate('window.__zlo')))
        ctx.close()

        gify = []
        ctx, s = stranka(prohl, github=False, uploads='zly', chyby=None, gify=gify)
        s.wait_for_timeout(3000)
        kontrola('Z7c kopie na webu je jina -> prohlizec ji nespusti (integrity), GIFy', zive(s) == ['web:gif', 'atari:gif', 'sega:gif', 'ps1:gif']
                 and s.evaluate('window.__zlo === undefined'), (zive(s), s.evaluate('window.__zlo')))
        ctx.close()

        gify = []
        ctx, s = stranka(prohl, github=False, uploads=False, chyby=chyby, gify=gify)
        s.wait_for_timeout(2500)
        kontrola('Z8 zadna adresa nejde -> puvodni GIFy', zive(s) == ['web:gif', 'atari:gif', 'sega:gif', 'ps1:gif'] and len(gify) == 4
                 and s.is_visible('#screenAtari') and s.is_visible('.game-box[data-karta=atari] .click-zone'), (zive(s), gify))
        ctx.close()

        gify, nacteno = [], []
        ctx, s = stranka(prohl, webgl=False, chyby=chyby, gify=gify)
        s.on('request', lambda r: nacteno.append(r.url) if 'nap_menu_web' in r.url else None)
        s.wait_for_timeout(1500)
        kontrola('Z9 prohlizec bez WebGL -> GIFy, 2 MB soubor se ani nestahuje', zive(s) == ['web:gif', 'atari:gif', 'sega:gif', 'ps1:gif'] and len(gify) == 4 and not nacteno,
                 (zive(s), gify, nacteno))
        ctx.close()

        ctx, s = stranka(prohl, chyby=chyby, jazyk='en-US')
        cekej_zive(s)
        kontrola('Z9b anglicky prohlizec: menu anglicky (zive)', zive(s) == ['web:zive', 'atari:zive', 'sega:zive', 'ps1:zive'])
        s.screenshot(path=os.path.join(ZDE, 'snimek_web_b322_en.png'))
        ctx.close()

        # Z10 odpocet a okno kodu (jako B321)
        ctx = prohl.new_context(timezone_id='Europe/Prague', viewport={'width': 412, 'height': 900})
        ctx.route(re.compile(r'^https://'), lambda r: r.fulfill(status=200, content_type='text/html; charset=utf-8', body=('<!doctype html><html><head><meta charset="utf-8"></head><body>'
                  + HTML + '</body></html>').encode('utf-8')) if '?page_id=1130' in r.request.url else r.fulfill(status=200, content_type='image/gif', body=GIF))
        s = ctx.new_page()
        s.on('pageerror', lambda e: chyby.append(str(e)))
        s.clock.install(time=dt.datetime(2026, 11, 14, 15, 0, 0, tzinfo=CET))
        s.goto('https://atarihelp.eu/?page_id=1130', wait_until='load')
        s.wait_for_timeout(300)
        c = [s.inner_text('#' + i) for i in ('days', 'hours')]
        kontrola('Z10 odpocet 14. 11. 15:00 CET -> 6 dni 3 hodiny', c[0] == '06' and c[1] in ('02', '03'), c)
        s.click('#mainDownloadBtn')
        s.type('#codeInput', 'nap 7k3q m9xd')
        kontrola('Z10 okno kodu: NAP-7K3Q-M9XD', s.input_value('#codeInput') == 'NAP-7K3Q-M9XD', s.input_value('#codeInput'))
        ctx.close()
        prohl.close()
    kontrola('zadna chyba JavaScriptu', not chyby, chyby)
    kontrola('v kodu stranky neni zadne porovnani s pevnym klicem (ani validKeys)', not re.search(r"==\s*['\"][A-Z0-9][A-Z0-9_-]{5,}['\"]", HTML)
             and 'validKeys' not in HTML)
    ok = sum(1 for r in vystup if r.startswith('OK'))
    vystup.append('')
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (ok, len(vystup) - 1 - ok))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_web_page1130_b322.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    return 0 if ok == len(vystup) - 2 else 1


if __name__ == '__main__':
    sys.exit(main())
