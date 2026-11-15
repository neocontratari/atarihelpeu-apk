#!/usr/bin/env python3
"""v12 (server N&P v12, 15. 11. 2026): stranka atarihelp.eu/?page_id=1130 - tlacitko REGISTRACE a odkazy z e-mailu
registrace, TAK JAK JI POSLE WORDPRESS (surova, Gutenberg = wptexturize, klasicky editor = wptexturize + wpautop,
doplnek "odlozit JavaScript"). Server N&P je tu falesny (Playwright zachyti adresu http://46.33.114.139:8080/...).

Kontroly (v kazde podobe stranky):
  G1  tlacitko "Registrace / Register" je videt pod tlacitkem Stahnout, vede na <server>/registrace
  G2  odkaz z e-mailu ...&nap_potvrdit=<32 hex> -> stranka hned otevre <server>/registrace/potvrdit?t=<token>
      (server tam ukaze tlacitko POTVRDIT - potvrzuje clovek)
  G3  odkaz z e-mailu ...&nap_kod=NAP-XXXX-XXXX -> otevre se okno kodu s vyplnenym kodem; Pokracovat ->
      <server>/aktivace?kod=NAP-XXXX-XXXX (stejne jako kdyz kod clovek napise)
  G4  podvrzene parametry (kratky token, nap_kod s HTML / spatnym tvarem) -> nic se nestane, zadna chyba JS
  G5  telefon (412 px): tlacitko cele na obrazovce, nic nepreteka do strany
Odkazy maji presne tvar, jaky posila server v12 (test_server.py S46 / S48: Odkazy + "&nap_potvrdit=" / "&nap_kod=").
Pouziti: python3 test_web_registrace.py ../../web/stranka_1130/html_kod_www_stranky_v12.txt  -> VYSTUP_test_web_registrace.txt
"""
import os
import re
import sys
import tempfile

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b322'))
from wpautop import wpautop          # noqa: E402
from wptexturize import wptexturize  # noqa: E402

STRANKA = open(sys.argv[1], encoding='utf-8').read()
SERVER = re.search(r"var NAP_SERVER = '([^']+)';", STRANKA).group(1)
GIF = bytes.fromhex('47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b')
TOKEN = '0123456789abcdef0123456789abcdef'
KOD = 'NAP-7K3Q-M9XD'
vystup = []


def kontrola(nazev, ok, detail=''):
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '   -> ' + str(detail)[:400]))
    print(vystup[-1], flush=True)


def odlozeny(html):
    return re.sub(r'<script>', '<script type="text/nap-odlozeno">', wptexturize(html)) + (
        "<script>window.addEventListener('load', function () { setTimeout(function () {"
        " var s = document.querySelectorAll('script[type=\"text/nap-odlozeno\"]');"
        " for (var i = 0; i < s.length; i++) { var n = document.createElement('script'); n.text = s[i].text; s[i].parentNode.replaceChild(n, s[i]); }"
        " }, 800); });</script>")


PODOBY = [('surova', lambda h: h), ('Gutenberg', wptexturize), ('klasicky editor', lambda h: wpautop(wptexturize(h))), ('odlozeny JavaScript', odlozeny)]


def otevri(prohl, obsah, parametry='', sirka=1200):
    ctx = prohl.new_context(viewport={'width': sirka, 'height': 900}, locale='cs-CZ', timezone_id='Europe/Prague')
    telo = ('<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
            '</head><body><div class="entry-content">' + obsah + '</div></body></html>').encode('utf-8')
    server = []

    def web(r):
        if '?page_id=1130' in r.request.url and r.request.resource_type == 'document':
            r.fulfill(status=200, content_type='text/html; charset=utf-8', body=telo)
        else:
            r.fulfill(status=200, content_type='image/gif', body=GIF)
    ctx.route('https://atarihelp.eu/**', web)
    ctx.route('https://raw.githubusercontent.com/**', lambda r: r.fulfill(status=503, body=''))   # menu -> GIFy (tady nejde o menu)

    def nap(r):
        server.append(r.request.url)
        r.fulfill(status=200, content_type='text/html; charset=utf-8', body='<p id="nap">SERVER NAP</p>')
    ctx.route(SERVER + '/**', nap)
    s = ctx.new_page()
    chyby = []
    s.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:160]))
    s.goto('https://atarihelp.eu/?page_id=1130' + parametry, wait_until='load')
    return ctx, s, server, chyby


