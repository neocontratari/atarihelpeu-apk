#!/usr/bin/env python3
"""B314: STRANKY APPKY CESKY / ANGLICKY (Chromium) - Atari, Sega, PS1, MP3, DZUBOX.

Pro kazdou stranku (PRESNE z appky, mosty Javy falesne) a jeji stavy (tlacitka, panely):
  a) cesky: texty stejne jako v B313 (krome toho, co se v B314 zamerne zmenilo - vypsano)
  b) anglicky: zadny zbytek cestiny (texty, atributy, titulek, alert / confirm, canvas)
  c) zadna NOVA chyba JavaScriptu (proti B313 se stejnymi mosty)
Vsechny anglicke texty kazde stranky jdou do VYSTUP_texty_<stranka>_en.txt (kontrola ocima).
Pouziti: python3 test_b314_jazyk_stranky.py [jmeno ...]      -> VYSTUP_test_b314_jazyk_stranky.txt
"""
import os
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

import jazyk_harness as H
from stranky_b314 import STRANKY

ZDE = H.ZDE
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)


def projdi(b, zaklad, st, jazyk):
    """Vsechny stavy stranky -> [(popis, sber)], chyby JS."""
    chyby = []
    ctx = b.new_context(viewport=st.get('okno', {'width': 412, 'height': 846}))
    ctx.add_init_script(H.init_skript(jazyk, st.get('mock', {})))
    pg = ctx.new_page()
    pg.on('pageerror', lambda e: chyby.append(str(e).split('\n')[0][:200]))
    pg.goto(zaklad + st['cesta'])
    pg.wait_for_timeout(st.get('start_ms', 600))
    sbery = [('nacteno', pg.evaluate(H.SBER_JS))]
    for popis, js, cekej in st.get('kroky', []):
        try:
            pg.evaluate("() => { %s }" % js)
        except Exception as e:
            chyby.append('krok "%s": %s' % (popis, str(e).split('\n')[0][:160]))
        pg.wait_for_timeout(cekej)
        try:
            sbery.append((popis, pg.evaluate(H.SBER_JS)))
        except Exception as e:
            chyby.append('sber po "%s": %s' % (popis, str(e).split('\n')[0][:160]))
    ctx.close()
    return sbery, chyby


def main():
    vyber = set(sys.argv[1:])
    srv, zaklad = H.server()
    tmp = tempfile.mkdtemp(prefix='b314_str_')
    subprocess.run('git -C "%s" archive HEAD app/src/main/assets | tar -x -C "%s"' % (H.KOREN, tmp), shell=True, check=True)
    srv0, zaklad0 = H.server(os.path.join(tmp, 'app/src/main/assets'))
    with sync_playwright() as p:
        b = p.chromium.launch()
        for jm, st in STRANKY.items():
            if vyber and jm not in vyber:
                continue
            cz, chyby_cz = projdi(b, zaklad, st, 'cs')
            puv, chyby_puv = projdi(b, zaklad0, st, 'cs')
            en, chyby_en = projdi(b, zaklad, st, 'en')
            # a) cesky jako B313
            zmena_ok = st.get('zmenene_cesky', ())
            rozdil = []
            for (popis, a), (_, a0) in zip(cz, puv):
                for druh in ('texty', 'atributy'):
                    x, y = a[druh], a0[druh]
                    if x == y:
                        continue
                    sx, sy = set(x), set(y)
                    navic = [t for t in sx - sy if not any(z in t for z in zmena_ok)]
                    chybi = [t for t in sy - sx if not any(z in t for z in zmena_ok)]
                    if navic or chybi or (sx == sy and [t for t in x if t in sy] != [t for t in y if t in sx]):
                        rozdil.append('%s/%s: +%s -%s' % (popis, druh, navic[:3], chybi[:3]))
                if a['titulek'] != a0['titulek']:
                    rozdil.append('%s/titulek: %s -> %s' % (popis, a0['titulek'], a['titulek']))
            over(not rozdil, '%s a) cesky: %d stavu stranky - texty jako v B313%s%s' % (
                jm, len(cz), (' (zamerne zmeneno: %s)' % ', '.join(zmena_ok)) if zmena_ok else '',
                '' if not rozdil else ' - ROZDIL: ' + ' | '.join(rozdil[:4])))
            # b) anglicky bez cestiny
            zbytky = []
            vse = []
            for popis, s in en:
                for druh, t, sl in H.zbytky_cestiny(s, st.get('povoleno_en', ()), st.get('slova_en', ())):
                    zbytky.append('%s / %s: "%s" (%s)' % (popis, druh, t[:90], ','.join(sl[:4])))
                vse += s['texty'] + s['atributy'] + ['[titulek] ' + s['titulek']] + \
                    ['[dialog] ' + d for d in s['dialogy']] + ['[canvas] ' + c for c in s['canvas']]
            zbytky = list(dict.fromkeys(zbytky))
            over(not zbytky, '%s b) anglicky: %d stavu stranky bez cestiny (%d ruznych textu)%s' % (
                jm, len(en), len(set(vse)), '' if not zbytky else ' - ZBYTKY (%d): %s' % (len(zbytky), ' | '.join(zbytky[:10]))))
            with open(os.path.join(ZDE, 'VYSTUP_texty_%s_en.txt' % jm), 'w', encoding='utf-8') as f:
                f.write('B314 - vsechny anglicke texty stranky %s (%s), pro kontrolu ocima\n' % (jm, st['cesta']))
                for t in dict.fromkeys(vse):
                    f.write(t + '\n')
            # c) chyby JS
            nove = sorted(set(chyby_cz + chyby_en) - set(chyby_puv))
            over(not nove, '%s c) zadna nova chyba JavaScriptu (B313 se stejnymi mosty: %d, B314 cesky %d, anglicky %d)%s' % (
                jm, len(set(chyby_puv)), len(set(chyby_cz)), len(set(chyby_en)), '' if not nove else ' - NOVE: ' + ' | '.join(nove[:4])))
        b.close()
    srv.shutdown(); srv0.shutdown()
    vystup.insert(0, 'B314 - stranky appky cesky / anglicky (Chromium, stranky z appky, falesne mosty Javy)')
    vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len([v for v in vystup if v.startswith(('OK', 'CHYBA'))]), chyb))
    jm = 'VYSTUP_test_b314_jazyk_stranky.txt' if not vyber else 'VYSTUP_test_b314_jazyk_stranky_%s.txt' % '_'.join(sorted(vyber))
    open(os.path.join(ZDE, jm), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
    print(vystup[-1])
    sys.exit(1 if chyb else 0)


if __name__ == '__main__':
    main()
