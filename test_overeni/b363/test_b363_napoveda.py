#!/usr/bin/env python3
"""B363: NAPOVEDA - kapitola "Online - dva telefony (Atari)" v Chromiu (skutecne stranky, falesne mosty Javy jako
../b314/jazyk_harness).

  N1) CS i EN: blok #onlineatari existuje a ma tlacitka presne jako okna appky (ZALOZIT HRU / PRIPOJIT SE /
      UKONCIT ONLINE, KAZDY SVUJ JOYSTICK / OBA NA JOYSTICKU 1, ZADAT CISLO HRY... - anglicky CREATE A GAME / JOIN /
      END ONLINE, EACH THEIR OWN JOYSTICK / BOTH ON JOYSTICK 1, TYPE THE GAME NUMBER...) a texty pod pristrojem
      z nap_atari_native.cpp (POSILAM HRU HRACI 2..., PRIJIMAM HRU OD HRACE 1..., SETRNE / ECO)
  N2) oba odkazy na kapitolu (tlacitko ONLINE v seznamu tlacitek pristroje, TIP u online Segy) dojedou na blok
  N3) hledani "šetrné" / "economy" a "joysticku 1" / "joystick 1" najde kapitolu s blokem
  J)  zadna chyba JavaScriptu
Pouziti (z test_overeni/b363): python3 test_b363_napoveda.py [PRAC]  -> VYSTUP_test_b363_napoveda.txt (+ obrazky v PRAC)
"""
import os
import sys

from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ZDE, '..', 'b314'))
import jazyk_harness as H                      # noqa: E402

PRAC = sys.argv[1] if len(sys.argv) > 1 else os.path.join('/tmp', 'nap_b363')
os.makedirs(PRAC, exist_ok=True)

vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def pockej_na_klid(pg, max_ms=6000):
    pg.wait_for_timeout(150)
    pg.evaluate("() => { window.__klidY = -1; window.__klidN = 0; }")
    pg.wait_for_function("""() => { const y = window.scrollY;
        if (window.__klidY === y) window.__klidN++; else { window.__klidY = y; window.__klidN = 0; }
        return window.__klidN >= 6; }""", polling=50, timeout=max_ms)


chybyJs = []
TEXTY = {
    'cs': ['ZALOŽIT HRU', 'PŘIPOJIT SE', 'UKONČIT ONLINE', 'KAŽDÝ SVŮJ JOYSTICK', 'OBA NA JOYSTICKU 1', 'ZADAT ČÍSLO HRY...',
           'ONLINE: POSÍLÁM HRU HRÁČI 2...', 'ONLINE: PŘIJÍMÁM HRU OD HRÁČE 1...', 'ŠETRNĚ', 'SETRNE KRESLENI'],
    'en': ['CREATE A GAME', 'JOIN', 'END ONLINE', 'EACH THEIR OWN JOYSTICK', 'BOTH ON JOYSTICK 1', 'TYPE THE GAME NUMBER...',
           'ONLINE: SENDING THE GAME...', 'ONLINE: RECEIVING THE GAME...', 'ECO', 'SETRNE KRESLENI'],
}
HLEDAT = {'cs': ['šetrné', 'joysticku 1'], 'en': ['economy', 'joystick 1']}

srv, zaklad = H.server()
with sync_playwright() as p:
    b = p.chromium.launch()
    for jazyk, st in (('cs', 'napoveda/index.html'), ('en', 'napoveda/en.html')):
        ctx = b.new_context(viewport={'width': 412, 'height': 915}, has_touch=True, is_mobile=True, device_scale_factor=2)
        ctx.add_init_script(H.init_skript(jazyk, {}))
        pg = ctx.new_page()
        pg.on('pageerror', lambda e, st=st: chybyJs.append(st + ': ' + str(e).split('\n')[0][:200]))
        pg.goto(zaklad + st)
        pg.wait_for_timeout(1200)
        # N1 obsah bloku
        text = pg.evaluate("(document.getElementById('onlineatari') || {innerText: ''}).innerText.replace(/\\s+/g, ' ')")
        chybi = [t for t in TEXTY[jazyk] if t not in text]
        over(len(text) > 500 and not chybi, 'N1) %s blok #onlineatari (%d znaku) - tlacitka a texty jako v appce%s'
             % (jazyk.upper(), len(text), '' if not chybi else ' - CHYBI: ' + ', '.join(chybi)))
        # N2 odkazy
        lista = pg.evaluate("document.querySelector('.lista').offsetHeight")
        odkazu = pg.evaluate("document.querySelectorAll('a[data-skok=\"onlineatari\"]').length")
        over(odkazu == 2, 'N2) %s odkazu na kapitolu: %d (ONLINE v tlacitkach pristroje + TIP u Segy)' % (jazyk.upper(), odkazu))
        for i in range(odkazu):
            pg.evaluate("i => window.scrollTo(0, 0)", i)
            pg.wait_for_timeout(200)
            kde = pg.evaluate("""i => { const a = document.querySelectorAll('a[data-skok="onlineatari"]')[i];
                a.scrollIntoView({block: 'center'}); a.click();
                return '#' + (a.closest('[id]') || {id: '?'}).id + ' ' + a.closest('dd, p').tagName.toLowerCase(); }""", i)
            pockej_na_klid(pg)
            top = pg.evaluate("document.getElementById('onlineatari').getBoundingClientRect().top")
            over(0 <= top - lista <= 30, 'N2) %s odkaz %d (z %s) dojede na Online - dva telefony (Atari) (top %.0f, lista %d)'
                 % (jazyk.upper(), i + 1, kde, top, lista))
            pg.screenshot(path=os.path.join(PRAC, 'napoveda_%s_odkaz%d.png' % (jazyk, i + 1)))
        # N3 hledani
        for slovo in HLEDAT[jazyk]:
            pg.fill('#hledat', slovo)
            pg.wait_for_timeout(600)
            vid = pg.evaluate("document.getElementById('onlineatari').offsetParent !== null")
            kap = pg.evaluate("(document.getElementById('onlineatari').closest('section.kap') || {id: '?'}).id")
            over(vid, 'N3) %s hledani "%s" najde kapitolu %s s blokem online Atari' % (jazyk.upper(), slovo, kap))
        pg.fill('#hledat', '')
        ctx.close()
    b.close()
srv.shutdown()
over(not chybyJs, 'J) zadna chyba JavaScriptu' + ('' if not chybyJs else ': ' + ' | '.join(chybyJs[:3])))
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b363_napoveda.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
