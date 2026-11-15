#!/usr/bin/env python3
"""B322 oprava 15. 11. 2026: stranka atarihelp.eu/?page_id=1130 TAK, JAK JI POSLE WORDPRESS (Chromium).

Co se stalo: u Reneho na webu skript stranky B322 vubec nebezel (odpocet 00:00:00:00, misto obrazku
rozbite ikonky). WordPress pousti obsah kazde stranky pres wptexturize (i blok Vlastni HTML):
v kazdem useku od "<" po nejblizsi ">" zmeni & na &#038; - i ve skriptu. B322 mela
"if (img && !img...)" mezi "c.length <= 3" a nejblizsim ">" -> "img &#038;&#038; !img" -> chyba syntaxe
-> skript nebezi vubec. B321 tam zadny && nemela, proto bezela. (wptexturize.py, wpautop.py = prepis
algoritmu WordPressu.)

Kontroly:
  L1  nova stranka: ve skriptech zadny znak "<", zadny "&", zadne zpetne lomitko, zadny prazdny radek,
      "[" nikdy pred pismenem (shortcode); ve stylu zadne "<" a "&"; v cele strance zadny prazdny radek
  L2  skripty projdou wptexturize i wptexturize + wpautop BEZ JEDINE ZMENY
  W*  nova stranka v Chromiu ve 4 podobach: surova, Gutenberg (blok Vlastni HTML = wptexturize),
      klasicky editor (wptexturize + wpautop), Gutenberg + doplnek "odlozit JavaScript" (skripty se spusti
      az po nacteni stranky, DOMContentLoaded uz neprijde): odpocet bezi, QR obrazek je, 4 zive panely,
      zadna chyba JavaScriptu, diagnostika skryta, okno kodu funguje
  W5  bez WebGL / bez souboru menu -> GIFy (jsou ve strance primo, nacteny), diagnostika skryta
  W6  chyba v CIZIM skriptu (sablona) -> stranka bezi, diagnostika se navstevnikum neukaze
  W7  ?nap_diag=1 -> nahore radek "skript stranky bezi | zive panely: 4"
  W8  nova stranka s umyslne rozbitym skriptem -> diagnostika SAMA ukaze "SKRIPT STRANKY NEBEZI" + chybu,
      GIFy a QR jsou videt i tak, odpocet "--" (WordPress z toho udela pomlcku)
  W9  rozlozeni stranky (odpocet, tlacitko, panely, GIF, QR, dialog) je surove / Gutenberg / klasicky editor
      stejne do 3 px - na PC (1200) i v telefonu (412); klasicky editor obali radky do <p> (s okraji)
  R1  (--rozbita = B322 z 14. 11.) pres Gutenberg: chyba syntaxe, odpocet 00, obrazky bez src = PRESNE to,
      co bylo u Reneho (test tu chybu umi najit)
  R2  (--funkcni = B321) pres Gutenberg bezi (u Reneho bezela)
Pouziti: python3 test_web_wordpress.py <nova stranka> [--rozbita stara.txt] [--funkcni b321.txt] [nap_menu_web.js]
  -> VYSTUP_test_web_wordpress.txt
"""
import datetime as dt
import os
import re
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, ZDE)
from wpautop import wpautop          # noqa: E402
from wptexturize import wptexturize  # noqa: E402

argv = sys.argv[1:]
def vezmi(prepinac):
    if prepinac in argv:
        i = argv.index(prepinac)
        hodnota = argv[i + 1]
        del argv[i:i + 2]
        return open(hodnota, encoding='utf-8').read()
    return None
ROZBITA = vezmi('--rozbita')
FUNKCNI = vezmi('--funkcni')
NOVA = open(argv[0], encoding='utf-8').read()
JS = open(argv[1] if len(argv) > 1 else os.path.join(ZDE, '..', '..', 'web', 'menu', 'vystup', 'nap_menu_web.js'), 'rb').read()
GIF = bytes.fromhex('47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b')
CIL = dt.datetime(2026, 11, 20, 18, 0, 0, tzinfo=dt.timezone(dt.timedelta(hours=1)))
vystup = []