def cekej_na(s, fn, ms=6000):
    for _ in range(ms // 100):
        if fn():
            return True
        s.wait_for_timeout(100)
    return fn()


def main():
    kontrola('G0 adresa serveru ve skriptu = adresa u tlacitka (pro jistotu, kdyby skript nebezel)',
             ('href="' + SERVER + '/registrace"') in STRANKA and 'id="napRegBtn"' in STRANKA, SERVER)
    with sync_playwright() as p:
        prohl = p.chromium.launch()
        for nazev, filtr in PODOBY:
            obsah = filtr(STRANKA)
            ctx, s, server, chyby = otevri(prohl, obsah)
            s.wait_for_timeout(1500)
            b = s.locator('#napRegBtn')
            pod = s.evaluate("() => { const a = document.getElementById('mainDownloadBtn').getBoundingClientRect(), r = document.getElementById('napRegBtn').getBoundingClientRect();"
                             " return [r.top - a.bottom, r.width, r.height, getComputedStyle(document.getElementById('napRegBtn')).color]; }")
            kontrola('G1 %s: tlacitko REGISTRACE videt pod Stahnout, vede na %s/registrace, text, zadna chyba JS' % (nazev, SERVER),
                     b.is_visible() and b.get_attribute('href') == SERVER + '/registrace' and 'Registrace / Register' in b.text_content()
                     and 0 < pod[0] < 120 and pod[2] > 30 and not chyby, (b.get_attribute('href'), pod, chyby))
            b.click()
            kontrola('G1 %s: klik na tlacitko -> stranka registrace na serveru NAP' % nazev,
                     cekej_na(s, lambda: any(u == SERVER + '/registrace' for u in server)), server)
            ctx.close()

            ctx, s, server, chyby = otevri(prohl, obsah, '&nap_potvrdit=' + TOKEN)
            kontrola('G2 %s: odkaz z e-mailu nap_potvrdit -> hned %s/registrace/potvrdit?t=TOKEN' % (nazev, SERVER),
                     cekej_na(s, lambda: (SERVER + '/registrace/potvrdit?t=' + TOKEN) in server) and not chyby, (server, chyby))
            ctx.close()

            ctx, s, server, chyby = otevri(prohl, obsah, '&nap_kod=' + KOD)
            ok = cekej_na(s, lambda: s.is_visible('#promptModal'))
            hodnota = s.input_value('#codeInput') if ok else ''
            kontrola('G3 %s: odkaz z e-mailu nap_kod -> okno kodu s vyplnenym %s, nic se jeste neposlalo' % (nazev, KOD),
                     ok and hodnota == KOD and server == [] and not chyby, (ok, hodnota, server, chyby))
            if ok:
                s.click('text=Pokračovat / Continue')
                kontrola('G3 %s: Pokracovat -> %s/aktivace?kod=%s' % (nazev, SERVER, KOD),
                         cekej_na(s, lambda: (SERVER + '/aktivace?kod=' + KOD) in server), server)
            ctx.close()

            for par, popis in (('&nap_potvrdit=0123abc', 'kratky token'), ('&nap_potvrdit=' + TOKEN.upper(), 'token velkymi (server dava male)'),
                               ('&nap_kod=%3Cb%3Ex%3C%2Fb%3E', 'nap_kod s HTML'), ('&nap_kod=NAP-0000-1111', 'kod se znaky 0 a 1'),
                               ('&nap_kod=%E0%A4%A', 'rozbite kodovani')):
                ctx, s, server, chyby = otevri(prohl, obsah, par)
                s.wait_for_timeout(1600)   # odlozeny JavaScript se spusti az 0,8 s po nacteni
                kontrola('G4 %s: %s -> nic (zadne okno, zadny server), stranka bezi' % (nazev, popis),
                         not s.is_visible('#promptModal') and server == [] and s.evaluate('!!window.nap1130Bezi')
                         and s.evaluate("document.getElementById('napRegBtn').href") == SERVER + '/registrace', (server, chyby))
                ctx.close()

        ctx, s, server, chyby = otevri(prohl, wptexturize(STRANKA), sirka=412)
        s.wait_for_timeout(1200)
        r = s.evaluate("() => { const r = document.getElementById('napRegBtn').getBoundingClientRect(); return [r.left, r.right, document.documentElement.scrollWidth]; }")
        s.locator('#napRegBtn').scroll_into_view_if_needed()
        s.screenshot(path=os.path.join(tempfile.gettempdir(), 'nap_v12_nahled_telefon_412.png'))   # jen pro oko, do repa ne
        kontrola('G5 telefon 412 px: tlacitko cele na obrazovce, stranka nepreteka do strany', r[0] >= 0 and r[1] <= 412 and r[2] <= 412, r)
        ctx.close()
        prohl.close()
    chyby = [v for v in vystup if v.startswith('CHYBA')]
    vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (len(vystup) - len(chyby), len(chyby)))
    print(vystup[-1])
    open(os.path.join(ZDE, 'VYSTUP_test_web_registrace.txt'), 'w', encoding='utf-8').write(
        'v12 stranka 1130 - tlacitko REGISTRACE a odkazy z e-mailu (Chromium, podoby jako z WordPressu)\n' + '\n'.join(vystup) + '\n')
    return 1 if chyby else 0


if __name__ == '__main__':
    sys.exit(main())
