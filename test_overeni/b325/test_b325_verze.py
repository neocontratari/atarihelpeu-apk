#!/usr/bin/env python3
"""B325: verze appky v OPTIONS (Rene: "udelej dalsi build s malou zmenou - napriklad v options bude videt,
jaka je verze - at muzu testnout, jestli aktualizace uz funguje").

Stranka menu (assets/index.html) v Chromiu, falesny most AHJAZYK jako v appce:
  V1 cesky: pod nadpisem OPTIONS "VERZE EMU10-B325-VERZE (368)"
  V2 anglicky: "VERSION EMU10-B325-VERZE (368)"
  V3 bez mostu (stara appka / chyba) radek neni videt, OPTIONS funguje dal (zadna chyba JS)
  V4 verze v build.gradle = B325 (368) - stejne cislo, jake ma radek ukazat
Pouziti: python3 test_b325_verze.py   -> VYSTUP_test_b325_verze.txt
"""
import os
import re
from playwright.sync_api import sync_playwright

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.abspath(os.path.join(ZDE, '..', '..'))
STRANKA = 'file://' + os.path.join(R, 'app', 'src', 'main', 'assets', 'index.html')
vystup = []


def kontrola(nazev, ok, detail=''):
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '   -> ' + str(detail)[:300]))
    print(vystup[-1], flush=True)


def otevri(prohl, jazyk, most=True):
    ctx = prohl.new_context(viewport={'width': 412, 'height': 900})
    if most:
        ctx.add_init_script("window.AHJAZYK = { kod: function () { return '%s'; }, nastav: function (k) { return k; },"
                            " verze: function () { return 'EMU10-B325-VERZE|368'; } };" % jazyk)
    s = ctx.new_page()
    chyby = []
    s.on('pageerror', lambda e: chyby.append(str(e)))
    s.goto(STRANKA, wait_until='load')
    s.wait_for_timeout(400)
    s.evaluate("window.napOtevriNastaveni && window.napOtevriNastaveni()")
    s.wait_for_timeout(300)
    vid = s.is_visible('#stVerze')
    text = s.inner_text('#stVerze') if vid else ''
    return ctx, s, vid, text, chyby


with sync_playwright() as p:
    b = p.chromium.launch()
    ctx, s, vid, text, chyby = otevri(b, 'cs')
    kontrola('V1 cesky: pod nadpisem OPTIONS "VERZE EMU10-B325-VERZE (368)"', vid and text == 'VERZE EMU10-B325-VERZE (368)' and not chyby, (vid, text, chyby))
    pod = s.evaluate("(function(){var t=document.getElementById('stTitul').getBoundingClientRect(),v=document.getElementById('stVerze').getBoundingClientRect();"
                     "return v.top>=t.bottom-2 && v.top-t.bottom<30;})()")
    kontrola('V1 radek je hned pod nadpisem OPTIONS', pod)
    ctx.close()
    ctx, s, vid, text, chyby = otevri(b, 'en')
    kontrola('V2 anglicky: "VERSION EMU10-B325-VERZE (368)"', vid and text == 'VERSION EMU10-B325-VERZE (368)' and not chyby, (vid, text, chyby))
    ctx.close()
    ctx, s, vid, text, chyby = otevri(b, 'cs', most=False)
    ok = (not vid) and not chyby and s.is_visible('#stZavrit')
    kontrola('V3 bez mostu radek neni videt, OPTIONS funguje (ZAVRIT videt, zadna chyba JS)', ok, (vid, chyby))
    ctx.close()
    b.close()

g = open(os.path.join(R, 'app', 'build.gradle'), encoding='utf-8').read()
kontrola('V4 build.gradle: versionCode 368, versionName EMU10-B325-VERZE',
         re.search(r'versionCode 368\b', g) is not None and 'versionName "EMU10-B325-VERZE"' in g)
ok = sum(1 for x in vystup if x.startswith('OK'))
vystup.append('')
vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (ok, len(vystup) - 1 - ok))
print(vystup[-1])
open(os.path.join(ZDE, 'VYSTUP_test_b325_verze.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