def kontrola(nazev, ok, detail=''):
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '   -> ' + str(detail)[:400]))
    print(vystup[-1], flush=True)


def skripty(html):
    return re.findall(r'<script\b[^>]*>(.*?)</script>', html, re.S | re.I)


def prazdne_radky(text):
    radky = text.replace('\r\n', '\n').split('\n')
    return sum(1 for r in radky[1:-1] if r.strip() == '')


def gutenberg(html):
    return wptexturize(html)


def klasicky(html):
    return wpautop(wptexturize(html))


def odlozeny(html):
    """doplnek typu WP Rocket "Delay JavaScript": skripty se spusti az po nacteni stranky"""
    return re.sub(r'<script>', '<script type="text/nap-odlozeno">', gutenberg(html))


ODLOZ = ("<script>window.addEventListener('load', function () { setTimeout(function () {"
         " var s = document.querySelectorAll('script[type=\"text/nap-odlozeno\"]');"
         " for (var i = 0; i < s.length; i++) { var n = document.createElement('script'); n.text = s[i].text; s[i].parentNode.replaceChild(n, s[i]); }"
         " }, 1000); });</script>")


def stranka(prohl, obsah, github=True, uploads=True, webgl=True, cizi_chyba=False, diag=False, chyby=None, nacteno=None):
    ctx = prohl.new_context(viewport={'width': 1200, 'height': 1000}, locale='cs-CZ', timezone_id='Europe/Prague')
    hlava = '<script src="https://atarihelp.eu/wp-content/themes/sablona/rozbite.js"></script>' if cizi_chyba else ''
    telo = ('<!doctype html><html><head><meta charset="utf-8">' + hlava + '</head><body><div class="entry-content">'
            + obsah + '</div>' + (ODLOZ if 'text/nap-odlozeno' in obsah else '') + '</body></html>').encode('utf-8')

    def web(r):
        u = r.request.url
        if nacteno is not None:
            nacteno.append(u)
        if '?page_id=1130' in u:
            r.fulfill(status=200, content_type='text/html; charset=utf-8', body=telo)
        elif u.endswith('/rozbite.js'):
            r.fulfill(status=200, content_type='application/javascript', body='sablona.neexistuje.nic();')
        elif u.endswith('/nap_menu_web.js'):
            if uploads:
                r.fulfill(status=200, content_type='application/javascript', body=JS, headers={'Access-Control-Allow-Origin': '*'})
            else:
                r.fulfill(status=404, body='')
        elif r.request.resource_type == 'document':
            r.fulfill(status=200, content_type='text/html', body='<p>atarihelp.eu</p>')
        else:
            r.fulfill(status=200, content_type='image/gif', body=GIF)
    ctx.route('https://atarihelp.eu/**', web)

    def gh(r):
        if github:
            r.fulfill(status=200, content_type='text/plain; charset=utf-8', body=JS,
                      headers={'Access-Control-Allow-Origin': '*', 'X-Content-Type-Options': 'nosniff'})
        else:
            r.fulfill(status=503, body='')
    ctx.route('https://raw.githubusercontent.com/**', gh)
    if not webgl:
        ctx.add_init_script("HTMLCanvasElement.prototype.getContext = (function (g) { return function (t) {"
                            " return /webgl/.test(String(t)) ? null : g.apply(this, arguments); }; })(HTMLCanvasElement.prototype.getContext);")
    s = ctx.new_page()
    if chyby is not None:
        s.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:160]))
    s.goto('https://atarihelp.eu/?page_id=1130' + ('&nap_diag=1' if diag else ''), wait_until='load')
    return ctx, s


def stav(s):
    return s.evaluate("""() => {
        const t = id => { const e = document.getElementById(id); return e ? e.textContent : null; };
        const boxy = Array.from(document.querySelectorAll('.game-box'));
        return {
            odpocet: ['days', 'hours', 'minutes', 'seconds'].map(t),
            qr: (() => { const q = document.getElementById('screenQR'); return q ? (q.complete && q.naturalWidth > 0) : false; })(),
            panely: boxy.map(b => b.getAttribute('data-karta') + ':' + (b.classList.contains('zive') ? 'zive' : (b.classList.contains('ceka') ? 'ceka' : 'gif'))),
            gify: boxy.map(b => { const i = b.querySelector('img.retro-render'); return !!(i && i.getAttribute('src') && i.complete && i.naturalWidth > 0 && getComputedStyle(i).display !== 'none'); }),
            diag: (() => { const d = document.getElementById('nap1130-diag'); return d ? (getComputedStyle(d).display !== 'none' ? d.textContent : '') : null; })(),
        };
    }""")


def ukaz_vse(s):
    """posunout stranku dolu a zpet (obrazky maji loading=lazy)"""
    s.evaluate("window.scrollTo(0, document.body.scrollHeight)")
    s.wait_for_timeout(600)
    s.evaluate("window.scrollTo(0, 0)")
    s.wait_for_timeout(300)


def cekej(s, podminka, ms=25000):
    for _ in range(ms // 250):
        if podminka(stav(s)):
            return
        s.wait_for_timeout(250)


def odpocet_ok(c):
    zbyva = (CIL - dt.datetime.now(dt.timezone.utc)).total_seconds()
    if zbyva <= 0:
        return c == ['00', '00', '00', '00']
    dny = int(zbyva // 86400)
    return all(x is not None and re.fullmatch(r'\d\d+', x) for x in c) and abs(int(c[0]) - dny) <= 1


def okno_kodu(s):
    s.click('#mainDownloadBtn')
    s.type('#codeInput', 'nap 7k3q m9xd')
    ok = s.is_visible('#promptModal') and s.input_value('#codeInput') == 'NAP-7K3Q-M9XD'
    s.click('text=Zrušit / Cancel')
    return ok and not s.is_visible('#promptModal')


def main():
    # ---- L1 / L2: staticky ----
    sk = skripty(NOVA)
    st = re.findall(r'<style\b[^>]*>(.*?)</style>', NOVA, re.S)
    vady = []
    for i, x in enumerate(sk):
        for znak, popis in (('<', 'znak <'), ('&', 'znak &'), ('\\', 'zpetne lomitko')):
            if znak in x:
                vady.append('skript %d: %s' % (i, popis))
        if prazdne_radky(x):
            vady.append('skript %d: prazdny radek' % i)
        if re.search(r'\[[A-Za-z_]', x):
            vady.append('skript %d: [ pred pismenem %s' % (i, re.findall(r'\[[A-Za-z_]\w*', x)[:3]))
    for x in st:
        if '<' in x or '&' in x or prazdne_radky(x):
            vady.append('style: < nebo & nebo prazdny radek')
    if any(r.strip() == '' for r in NOVA.replace('\r\n', '\n').rstrip('\n').split('\n')):
        vady.append('prazdny radek ve strance')
    kontrola('L1 skripty bez "<", "&", zpetneho lomitka, prazdnych radku a "[pismeno"; styl i stranka bez prazdnych radku (%d skripty)' % len(sk),
             not vady and len(sk) == 2, vady)
    kontrola('L2 skripty projdou wptexturize (Gutenberg) i wptexturize + wpautop (klasicky editor) beze zmeny',
             skripty(gutenberg(NOVA)) == sk and skripty(klasicky(NOVA)) == sk)

    with sync_playwright() as p:
        prohl = p.chromium.launch(args=['--enable-unsafe-swiftshader', '--use-angle=swiftshader', '--ignore-gpu-blocklist'])
        for znacka, nazev, uprav in (('W1', 'surova', lambda h: h), ('W2', 'Gutenberg - blok Vlastni HTML (wptexturize)', gutenberg),
                                     ('W3', 'klasicky editor (wptexturize + wpautop)', klasicky),
                                     ('W4', 'Gutenberg + doplnek "odlozit JavaScript" (bez DOMContentLoaded)', odlozeny)):
            chyby, nacteno = [], []
            ctx, s = stranka(prohl, uprav(NOVA), chyby=chyby, nacteno=nacteno)
            if znacka == 'W4':
                s.wait_for_timeout(1500)
            cekej(s, lambda x: all(q.endswith(':zive') for q in x['panely']))
            s.wait_for_timeout(800)
            ukaz_vse(s)
            x = stav(s)
            gify = [u for u in nacteno if u.endswith('.gif')]
            okno = okno_kodu(s)
            kontrola('%s %s: odpocet bezi %s, QR je, 4 zive panely, okno kodu, zadna chyba JS, diagnostika skryta%s' % (
                znacka, nazev, ':'.join(str(c) for c in x['odpocet']), '' if znacka == 'W4' else ', GIFy se nestahuji'),
                odpocet_ok(x['odpocet']) and x['qr'] and x['panely'] == ['web:zive', 'atari:zive', 'sega:zive', 'ps1:zive'] and okno
                and not chyby and x['diag'] == '' and (znacka == 'W4' or not gify), (x, chyby, gify, okno))
            ctx.close()

        for nazev, kw in (('bez WebGL', {'webgl': False}), ('soubor menu nejde (GitHub ani kopie)', {'github': False, 'uploads': False})):
            chyby = []
            ctx, s = stranka(prohl, gutenberg(NOVA), chyby=chyby, **kw)
            cekej(s, lambda x: all(q.endswith(':gif') for q in x['panely']), 8000)
            ukaz_vse(s)
            s.wait_for_timeout(500)
            x = stav(s)
            kontrola('W5 Gutenberg, %s: 4 GIFy nactene a videt, odpocet, QR, diagnostika skryta' % nazev,
                     x['gify'] == [True] * 4 and x['panely'] == ['web:gif', 'atari:gif', 'sega:gif', 'ps1:gif'] and odpocet_ok(x['odpocet'])
                     and x['qr'] and x['diag'] == '' and not chyby, (x, chyby))
            ctx.close()

        chyby = []
        ctx, s = stranka(prohl, gutenberg(NOVA), cizi_chyba=True, chyby=chyby)
        cekej(s, lambda x: all(q.endswith(':zive') for q in x['panely']))
        s.wait_for_timeout(4800)
        x = stav(s)
        kontrola('W6 chyba v cizim skriptu (sablona) -> stranka bezi, navstevnik diagnostiku nevidi',
                 x['panely'] == ['web:zive', 'atari:zive', 'sega:zive', 'ps1:zive'] and x['diag'] == '' and any('sablona' in c for c in chyby), (x, chyby))
        ctx.close()

        ctx, s = stranka(prohl, gutenberg(NOVA), diag=True)
        cekej(s, lambda x: all(q.endswith(':zive') for q in x['panely']))
        cekej(s, lambda x: 'zive panely: 4' in (x['diag'] or ''), 20000)
        x = stav(s)
        kontrola('W7 ?nap_diag=1 -> nahore radek "skript stranky bezi | zive panely: 4 | ... selhalo: nic" (obnovuje se)',
                 'skript stranky bezi' in (x['diag'] or '') and 'zive panely: 4' in x['diag'] and 'selhalo: nic' in x['diag'], x['diag'])
        ctx.close()

        rozbita_nova = NOVA.replace('var spusteno = false;', 'var spusteno = false; #rozbito;')
        chyby = []
        ctx, s = stranka(prohl, gutenberg(rozbita_nova), chyby=chyby)
        cekej(s, lambda x: bool(x['diag']), 9000)
        ukaz_vse(s)
        s.wait_for_timeout(400)
        x = stav(s)
        d = x['diag'] or ''
        kontrola('W8 umyslne rozbity skript -> diagnostika sama ukaze "SKRIPT STRANKY NEBEZI" + chybu, GIFy a QR videt, odpocet "--"',
                 'SKRIPT STRANKY NEBEZI' in d and 'SyntaxError' in d and x['gify'] == [True] * 4 and x['qr'] and x['odpocet'] in (['--'] * 4, ['\u2014'] * 4),
                 (x, chyby))
        ctx.close()

        SEL = ['.countdown-container', '#mainDownloadBtn', '.game-box[data-karta=web]', '.game-box[data-karta=ps1]', '#screenAtari',
               '.qr-container', '#screenQR', '.lunar-dialog-box']
        rozdily = []
        for sirka in (1200, 412):
            mira = {}
            for druh, uprav in (('surova', lambda h: h), ('Gutenberg', gutenberg), ('klasicky', klasicky)):
                ctx = prohl.new_context(viewport={'width': sirka, 'height': 900})
                ctx.add_init_script("HTMLCanvasElement.prototype.getContext = function () { return null; };")
                telo = ('<!doctype html><html><head><meta charset="utf-8"><style>p { margin: 1em 0; }</style></head><body><div class="entry-content">'
                        + uprav(NOVA) + '</div></body></html>').encode('utf-8')
                def obsluha(telo):   # (route) - Playwright by handleru se 2 parametry predal i request
                    return lambda r: (r.fulfill(status=200, content_type='text/html; charset=utf-8', body=telo) if '?page_id=1130' in r.request.url
                                      else (r.fulfill(status=404, body='') if r.request.url.endswith('.js')
                                            else r.fulfill(status=200, content_type='image/gif', body=GIF)))
                ctx.route(re.compile(r'^https://'), obsluha(telo))
                s = ctx.new_page()
                s.goto('https://atarihelp.eu/?page_id=1130', wait_until='load')
                s.wait_for_timeout(600)
                mira[druh] = [tuple(round(v) for v in (b['x'], b['y'], b['width'], b['height'])) for b in (s.locator(x).first.bounding_box() for x in SEL)]
                ctx.close()
            for druh in ('Gutenberg', 'klasicky'):
                for i, (a, b) in enumerate(zip(mira['surova'], mira[druh])):
                    if max(abs(u - v) for u, v in zip(a, b)) > 3:
                        rozdily.append((sirka, druh, SEL[i], a, b))
        kontrola('W9 rozlozeni surova / Gutenberg / klasicky editor stejne do 3 px (PC 1200 i telefon 412, %d prvku)' % len(SEL), not rozdily, rozdily[:4])

        if ROZBITA:
            chyby = []
            ctx, s = stranka(prohl, gutenberg(ROZBITA), chyby=chyby)
            s.wait_for_timeout(3000)
            ukaz_vse(s)
            x = stav(s)
            src = s.evaluate("Array.from(document.querySelectorAll('img')).map(i => i.getAttribute('src') ? 'src' : 'BEZ src')")
            kontrola('R1 B322 z 14. 11. pres Gutenberg: chyba syntaxe, odpocet 00:00:00:00, obrazky bez src (= u Reneho na webu)',
                     any('SyntaxError' in c or 'Invalid or unexpected token' in c for c in chyby) and x['odpocet'] == ['00'] * 4
                     and src.count('BEZ src') >= 5, (x['odpocet'], src, chyby))
            ctx.close()
        if FUNKCNI:
            chyby = []
            ctx, s = stranka(prohl, gutenberg(FUNKCNI), chyby=chyby)
            s.wait_for_timeout(2000)
            x = stav(s)
            kontrola('R2 B321 pres Gutenberg bezi (odpocet, zadna chyba) - u Reneho bezela', odpocet_ok(x['odpocet']) and not chyby, (x['odpocet'], chyby))
            ctx.close()
        prohl.close()

    ok = sum(1 for r in vystup if r.startswith('OK'))
    vystup.append('')
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (ok, len(vystup) - 1 - ok))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_web_wordpress.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    return 0 if ok == len(vystup) - 2 else 1


if __name__ == '__main__':
    sys.exit(main())
